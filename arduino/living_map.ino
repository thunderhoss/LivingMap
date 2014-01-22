/*

LEDs driven by FASTSPI https://code.google.com/p/fastspi/ (MIT License)
LED animations courtesy of http://funkboxing.com/wordpress/

Uses audio_slave sketch running on another arduino
The arduinos communicate using EasyTransfer by Bill Porter.
//http://www.billporter.info/2011/05/30/easytransfer-arduino-library/
*/

#include "SPI.h"
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <TextFinder.h>
#include "FastSPI_LED2.h"
#include <Time.h> 
#include <EasyTransfer.h>
#include <MemoryFree.h>

//EasyTransfer stuff
//create object
EasyTransfer ET; 

struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int f_name;  
  int d_name;  
};

//give a name to the group of data
SEND_DATA_STRUCTURE mydata;

//Mac address of Ethernet shield
byte g_mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0xAF };
EthernetClient g_client;     
char g_host[] = "www.gt140.co.uk";

//birthday_array - defines not used (yet) but useful - they identify the birthdays in the birthdays array.
#define BARRY 0
#define BEN 1
#define CLARE 2
#define DAVID 3
#define GRAHAM 4
#define JEN 5
#define KAREN 6
#define MIKEG 7
#define MIKEP 8
#define RICH 9
#define NO_OF_US 10
typedef struct {
  int day_of_month;
  int month_of_year;
} dob;

dob g_birthdays[NO_OF_US] = {{13, 12}, {26, 3}, {10, 9}, {5, 5}, {8, 7}, {2, 9}, {1, 4}, {2, 8}, {31, 8}, {7, 5}};

//sound stuff - these defines also need to be exactly the same in audio slave
#define BIRTH0 0
#define BIRTH1 1
#define BIRTH2 2
#define COPS1 3
#define COPS2 4
#define LUNCH1 5
#define LUNCH2 6
#define MORNING1 7
#define MORNING2 8
#define POLL1 9
#define POLL2 10
#define THE_END1 11
#define THE_END2 12
#define TRAFFIC1 13
#define TRAFFIC2 14
#define THUNDER1 15
#define HORSE1 16
#define XMAS1 17

//Twitter stuff
#define NO_TWITTER_ACCS 3
//defines the length of the period which we consider recent - last 15 mins in this case
#define TWITTER_PERIOD 15
#define SALFORD_CITY_COPS 0
#define THUNDER_DEV 1
#define THUNDERHOSS 2
typedef struct {
  char account[20];
  char last_tweet[140];
  //This is used to make sure accounts are checked once every 15 minutes.
  //If the account has been mentioned in the last 15 minutes an event will occur.
  //We can stagger the accounts so that only one account will hit the 15 minute mark in a given loop
  //and we won't have to process multiple twitter events at once.
  int minuteCounter;
  boolean recentTweet;
  boolean firstTweet;
} twitter;
char g_tweet[140] = "";

twitter g_twitterAccs[NO_TWITTER_ACCS]={{"GMPSalfordCen", "", 1, false, true}, {"thunder_dev", "", 5, false, true}, {"thunderhoss","", 10, false, true}};
int g_currentAccount = 0;

//Time stuff
//NTP servers
IPAddress g_timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
//IPAddress g_timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress g_timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
const int g_timeZone = 0;     // Greenwich Meantime

EthernetUDP Udp;
unsigned int g_localPort = 8888;  // local port to listen for UDP packets
time_t g_time;

//LED stuff
#define NUM_LEDS 100
CRGB g_leds[NUM_LEDS];

int g_brightnessPin = A2;
int g_brightnessValRead = 0;
int g_brightnessVal = 0;

//animation variables
int g_BOTTOM_INDEX = 0;
int g_TOP_INDEX = int(NUM_LEDS/2);
int g_EVENODD = NUM_LEDS%2;
int g_ledsX[NUM_LEDS][3]; //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, ETC)
int g_idex = 0;        //-LED INDEX (0 to NUM_LEDS-1
int g_idx_offset = 0;  //-OFFSET INDEX (BOTTOM LED TO ZERO WHEN LOOP IS TURNED/DOESN'T REALLY WORK)
int g_ihue = 0;        //-HUE (0-360)
int g_ibright = 0;     //-BRIGHTNESS (0-255)
int g_isat = 0;        //-SATURATION (0-255)
int g_bouncedirection = 0;  //-SWITCH FOR COLOR BOUNCE (0-1)
float g_tcount = 0.0;      //-INC VAR FOR SIN LOOPS
int g_lcount = 0;      //-ANOTHER COUNTING VAR

