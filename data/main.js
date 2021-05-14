function getProfile(){
    getFile('/profiles.json', function(response) { console.log(response)});
}


function getFile(adress, callback) {
	if (adress === undefined) return;
	if (callback === undefined) callback = function () { };
	var onTimeout = function () {
        console.log("ERROR: timeout loading file " + adress);
    };
	
	var onError = function () {
        console.log("ERROR: loading file: " + adress);
    };	

	var request = new XMLHttpRequest();

	request.open("GET", encodeURI(adress), true);
	request.timeout = 8000;
	request.ontimeout = onTimeout;
	request.onerror = onError;
	request.overrideMimeType("application/json");

	request.onreadystatechange = function () {
		if (this.readyState == 4) {
			if (this.status == 200) {
				callback(this.responseText);
			}
		}
	};
	request.send();
}

function createProfile(){
	var createProfileForm = document.getElementById("createProfileForm");
	createProfileForm.checkValidity();
	var json = {
		name: document.getElementById("name").value,
		soakRampDeltaTemperature: document.getElementById("soakRampDeltaTemperature").value,
		soakTemperature: document.getElementById("soakTemperature").value,
		soakDuration: document.getElementById("soakDuration").value,
		peakRampDeltaTemperature: document.getElementById("peakRampDeltaTemperature").value,
		reflowDuration: document.getElementById("reflowDuration").value
	}
	postRequest('create-profile', json);	
}

function postRequest(adress, json) {
	fetch(adress, {
		method: 'POST',
		headers: {
		  'Content-Type': 'application/json',
		},
		body: JSON.stringify(json),
	})
}