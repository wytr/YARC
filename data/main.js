function loadProfiles(){
    getFile('/profiles.json', function(response) { 
		var profiles = JSON.parse(response).profiles;
		var ulProfiles = document.getElementById('ulProfiles');
		profiles.forEach(profile => {
			ulProfiles.appendChild(createProfileListItem(profile.name));
		});		
	});
}

function createProfileListItem(name) {
	var li = document.createElement('li');
	li.setAttribute('id', 'li' + name);
	li.setAttribute('class', 'collection-item');
  	li.appendChild(document.createTextNode(name));
	// var updateBtn = document.createElement('a');
	// updateBtn.setAttribute('href', '/profile?name=' + name)
	// updateBtn.setAttribute('class', 'waves-effect waves-light btn-small');
	// updateBtn.innerText = 'update';
	// li.appendChild(updateBtn);
	var removeBtn = document.createElement('a');
	removeBtn.setAttribute('onclick', 'removeProfile("' + name + '")')
	removeBtn.setAttribute('class', 'waves-effect waves-light btn-small remove-btn red darken-4');
	removeBtn.innerText = 'Remove'
	li.appendChild(removeBtn);
	return li;
}

function removeProfile(name) {
	var json = {
		name: name		
	}
	postRequest('remove-profile', json, function() { document.getElementById('li' + name).remove() });		
}


function getFile(adress, callback) {
	if (adress === undefined) return;
	if (callback === undefined) callback = function () { };
	var onTimeout = function () {
        console.log('ERROR: timeout loading file ' + adress);
    };
	
	var onError = function () {
        console.log('ERROR: loading file: ' + adress);
    };	

	var request = new XMLHttpRequest();

	request.open('GET', encodeURI(adress), true);
	request.timeout = 8000;
	request.ontimeout = onTimeout;
	request.onerror = onError;
	request.overrideMimeType('application/json');

	request.onreadystatechange = function () {
		if (this.readyState == 4) {
			if (this.status == 200) {
				callback(this.responseText);
			}
		}
	};
	request.send();
}

function createProfile(event) {	
	var createProfileForm = document.getElementById('createProfileForm');
	if(createProfileForm.checkValidity()) {
		event.preventDefault();
		var json = {
			name: document.getElementById('name').value,
			soakRampRate: document.getElementById('soakRampRate').value,
			soakTemperature: document.getElementById('soakTemperature').value,
			soakDuration: document.getElementById('soakDuration').value,
			reflowRampRate: document.getElementById('reflowRampRate').value,
			reflowTemperature: document.getElementById('reflowTemperature').value,
			reflowDuration: document.getElementById('reflowDuration').value
		}
		postRequest('create-profile', json, function() { window.location = '/' } );	
	}	
}

function postRequest(adress, json, callback) {
	fetch(adress, {
		method: 'POST',
		headers: {
		  'Content-Type': 'application/json',
		},
		body: JSON.stringify(json),
	}).then(callback);
}