//Traffic
boolean g_trafficRedEventOccurred = false;
int g_percentages[NUM_LEDS];
#define TRAFFIC_GREEN 72
#define TRAFFIC_YELLOW 32
//everything else red
#define MAX_STRING_LEN 400
char g_buffer[MAX_STRING_LEN];

//Pollution
int g_pollIndex = 0;
boolean g_pollutionEventOccurred = false;

//Events
//time events
boolean g_snooze = false;
boolean g_eightAmEventOccurred = false;
boolean g_elevenAmEventOccurred = false;
boolean g_twelvePmEventOccurred = false;
boolean g_xMasEventOccurred = false;
boolean g_sixPmEventOccurred = false;

int breathePin = 26;

void setup() {
  //Wait for audio slave to start (setup has delay of at least 5 seconds)
  delay(10000);
  
  Serial.begin(9600);
  Serial.println("Start setup...");    
  Serial.print("Timestamp:");    
  Serial.println(now());

  pinMode(breathePin, OUTPUT);

  Serial.println("Initialise EasyTransfer..."); 
  ET.begin(details(mydata), &Serial);
  Serial.println("done.");

  //Get IP address
  Serial.println("Looking for IP address via DHCP...");    
  while (Ethernet.begin(g_mac) != 1) {
    Serial.println("Error getting IP address via DHCP, trying again...");
    delay(15000);
  }  
  Serial.println("Got IP address.");

  Serial.println("Getting LED strip...");
  //Set LED's up for bit banging
  LEDS.addLeds<WS2801, 30, 31, RGB, DATA_RATE_MHZ(1)>(g_leds, NUM_LEDS);  
  //Would have used hardware SPI i.e. the following line but i got some
  //flicker on the 1st LED.
  //LEDS.addLeds<WS2801>(g_leds, NUM_LEDS);
  one_color_all_at_once(0, 0, 0);          

  Serial.println("done.");
  
  Serial.println("Syncing with time server...");
  Udp.begin(g_localPort);
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  Serial.println("Synced with time server.");
    
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
  
  //Initialise traffic array
  for (int i = 0; i < NUM_LEDS; i++) {
    g_percentages[i] = -1;
  }

  Serial.println("Run event test...");

  tHossEvent();
  delay(30000);
  
//  eightAmEvent();
//  delay(30000);  
//  sixPmEvent();
//  delay(30000);    
//  copEvent();
//  delay(30000);    
//  pollutionEvent();
//  delay(30000);    
//  //Note people's names are named differently to other sound samples - they start at A0.
//  //This reflects the array above.
//  //The other sounds start at A1. 
//  birthdayEvent(random(0, NO_OF_US - 1));
//  delay(30000);    
//  trafficRedEvent();
//  delay(30000);    
//  twelvePmEvent();  
//  delay(30000);    
//  tHossEvent();  
//  delay(30000);    
  Serial.println("done.");

  Serial.println("End setup.");    
}


