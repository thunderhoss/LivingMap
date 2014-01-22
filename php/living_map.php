<?php

    ini_set('display_errors', 1);
	
	function get_pollution_index() {
		//return wind direction from a web service as a string
		//with surrounded by <poll_index>
		$url = 'http://uk-air.defra.gov.uk/rss/current_site_levels.xml';

		$ch = curl_init($url);			
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
		curl_setopt($ch, CURLOPT_USERAGENT, $_SERVER['HTTP_USER_AGENT']);
		$response = curl_exec($ch);
		$xml = simplexml_load_string($response);
		
		//parse xml and add to response string
		for ($i = 0; $i < count($xml->channel->item); $i++) {
			$item = $xml->channel->item[$i];
			$title = $item->title;
			if ($title == 'Salford Eccles') {
				$description = $item->description;
				//get numerical index from description.
				//Description is in the format:
				//<![CDATA[Location: 53&deg;22&acute;8.49&quot;N    2&deg;14&acute;35.81&quot;W <br />Current Pollution level is Low at index 2]]>			
				$index = substr($description, strpos($description, 'at index ') + strlen('at index '), strlen($description)); 
				break;
			}
		}
		
		return '<poll_index>'.$index.'</poll_index>';
	}

	function get_percentages($url, $array_tmc) {
		//queries tmcs passed in an array using the url.
		//calculates percentage current/free flow and returns pipe delimited string
		$array_per = array(
			0 => -1,
			1 => -1,
			2 => -1,
			3 => -1,
			4 => -1,
			5 => -1,
			6 => -1,
			7 => -1,
			8 => -1,
			9 => -1,
			10 => -1,
			11 => -1,
			12 => -1,
			13 => -1,
			14 => -1,
			15 => -1,
			16 => -1,
			17 => -1,
			18 => -1,
			19 => -1,
			20 => -1,
			21 => -1,
			22 => -1,
			23 => -1,
			24 => -1
		);
		
		$ch = curl_init($url);			
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
		curl_setopt($ch, CURLOPT_USERAGENT, $_SERVER['HTTP_USER_AGENT']);
		$response = curl_exec($ch);
		$xml = simplexml_load_string($response);
		
		//parse xml and add to response string
		for ($i = 0; $i < count($xml->Response->Link); $i++) {
			$link = $xml->Response->Link[$i];
			$dynamic_speed_info = $link->DynamicSpeedInfo;
			$tmc_codes = explode(' ', $link->TMCCodes);
			//calculate percentage of current vs freeflow
			$percentage = floor(($dynamic_speed_info->TrafficSpeed / $dynamic_speed_info->BaseSpeed) * 100);

			//for each tmc code assign the percentage just calculated
			foreach($tmc_codes as $tmc_code) {
				if (stripos($tmc_code, 'C07') !== false && stripos($tmc_code, 'C07N') === false && stripos($tmc_code, 'C07P') === false) {
					//get array keys which have this tmc
					foreach (array_keys($array_tmc, $tmc_code) as $array_key) {
						$array_per[$array_key] = $percentage;
					}
				}
			}
		}
		//build pipe delimited string from array containing percentages
		$return_string = '';
		for ($i = 0; $i < count($array_per); $i++) {
			$return_string = $return_string.$array_per[$i].'|';
		}
		//take off trailing pipe
		if (substr($return_string, -strlen('|'))==='|') {
			$return_string = substr($return_string, 0, strlen($return_string)-strlen('|'));
		}		
		return $return_string;
	}
	
	$array_1_25_ackwise_tmc = array(
		0 => 'C07-03432',
		1 => 'C07-03433',
		2 => 'C07-03433',
		3 => 'C07-03434',
		4 => 'C07-03434',
		5 => 'C07-03435',
		6 => 'C07-03435',
		7 => 'C07-03436',
		8 => 'C07-03436',
		9 => 'C07-03437',
		10 => 'C07-03438',
		11 => 'C07-03438',
		12 => 'C07-03439',
		13 => 'C07-03439',
		14 => 'C07-03440',
		15 => 'C07-03441',
		16 => 'C07-03442',
		17 => 'C07-03443',
		18 => 'C07-03444',
		19 => 'C07-03444',
		20 => 'C07-03445',
		21 => 'C07-03445',
		22 => 'C07-03445',
		23 => 'C07-03446',
		24 => 'C07-03446'
	);

	$array_26_50_ackwise_tmc = array(
		0 => 'C07-03447',
		1 => 'C07-03448',
		2 => 'C07-03448',
		3 => 'C07-03449',
		4 => 'C07-03449',
		5 => 'C07-03449',
		6 => 'C07-03449',
		7 => 'C07-03450',
		8 => 'C07-03451',
		9 => 'C07-03451',
		10 => 'C07-03451',
		11 => 'C07-03451',
		12 => 'C07-03452',
		13 => 'C07-03452',
		14 => 'C07-03452',
		15 => 'C07-03453',
		16 => 'C07-03453',
		17 => 'C07-03453',
		18 => 'C07-03453',
		19 => 'C07-03454',
		20 => 'C07-03455',
		21 => 'C07-03456',
		22 => 'C07-03430',
		23 => 'C07-03430',
		24 => 'C07-03431'
	);
	
	$array_1_25_ckwise_tmc = array(
		0 => 'C07+03431',
		1 => 'C07+03431',
		2 => 'C07+03430',
		3 => 'C07+03456',
		4 => 'C07+03455',
		5 => 'C07+03454',
		6 => 'C07+03454',
		7 => 'C07+03454',
		8 => 'C07+03454',
		9 => 'C07+03453',
		10 => 'C07+03453',
		11 => 'C07+03452',
		12 => 'C07+03452',
		13 => 'C07+03452',
		14 => 'C07+03452',
		15 => 'C07+03452',
		16 => 'C07+03451',
		17 => 'C07+03450',
		18 => 'C07+03450',
		19 => 'C07+03450',
		20 => 'C07+03449',
		21 => 'C07+03449',
		22 => 'C07+03448',
		23 => 'C07+03448',
		24 => 'C07+03447'
	);
	
	$array_26_50_ckwise_tmc = array(
		0 => 'C07+03447',
		1 => 'C07+03446',
		2 => 'C07+03446',
		3 => 'C07+03446',
		4 => 'C07+03445',
		5 => 'C07+03444',
		6 => 'C07+03443',
		7 => 'C07+03443',
		8 => 'C07+03442',
		9 => 'C07+03442',
		10 => 'C07+03441',
		11 => 'C07+03440',
		12 => 'C07+03440',
		13 => 'C07+03439',
		14 => 'C07+03438',
		15 => 'C07+03438',
		16 => 'C07+03437',
		17 => 'C07+03436',
		18 => 'C07+03435',
		19 => 'C07+03435',
		20 => 'C07+03434',
		21 => 'C07+03434',
		22 => 'C07+03434',
		23 => 'C07+03433',
		24 => 'C07+03432'	
	);
	
	$url_1_25_ackwise = 'http://route.nlp.nokia.com/routing/6.2/getlinkinfo.xml?app_id=YOUR_APP_ID&app_code=APP_CODE&TMCCodes=C07-03432,C07-03433,C07-03434,C07-03435,C07-03436,C07-03437,C07-03438,C07-03439,C07-03440,C07-03441,C07-03442,C07-03443,C07-03444,C07-03445,C07-03446&linkattributes=dynamicSpeedInfo,TMCCodes';	
	$url_26_50_ackwise = 'http://route.nlp.nokia.com/routing/6.2/getlinkinfo.xml?app_id=YOUR_APP_ID&app_code=YOUR_APP_CODE&TMCCodes=C07-03447,C07-03448,C07-03449,C07-03450,C07-03451,C07-03452,C07-03453,C07-03454,C07-03455,C07-03456,C07-03430,C07-03431&linkattributes=dynamicSpeedInfo,TMCCodes';		
	$url_1_25_ckwise = 'http://route.nlp.nokia.com/routing/6.2/getlinkinfo.xml?app_id=YOUR_APP_ID&app_code=YOUR_APP_CODE&TMCCodes=C07%2b03431,C07%2b03430,C07%2b03456,C07%2b03455,C07%2b03454,C07%2b03453,C07%2b03452,C07%2b03451,C07%2b03450,C07%2b03449,C07%2b03448,C07%2b03447&linkattributes=dynamicSpeedInfo,TMCCodes';
	$url_26_50_ckwise = 'http://route.nlp.nokia.com/routing/6.2/getlinkinfo.xml?app_id=YOUR_APP_ID&app_code=YOUR_APP_CODE&TMCCodes=C07%2b03447,C07%2b03446,C07%2b03445,C07%2b03444,C07%2b03443,C07%2b03442,C07%2b03441,C07%2b03440,C07%2b03439,C07%2b03438,C07%2b03437,C07%2b03436,C07%2b03435,C07%2b03434,C07%2b03433,C07%2b03432&linkattributes=dynamicSpeedInfo,TMCCodes';

	echo '<traffic>'.get_percentages($url_1_25_ackwise, $array_1_25_ackwise_tmc).'|'.get_percentages($url_26_50_ackwise, $array_26_50_ackwise_tmc).'|'.get_percentages($url_1_25_ckwise, $array_1_25_ckwise_tmc).'|'.get_percentages($url_26_50_ckwise, $array_26_50_ckwise_tmc).'</traffic>';
	
	echo get_pollution_index();
	echo '<BR>';
?>