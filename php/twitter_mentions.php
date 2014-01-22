<?php

    ini_set('display_errors', 1);

    require_once('TwitterAPIExchange.php');
	
	function get_tweets($url, $user_name, $since_id) {
		//Get tweets using provided URL, user name and since_id
		//if since id is omitted only one tweet is returned.
		//Return first tweet and id surrounded by <tweet> and <id> tags.
		
	    //Set access tokens here - see: https://dev.twitter.com/apps/
		$settings = array(
			'oauth_access_token' => "YOUR_OAUTH_ACCESS_TOKEN",
			'oauth_access_token_secret' => "YOUR_OAUTH_ACCESS_TOKEN_SECRET",
			'consumer_key' => "YOUR_CONSUMER_KEY",
			'consumer_secret' => "YOUR_CONSUMER_SECRET"
		);

		$getfield = '?q=@'.$user_name;
		if($since_id == "") {
			$getfield = $getfield.'&count=1';
		} else {
			$getfield = $getfield.'&since_id='.$since_id;
		}
		$requestMethod = 'GET';
		
		$twitter = new TwitterAPIExchange($settings);
		$response = $twitter->setGetfield($getfield)
					 ->buildOauth($url, $requestMethod)
					 ->performRequest();
			
		//Get since id from response
		$tweets = json_decode($response, true);
		return '<id>'.$tweets["statuses"][0]["id"].'</id><tweet>'.$tweets["statuses"][0]["text"].'</tweet>';
	}
	
	$user_name=$_GET['username'];
	$since_id=$_GET['since_id'];
	$url = 'https://api.twitter.com/1.1/search/tweets.json';
	echo get_tweets($url, $user_name, $since_id);
	echo '<BR>';
	    
?>