void loop() {
  
  int i;
  time_t startLoopTime, endLoopTime;
  
  //Does it stuff once every minute - see delay at end of loop
  startLoopTime = now();
  Serial.print("Start loop at: ");
  Serial.println(startLoopTime);
  Serial.print("Free memory: ");
  Serial.println(freeMemory());
  
  digitalWrite(breathePin, HIGH);
  delay(250);
  digitalWrite(breathePin, LOW);  

  g_brightnessValRead = analogRead(g_brightnessPin);
  g_brightnessVal = map(g_brightnessValRead, 0, 1023, 32, 255);
  Serial.print("Brightness from pot: ");
  Serial.println(g_brightnessVal);
  delay(1000);
  
  // Do timed events and then return to traffic display
  g_time = now();
  Serial.print("Time: ");
  Serial.print(day(g_time));
  Serial.print("/");
  Serial.print(month(g_time));
  Serial.print("/");
  Serial.print(year(g_time));
  Serial.print(" ");
  Serial.print(hour(g_time));
  Serial.print(":");
  Serial.println(minute(g_time));
  
  if ((hour(g_time) < 8 || hour(g_time) > 18) || (weekday(g_time) == 1 || weekday(g_time) == 7)) {
    Serial.println("Snoozing...");
    g_snooze = true;
  } else if (hour(g_time) == 8) {
    if (g_eightAmEventOccurred == false) {
      eightAmEvent();
      g_eightAmEventOccurred = true;
      //Wake up the map
      Serial.println("Waking...");      
      g_snooze = false;
    }
  } else if (hour(g_time) == 11) {
    if (g_elevenAmEventOccurred == false) {
      elevenAmEvent();
      g_elevenAmEventOccurred = true;        
    }
  } else if (hour(g_time) == 12) {
    if (g_twelvePmEventOccurred == false) {
      twelvePmEvent();
      g_twelvePmEventOccurred = true;    
    }
  } else if (hour(g_time) == 15 && month(g_time) == 12  && (day(g_time) >= 1 && day(g_time) <= 24)) {
    if (g_xMasEventOccurred == false) {
      xMasEvent();
      g_xMasEventOccurred = true;    
    }    
  } else if (hour(g_time) == 18) {
     if (g_sixPmEventOccurred == false) {
       sixPmEvent();
       g_sixPmEventOccurred = true;        
       //Snooze the map
       Serial.println("Snoozing...");
       g_snooze = true;
     }
  } else {
    g_snooze = false;
    g_eightAmEventOccurred = false;
    g_elevenAmEventOccurred = false;
    g_twelvePmEventOccurred = false;
    g_xMasEventOccurred = false;
    g_sixPmEventOccurred = false;  
  }
    
  //if we have traffic and we're not snoozing show it
  if (g_percentages[0] != -1 && !g_snooze) {
    setTrafficLEDs();
  }
  
  //Hit living map php page for HPE services
  Serial.println("Connecting to server...");
  if (g_client.connect(g_host, 80)) {

    Serial.println("Connected");
  
    g_client.println("GET /living_map/living_map.php HTTP/1.1");
    g_client.print("Host: ");
    g_client.println(g_host);
    g_client.println("User-Agent: Arduino/1.0");
    g_client.println();

    TextFinder finder(g_client);

    while (g_client.connected()) {
      if (g_client.available()) {
        if (finder.getString("<traffic>", "</traffic>", g_buffer, sizeof(g_buffer)) != 0) {
          Serial.println(g_buffer);  
          setPercentages(g_buffer, MAX_STRING_LEN, g_percentages, NUM_LEDS);
  //          for(i = 0; i < NUM_LEDS; i++) {
  //            Serial.println(g_percentages[i]);
  //          }
        }    
        if (finder.getString("<poll_index>", "</poll_index>", g_buffer, sizeof(g_buffer)) != 0) {
          g_pollIndex = atoi(g_buffer);
          Serial.print("Pollution index = ");
          Serial.println(g_pollIndex);
        }            
      }
    }
    delay(1);
    g_client.stop();
    Serial.println("Client stopped");
  } else {
    Serial.println("Connection failed");
  }    
    
  //Hit twitter for latest tweets from accounts
  Serial.println("Connecting to server...");
  if (g_client.connect(g_host, 80)) {
    
    Serial.println("Connected");    
    Serial.print("Getting tweets for ");
    Serial.println(g_twitterAccs[g_currentAccount].account);
    // Check twitter accounts on a rotating basis    
    g_client.print("GET /living_map/twitter_mentions.php");
    g_client.print("?username=");
    g_client.print(g_twitterAccs[g_currentAccount].account);
    g_client.println(" HTTP/1.1");
    g_client.print("Host: ");
    g_client.println(g_host);
    g_client.println("User-Agent: Arduino/1.0");
    g_client.println();

    TextFinder finder(g_client);

    while (g_client.connected()) {
      if (g_client.available()) {
        if (finder.getString("<tweet>", "</tweet>", g_tweet, 140) != 0) {
          for (i = 0; i < 140; i++) {
            if(g_twitterAccs[g_currentAccount].last_tweet[i] != g_tweet[i]) break;
          }
          if (i != 140) {
            //Set boolean to ensure twitter event is triggered
            //but don't set it for setting the first tweet encounted
            if(g_twitterAccs[g_currentAccount].firstTweet) {
              g_twitterAccs[g_currentAccount].firstTweet = false;
            } else {
              g_twitterAccs[g_currentAccount].recentTweet = true;
            }
            Serial.print("Setting last tweet: ");
            Serial.println(g_tweet);
            //copy current tweet
            for (i = 0; i < 140; i++) {
              g_twitterAccs[g_currentAccount].last_tweet[i] = g_tweet[i];
            }
          }
        }
      }
    }
    delay(1);
    g_client.stop();
    Serial.println("Client stopped");
  } else {
    Serial.println("Connection failed");
  }        
  
  //if we have traffic show it
  if (g_percentages[0] != -1 && !g_snooze) {
    Serial.println("Showing traffic after services have been hit.");
    setTrafficLEDs();
  }  

  Serial.println("Checking if recently tweeted...");  
  //if it's been 15 minutes since any account was last mentioned do the tweet event.
  for (i = 0; i < NO_TWITTER_ACCS; i++) {
    Serial.print("Checking account:");  
    Serial.println(i);
    Serial.print("Period:");  
    Serial.println(g_twitterAccs[i].minuteCounter);
    if(g_twitterAccs[i].recentTweet && g_twitterAccs[i].minuteCounter == TWITTER_PERIOD) {
      switch (i) {
        case SALFORD_CITY_COPS:
            if (!g_snooze) copEvent();
            g_twitterAccs[i].recentTweet = false;  
            break;
        case THUNDER_DEV:
            if (!g_snooze) tHossEvent();
            g_twitterAccs[i].recentTweet = false;          
            break;
        case THUNDERHOSS:
            if (!g_snooze) tHossEvent();
            g_twitterAccs[i].recentTweet = false;  
            break;          
      }
    }
  }
  
  //increment minute counters for all accounts - reset back to 1 if
  //they've gone over the period
  for (i = 0; i < NO_TWITTER_ACCS; i++) {
    g_twitterAccs[i].minuteCounter++;
    if (g_twitterAccs[i].minuteCounter > TWITTER_PERIOD) g_twitterAccs[i].minuteCounter = 1;
  }
  
  //Check a different account next time
  g_currentAccount++;
  if (g_currentAccount == NO_TWITTER_ACCS) {
    g_currentAccount = 0;
  }
  
  //Do feed events - only one will occur in a given loop
  if (isTrafficRed()) {
    if (!g_trafficRedEventOccurred) {
      Serial.println("Event: traffic");
      if (!g_snooze) trafficRedEvent();
      g_trafficRedEventOccurred = true;
    }
  } else if (g_pollIndex > 3) {
    if (!g_pollutionEventOccurred) {
      Serial.println("Event: pollution");
      if (!g_snooze) pollutionEvent();
      g_pollutionEventOccurred = true;
    }
  } else {
    g_trafficRedEventOccurred = false;
    g_pollutionEventOccurred = false;
  }
  
  //Display traffic if an event fired.  
  if (g_percentages[0] != -1 && !g_snooze) {
    Serial.println("Showing traffic after events have fired.");
    setTrafficLEDs();
  }  
  
  endLoopTime = now();
  Serial.print("End loop at: ");
  Serial.println(endLoopTime);

  if(endLoopTime - startLoopTime < 60) {
    if (60 - (endLoopTime - startLoopTime) > 0) {
      Serial.print("Delaying loop by: ");
      Serial.print(60 - (endLoopTime - startLoopTime));
      Serial.println(" secs.");    
      delay (1000 * (60 - (endLoopTime - startLoopTime)));  
    }
  }
  Serial.println("End loop.");
}

