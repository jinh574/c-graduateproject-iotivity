var bleno = require('bleno');
var bleacon = require('bleacon');
var fs = require('fs');

var uuid ='91558c37f1d34b3a924800271ddc6e42';		// 라즈베리파이 UUID
var androidUuid = 'c368ad15dd7249ecb5f9b163bc1fc254';	// 안드로이드 UUID
var measuredPower = 127;
var personList = {};

var major = 1;
var minor = 2;
if(process.argv.length == 4) {
	major = process.argv[2];
	minor = process.argv[3];
}

bleno.on('stateChange', function(state) {
	console.log('on -> stateChange: ' + state);
	if(state === 'poweredOn') {
		bleno.startAdvertisingIBeacon(uuid, major, minor, measuredPower);
	} else {
		bleno.stopAdvertising();
	}
});

bleno.on('advertisingStart', function(err) {
	console.log('on -> advertisingStart: ' + (err ? 'error' + err : 'success'));
	
	bleacon.startScanning([androidUuid]);
	setInterval(function() {
		var tmp = personList;
		var data = {};
		var makeData = [];
		for(key in tmp) {
			makeData.push(tmp[key]);
		}
		data['count'] = makeData.length;
		data['people'] = makeData;
		fs.writeFile("./json_string.txt", JSON.stringify(data), function(err) {
			if(err) {
				console.log(err);
				throw err;
			}
		});
	}, 5000);
});



bleacon.on('discover', function(bleacon) {
	var tmp = personList;
	var makeArr = [];
	for(var key in tmp) {
		if(Math.floor(Date.now() / 1000) - tmp[key].timestamp > 5) {
			delete tmp[key];
		}
	}
	var data = {
		address: bleacon.mac,
		major: bleacon.major,
		minor: bleacon.minor,
		distance: bleacon.accuracy,
		timestamp: Math.floor(Date.now() / 1000)
	}
	personList[bleacon.mac] = data;
});


