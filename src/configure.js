var initialized = false;

var options = {};

Pebble.addEventListener("ready", function() {
	console.log("7S26: ready called!");
	initialized = true;
});

Pebble.addEventListener("showConfiguration", function() {
	// on call, make sure options represents latest persistant data
	options = JSON.parse(localStorage.getItem("settings"));
	console.log("Options being sent= " + JSON.stringify(options));
	
	// open configuation
	Pebble.openURL('http://www.domwakeling.com/pebble/7S26/7S26_config_1_7.html?'+encodeURIComponent(JSON.stringify(options)));

});

Pebble.addEventListener("webviewclosed", function(e) {
	// make a console entry
	console.log("configuration closed");

	//Using primitive JSON validity and non-empty check
	if (e.response.charAt(0) == "{" && e.response.slice(-1) == "}" && e.response.length > 5) {
		
		// set options based on the return data, and send to persistant storage
		options = JSON.parse(decodeURIComponent(e.response));
		localStorage.setItem("settings", JSON.stringify(options));
		
		// log the returned info
		console.log("Options = " + JSON.stringify(options));
		
		// we want to make a dictionary from the available information, so get the values as variables ...
		var dialcolour;
		if(options.dialcolour_black == 'checked') {
			dialcolour = 'black';
		} else {
			dialcolour = 'orange';
		}
		console.log("dial colour chosen: " + dialcolour);
				
		// ... then build a dictionary ...
		var dictionary = {
			'KEY_DIAL_COLOUR' : dialcolour
		};
		
		// ... and send it to the watch
		Pebble.sendAppMessage(dictionary,
			function(e) {
				console.log("Configuation successfuly sent to Pebble");
			},
			function(e) {
				console.log("Error sending configuration to Pebble");
			}
		);
		
	} else {
		// if we failed the test, log that configuation was cancelled
		console.log("Cancelled");
	}
});