void setPercentages(char buffer[], char buffer_size, int per[], int per_size) {
  //Loop through an integer array grabbing delimited char arrays at the appropriate
  //index and add them to the integer array.
  int i;
  for (i = 0; i < per_size; i++) {
    //Note i + 1 because subStr indexes start at 1.
    per[i] =  atoi(subStr(buffer, "|", i + 1));      
  }
}

// Function to return a substring defined by a delimiter at an index
char *subStr (char *str, char *delim, int index) { 
   char *act, *sub, *ptr;
   static char copy[MAX_STRING_LEN];
   int i;

   // Since strtok consumes the first arg, make a copy
   strcpy(copy, str);

   for (i = 1, act = copy; i <= index; i++, act = NULL) {
      //Serial.print(".");
      sub = strtok_r(act, delim, &ptr);
      if (sub == NULL) break;
   }
   return sub;

}

//Returns true if any integer in the array is below the yellow threshold
//i.e. red traffic
boolean isTrafficRed() {
  for(int i=0; i < NUM_LEDS; i++) {
    if (g_percentages[i] < TRAFFIC_YELLOW) {
      Serial.print("Red traffic at: ");
      Serial.print(i);
      Serial.print(" percentage: ");
      Serial.println(g_percentages[i]);
      return true;
    }
  }
  return false;
}

