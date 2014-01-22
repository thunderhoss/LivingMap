//audio_slave is used with the living_map sketch.
//It is used to play audio so living_map can show led's and play music.
//it receives 2 integers on the serial port:
//the first is x where the file name to be played is Ax.WAV
//the second is a an array element which gives the directory on the SD card in which the file sits.

//Uses EasyTransfer by Bill Porter.
//http://www.billporter.info/2011/05/30/easytransfer-arduino-library/

#include <EasyTransfer.h>
#include <WaveHC.h>
#include <WaveUtil.h>

//SD card stuff
SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader dir;    // A directory object
FatReader file;   // This object represent the WAV file for a pi digit or period
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

//These arrays *must* be the same size
#define NO_SOUND_DIRS 18
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

char* soundDirNames[NO_SOUND_DIRS]={"BIRTH0", "BIRTH1", "BIRTH2", "COPS1", "COPS2", "LUNCH1", "LUNCH2", "MORNING1", "MORNING2", "POLL1", "POLL2", "THE_END1", "THE_END2", "TRAFFIC1", "TRAFFIC2", "THUNDER1", "HORSE1", "XMAS1"};
int soundDirSizes[NO_SOUND_DIRS];
//tune names can never be more than 8 characters (plus one for terminator).
char selectedTune[9];

//create EasyTransfer object
EasyTransfer ET; 

int ledPin = 7;

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int f_name;  
  int d_name;  
};

//give a name to the group of data
RECEIVE_DATA_STRUCTURE mydata;

void setup(){
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);

  //initialise random numbers
  randomSeed(analogRead(0));
  
  //flash the led to show we're alive.
  digitalWrite(ledPin, HIGH);
  delay(2000);
  digitalWrite(ledPin, LOW);
  delay(1000);

  //populate array of directory sizes
  for (int i = 0; i < NO_SOUND_DIRS; i++) {
    soundDirSizes[i] = getNoFilesInDir(soundDirNames[i]);
  }

  //start the library, pass in the data details and the name of the serial port. Can be Serial, Serial1, Serial2, etc. 
  ET.begin(details(mydata), &Serial);
    
}

void loop(){
  
  digitalWrite(ledPin, LOW);
  //check and see if a data packet has come in. 
  if(ET.receiveData()) {
    ////stop the currently playing wave file if necessary.
    if (wave.isplaying) {
      wave.stop();
    }
    
    digitalWrite(ledPin, HIGH);    

    if (mydata.f_name == -1) {
      //play random tune in directory  
      selectARandomTune(mydata.d_name);
    } else {
      getSelectedTune(mydata.f_name);
    }
    playfile(selectedTune, soundDirNames[mydata.d_name]);

  }
  
  //you should make this delay shorter then your transmit delay or else messages could be lost
  //delay(250);
  
}

int getNoFilesInDir(char *dir_name) {
  if (!card.init()) {
    return -1;
  }
  if (!vol.init(card)) {
    return -1;
  }
  if (!root.openRoot(vol)) {
    return -1;
  }
  if (!dir.open(root, dir_name)) {
    return -1;
  }
  return dir.count();
}


void getSelectedTune(int file) {
  sprintf(selectedTune, "A%d.WAV", file);
}

//Sets global variable selectedTune - a char buffer.
void selectARandomTune(int dir) {
  int randomTune;
  
  randomTune = random(1, soundDirSizes[dir] + 1);
  sprintf(selectedTune, "A%d.WAV", randomTune);
}

void playfile(char *file_name, char *dir_name) {
  //ignores errors - makes debug fun ;)
  if (!card.init()) {
    return;
  }
  if (!vol.init(card)) {
    return;
  }
  if (!root.openRoot(vol)) {
    return;
  }
  if (!dir.open(root, dir_name)) {
    return;
  }

  if (!file.open(dir, file_name)) {
    return;
  }
  if (!wave.create(file)) {
    return;
  }
  // ok time to play!
  wave.play();   
}