void setTrafficLEDs(void) { 
    //display traffic on leds
    for (int i=0; i < 100; i++) {
      showTrafficOnLED(g_percentages[i], i); 
      //showTrafficOnLEDFade(g_percentages[i], i);
    }
    
    LEDS.setBrightness(g_brightnessVal);  

    LEDS.show(); 
}

void showTrafficOnLED(int percentage, int ledPos) {
  //72%-100% Green
  //32%-71% Yellow
  //0-32% Red
  if (percentage >= TRAFFIC_GREEN) {
    g_leds[ledPos] = CRGB(0, 255, 0);
  } else if (percentage >= TRAFFIC_YELLOW) {
    g_leds[ledPos] = CRGB(255, 121, 0);
  } else {
    g_leds[ledPos] = CRGB(255, 0, 0);
  }
}

void showTrafficOnLEDFade(int percentage, int ledPos) {
  int red = 0;
  int green = 0;

  if (percentage > 100) {
    percentage = 100;
  }

  if (percentage <= 50) {
    red = 255;
    green = (((50.0 - percentage) / 50.0) * 100.0) * (255.0 / 100.0);
  } else {
    green = 255;  
    red = (100.0 - ((percentage - 50.0) / 50.0) * 100.0) * (255.0 / 100.0);   
  }
  g_leds[ledPos] = CRGB(red, green, 0);
}

/*-------- Sound functions ----------*/
//Sound is now played from another arduino to allow lights and music.
//This uses serial comms between the arduinos - EasyTransfer arduino library.
//The other arduino runs the audio_slave sketch.
//Wasn't able to do this on just the mega due to the WaveShield SD card and the
//LEDs using SPI.
//See: https://plus.google.com/117284509321122525986/posts/8mcnrfEEd9M
void playFile(int file, int dir) {
  mydata.f_name = file;
  mydata.d_name = dir;
  //send the data
  ET.sendData();
}

void playFile(int dir) {
  mydata.f_name = -1;
  mydata.d_name = dir;
  //send the data
  ET.sendData();
}

/*-------- Event functions ----------*/
void eightAmEvent(void) {
  one_color_all_at_once(0, 0, 0);          
  playFile(MORNING1);  
  // fade up red
  for(int x = 0; x < 256; x++) { 
    // The showColor method sets all the leds in the strip to the same color
    LEDS.showColor(CRGB(x, 0, 0), x / 2);
    delay(200);
  }
  playFile(MORNING2);  
  // fade up green making yellow
  for(int x = 0; x < 129; x++) { 
    // The showColor method sets all the leds in the strip to the same color
    LEDS.showColor(CRGB(255, x, 0), x + 128);
    delay(100);
  }
  one_color_all_at_once(0, 0, 0);          
}

void elevenAmEvent(void) {
  int birthday = isItABirthday();
  //Check birthdays
  if (birthday != -1) {
    birthdayEvent(birthday);
  }
}

void twelvePmEvent(void) {
  int i,j;
  int r = 10;
  one_color_all_at_once(0, 0, 0);          
  playFile(LUNCH1);
  delay(5500);
  playFile(LUNCH2);  
  for(i=0; i < 4; i++) {
    for(j=0; j<r*20; j++) {rainbow_loop(10, 20);}
  }
  one_color_all_at_once(0, 0, 0);    
}

void xMasEvent(void) {
  //If it's December and the 1st to the 24th play a christmas
  //tune and flash some red LEDs
  int r = 90;
  one_color_all_at_once(0, 0, 0);  
  Serial.println("Xmas");
  Serial.println(day(g_time));
  playFile(day(g_time), XMAS1);  
  random_red();              
  for(int i=0; i<r*5; i++) {rule30(100);}
  one_color_all_at_once(0, 0, 0);          
}

void sixPmEvent(void) {
  one_color_all_at_once(0, 0, 0);          
  playFile(THE_END1);
  // fade down green
  for(int x = 128; x >= 0; x--) { 
    LEDS.showColor(CRGB(255, x, 0), 255 - x);
    delay(100);
  }  
  playFile(THE_END2);
  // fade down red
  for(int x = 255; x >= 0; x--) { 
    LEDS.showColor(CRGB(x, 0, 0), x / 2);
    delay(200);
  }
  one_color_all_at_once(0, 0, 0);          
}

void copEvent(void) {
  int i, j;
  int r;
  
  one_color_all_at_once(0, 0, 0);          
  playFile(COPS1);
  for(i=0; i < 5; i++) {
    r = 10;
    for(j=0; j<r*5; j++) {police_lightsONE(40);}
  }
  playFile(COPS2);
  for(i=0; i < 7; i++) {
    r = 10;
    for(j=0; j<r*5; j++) {police_lightsALL(40);}
  }  
  one_color_all_at_once(0, 0, 0);          
}

void birthdayEvent(int person) {
  int i, j;
  int r;
  
  one_color_all_at_once(0, 0, 0);          
  //drum roll etc followed by persons name
  playFile(BIRTH0);
  delay(3000);
  playFile(person, BIRTH1);
  for(i=0; i < 2; i++) {
    r = 10;
    for(j=0; j<r*20; j++) {random_burst(20);}
  }
  //random birthday track
  playFile(BIRTH2);
  for(i=0; i < 9; i++) {
    r = 10;
    for(j=0; j<r*3; j++) {strip_march_cw(100);}
  }
  one_color_all_at_once(0, 0, 0);          
}

void pollutionEvent(void) {
  int i, j;
  int r;  
  
  one_color_all_at_once(0, 0, 0);          
  playFile(POLL1);
  delay(3000);  
  playFile(POLL2);
  for(i=0; i < 6; i++) {
    r = 10;
    for(j=0; j<r*50; j++) {pulse_one_color_all(80, 2);}
  }
  one_color_all_at_once(0, 0, 0);          
}

void trafficRedEvent(void) {
  int i, j;
  int r;    
  
  one_color_all_at_once(0, 0, 0);          
  playFile(TRAFFIC1);
  delay(4000);  
  playFile(TRAFFIC2);
  for(i=0; i < 5; i++) {
    r = 10;
    for(j=0; j<r*50; j++) {pulse_one_color_all(0, 2);}
  }      
  one_color_all_at_once(0, 0, 0);          
}

//Checks to see if it's anybody's birthday.
int isItABirthday(void) {
  int i;
  for (i = 0; i < NO_OF_US; i++) {
    if (g_birthdays[i].day_of_month == day(g_time) && g_birthdays[i].month_of_year == month(g_time)) {
      return i;  
    }
  }
  return -1;
}

void tHossEvent(void) {  
  int i, j;
  int r;  
  
  one_color_all_at_once(0, 0, 0);      
  playFile(THUNDER1);
  delay(6000);  
  playFile(HORSE1);
  r = 10;
  for(int i=0; i < 2; i++) {
    for(int j=0; j<r*35; j++) {flicker(160, 0);}
  }
  one_color_all_at_once(0, 0, 0);          
}

/*-------- LED animation functions ----------*/

void police_lightsONE(int idelay) { //-POLICE LIGHTS (TWO COLOR SINGLE LED)
  g_idex++;
  if (g_idex >= NUM_LEDS) {g_idex = 0;}
  int idexR = g_idex;
  int idexB = antipodal_index(idexR);  
  for(int i = 0; i < NUM_LEDS; i++ ) {
    if (i == idexR) {set_color_led(i, 255, 0, 0);}
    else if (i == idexB) {set_color_led(i, 0, 0, 255);}    
    else {set_color_led(i, 0, 0, 0);}
  }
  LEDS.show();  
  delay(idelay);
}

void police_lightsALL(int idelay) { //-POLICE LIGHTS (TWO COLOR SOLID)
  g_idex++;
  if (g_idex >= NUM_LEDS) {g_idex = 0;}
  int idexR = g_idex;
  int idexB = antipodal_index(idexR);
  set_color_led(idexR, 255, 0, 0);
  set_color_led(idexB, 0, 0, 255);
  LEDS.show();  
  delay(idelay);
}

void random_burst(int idelay) { //-RANDOM INDEX/COLOR
  int icolor[3];  
  
  g_idex = random(0,NUM_LEDS);
  g_ihue = random(0,359);

  HSVtoRGB(g_ihue, 255, 255, icolor);
  set_color_led(g_idex, icolor[0], icolor[1], icolor[2]);
  LEDS.show();
  delay(idelay);
}

void strip_march_cw(int idelay) { //-MARCH STRIP C-W
  copy_led_array();
  int iCCW;  
  for(int i = 0; i < NUM_LEDS; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
    iCCW = adjacent_ccw(i);
    g_leds[i].r = g_ledsX[iCCW][0];
    g_leds[i].g = g_ledsX[iCCW][1];
    g_leds[i].b = g_ledsX[iCCW][2];    
  }
  LEDS.show();
  delay(idelay);
}

void one_color_all(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
      set_color_led(i, cred, cgrn, cblu);
      LEDS.show(); 
      delay(1);
    }  
}

void one_color_all_at_once(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
      set_color_led(i, cred, cgrn, cblu);
      delay(1);
    }  
    LEDS.show(); 
}

void rainbow_fade(int idelay) { //-FADE ALL LEDS THROUGH HSV RAINBOW
    g_ihue++;
    if (g_ihue >= 359) {g_ihue = 0;}
    int thisColor[3];
    HSVtoRGB(g_ihue, 255, 255, thisColor);
    for(int g_idex = 0 ; g_idex < NUM_LEDS; g_idex++ ) {
      set_color_led(g_idex,thisColor[0],thisColor[1],thisColor[2]); 
    }
    LEDS.show();    
    delay(idelay);
}

void rainbow_loop(int istep, int idelay) { //-LOOP HSV RAINBOW
  g_idex++;
  g_ihue = g_ihue + istep;
  int icolor[3];  

  if (g_idex >= NUM_LEDS) {g_idex = 0;}
  if (g_ihue >= 359) {g_ihue = 0;}

  HSVtoRGB(g_ihue, 255, 255, icolor);
  set_color_led(g_idex, icolor[0], icolor[1], icolor[2]);
  LEDS.show();
  delay(idelay);
}

void pulse_one_color_all(int ahue, int idelay) { //-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR 

  if (g_bouncedirection == 0) {
    g_ibright++;
    if (g_ibright >= 255) {g_bouncedirection = 1;}
  }
  if (g_bouncedirection == 1) {
    g_ibright = g_ibright - 1;
    if (g_ibright <= 1) {g_bouncedirection = 0;}         
  }  
    
  int acolor[3];
  HSVtoRGB(ahue, 255, g_ibright, acolor);
  
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
      set_color_led(i, acolor[0], acolor[1], acolor[2]);
    }
    LEDS.show();
    delay(idelay);
}

void flicker(int thishue, int thissat) {
  int random_bright = random(0,255);
  int random_delay = random(10,100);
  int random_bool = random(0,random_bright);
  int thisColor[3];
  
  if (random_bool < 10) {
    HSVtoRGB(thishue, thissat, random_bright, thisColor);

    for(int i = 0 ; i < NUM_LEDS; i++ ) {
      set_color_led(i, thisColor[0], thisColor[1], thisColor[2]);
    }
    
    LEDS.show();  
    delay(random_delay);
  }
}

void random_red() { //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED  
  int temprand;
  for(int i = 0; i < NUM_LEDS; i++ ) {
    temprand = random(0,100);
    if (temprand > 50) {g_leds[i].r = 255;}
    if (temprand <= 50) {g_leds[i].r = 0;}
    g_leds[i].b = 0; g_leds[i].g = 0;
  }
  LEDS.show();
}

void rule30(int idelay) { //1D CELLULAR AUTOMATA - RULE 30 (RED FOR NOW)
  copy_led_array();
  int iCW;
  int iCCW;
  int y = 100;  
  for(int i = 0; i < NUM_LEDS; i++ ) {
    iCW = adjacent_cw(i);
    iCCW = adjacent_ccw(i);
    if (g_ledsX[iCCW][0] > y && g_ledsX[i][0] > y && g_ledsX[iCW][0] > y) {g_leds[i].r = 0;}
    if (g_ledsX[iCCW][0] > y && g_ledsX[i][0] > y && g_ledsX[iCW][0] <= y) {g_leds[i].r = 0;}
    if (g_ledsX[iCCW][0] > y && g_ledsX[i][0] <= y && g_ledsX[iCW][0] > y) {g_leds[i].r = 0;}
    if (g_ledsX[iCCW][0] > y && g_ledsX[i][0] <= y && g_ledsX[iCW][0] <= y) {g_leds[i].r = 255;}
    if (g_ledsX[iCCW][0] <= y && g_ledsX[i][0] > y && g_ledsX[iCW][0] > y) {g_leds[i].r = 255;}
    if (g_ledsX[iCCW][0] <= y && g_ledsX[i][0] > y && g_ledsX[iCW][0] <= y) {g_leds[i].r = 255;}
    if (g_ledsX[iCCW][0] <= y && g_ledsX[i][0] <= y && g_ledsX[iCW][0] > y) {g_leds[i].r = 255;}
    if (g_ledsX[iCCW][0] <= y && g_ledsX[i][0] <= y && g_ledsX[iCW][0] <= y) {g_leds[i].r = 0;}
  }
  
  LEDS.show();
  delay(idelay);
}

/*-------- LED animation helper functions ----------*/

//-SET THE COLOR OF A SINGLE RGB LED
void set_color_led(int adex, int cred, int cgrn, int cblu) {  
  int bdex;
  
  if (g_idx_offset > 0) {  //-APPLY INDEX OFFSET 
    bdex = (adex + g_idx_offset) % NUM_LEDS;
  }
  else {bdex = adex;}
  
  g_leds[bdex].r = cred;
  g_leds[bdex].g = cgrn;
  g_leds[bdex].b = cblu;  
}

//-FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
  //-ONLY WORKS WITH INDEX < TOPINDEX
  if (i == g_BOTTOM_INDEX) {return g_BOTTOM_INDEX;}
  if (i == g_TOP_INDEX && g_EVENODD == 1) {return g_TOP_INDEX + 1;}
  if (i == g_TOP_INDEX && g_EVENODD == 0) {return g_TOP_INDEX;}
  return NUM_LEDS - i;  
}

//-FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
  //int N2 = int(NUM_LEDS/2);
  int iN = i + g_TOP_INDEX;
  if (i >= g_TOP_INDEX) {iN = ( i + g_TOP_INDEX ) % NUM_LEDS; }
  return iN;
}

//-FIND ADJACENT INDEX CLOCKWISE
int adjacent_cw(int i) {
  int r;
  if (i < NUM_LEDS - 1) {r = i + 1;}
  else {r = 0;}
  return r;
}

//-FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {r = i - 1;}
  else {r = NUM_LEDS - 1;}
  return r;
}

//-CONVERT HSV VALUE TO RGB
void HSVtoRGB(int hue, int sat, int val, int colors[3]) {
	// hue: 0-359, sat: 0-255, val (lightness): 0-255
	int r, g, b, base;

	if (sat == 0) { // Achromatic color (gray).
		colors[0]=val;
		colors[1]=val;
		colors[2]=val;
	} else  {
		base = ((255 - sat) * val)>>8;
		switch(hue/60) {
			case 0:
				r = val;
				g = (((val-base)*hue)/60)+base;
				b = base;
				break;
			case 1:
				r = (((val-base)*(60-(hue%60)))/60)+base;
				g = val;
				b = base;
				break;
			case 2:
				r = base;
				g = val;
				b = (((val-base)*(hue%60))/60)+base;
				break;
			case 3:
				r = base;
				g = (((val-base)*(60-(hue%60)))/60)+base;
				b = val;
				break;
			case 4:
				r = (((val-base)*(hue%60))/60)+base;
				g = base;
				b = val;
				break;
			case 5:
				r = val;
				g = base;
				b = (((val-base)*(60-(hue%60)))/60)+base;
				break;
		}
		colors[0]=r;
		colors[1]=g;
		colors[2]=b;
	}
}

void copy_led_array(){
  for(int i = 0; i < NUM_LEDS; i++ ) {
    g_ledsX[i][0] = g_leds[i].r;
    g_ledsX[i][1] = g_leds[i].g;
    g_ledsX[i][2] = g_leds[i].b;
  }  
}

void print_led_arrays(int ilen){
  copy_led_array();
  Serial.println("~~~***ARRAYS|idx|led-r-g-b|ledX-0-1-2");
  for(int i = 0; i < ilen; i++ ) {
      Serial.print("~~~");
      Serial.print(i);
      Serial.print("|");      
      Serial.print(g_leds[i].r);
      Serial.print("-");
      Serial.print(g_leds[i].g);
      Serial.print("-");
      Serial.print(g_leds[i].b);
      Serial.print("|");      
      Serial.print(g_ledsX[i][0]);
      Serial.print("-");
      Serial.print(g_ledsX[i][1]);
      Serial.print("-");
      Serial.println(g_ledsX[i][2]);
    }  
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(g_timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + g_timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

