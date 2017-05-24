var mqtt = require('mqtt');
var client = mqtt.connect('mqtt://localhost');
var mongoose = require('mongoose');
var fire_flag = 0;
var calcPersonData = null;
//mongoose configuration
mongoose.Promise = global.Promise;
mongoose.connect('mongodb://localhost:27017/iotivity');

var Sensor = require('./databases/sensor');

client.on('connect', function(){
	client.subscribe('iotivity');
});

client.on('message', function(topic, msg) {
	if(msg.toString() == ' ') {
		return;
	}
	var data = JSON.parse(msg.toString());
	var sensor = new Sensor();
	sensor.controller_id = data.controller_id;
	sensor.controller_name = data.controller_name;
	sensor.admin_name = data.admin_name;
	sensor.admin_tel = data.admin_tel;
	sensor.description = data.description;
	sensor.update_timestamp= data.update_timestamp;
	sensor.sensor_data = data.sensor_data;
	var tmp = data.sensor_data;
	var count = 0;
	var warningFlag = 0;
	var dangerPoint = 0;

	if(data.sensor_data.length == 1) {
		warningFlag++;
	}

	for(idx in tmp) {
		if(Math.floor(Date.now() / 1000) - tmp[idx].update_timestamp > 10) {
			continue;
		}
		if(tmp[idx].gas_state > 0 | tmp[idx].temp_state > 0 | tmp[idx].humi_state > 0 | tmp[idx].flame_state > 0) {
			warningFlag++;
		}
		if(tmp[idx].gas_state == 2) tmp[idx].gas_state = 4;
		if(tmp[idx].temp_state == 2) tmp[idx].temp_state = 4;
		if(tmp[idx].humi_state == 2) tmp[idx].humi_state = 4;
		if(tmp[idx].humi_state == 2) tmp[idx].flame_state =4;

		var tmpCount = tmp[idx].gas_state + tmp[idx].temp_state + tmp[idx].humi_state + tmp[idx].flame_state;
		if(dangerPoint < tmpCount) {
			dangerPoint = tmpCount;	
		}
	}
	if(warningFlag >= 2 && dangerPoint >= 6) {
		sensor.fire_alarm = 1;
		data.fire_alarm = 1;
		if(fire_flag == 0) {
			fire_flag = 1;
			client.publish('iotivityCtrl', JSON.stringify({
				code: 1,
				data: {
					controller_id: sensor.controller_id,
					type: 1
				}
			}));
		}
		
	}
	else {
		var adminAlertFlag = false;
		for(idx in tmp) {
			if(tmp[idx].isFire == 2) {
				adminAlertFlag = true;
			}
		}

		if(adminAlertFlag) {
			sensor.fire_alarm = 2;
			data.fire_alarm = 2;
		}
		else {
			sensor.fire_alarm = 0;
			data.fire_alarm = 0;
			if(fire_flag == 1) {
				fire_flag = 0;
				client.publish('iotivityCtrl', JSON.stringify({
					code: 2,
					data: {
						controller_id: sensor.controller_id
					}
				}));
			}
		}	
	}

	sensor.save(function(err) {
		if(err) {
			console.error(err);
			return;
		}
	});

	calcPerson(data);
});
function calcPerson(jsonData) {
	var checkNear = {};
	var sensors = jsonData.sensor_data;
	if(jsonData.fire_alarm > 0) {
		for(i in sensors) {
			if(sensors[i].person) {
				var data = sensors[i].person.people;
				for(j in data) {
					if((checkNear[data[j].address] ? checkNear[data[j].address] : 100) > data[j].distance) {
						if(Math.floor(Date.now() / 1000) - sensors[i].update_timestamp < 10) {
							checkNear[data[j].address] = data[j].distance;
						}
					}
				}
			}
		}
		var result = {
			code: 3,
			data: []
		};
		var sensors2 = jsonData.sensor_data;
		for(sensorIdx in sensors2) {
			if(sensors2[sensorIdx].person) {
				var tmp = sensors2[sensorIdx].person.people;
				var count = 0;
				for(personIdx in tmp) {
					for(key in checkNear) {
						if(tmp[personIdx].address == key && tmp[personIdx].distance == checkNear[key]) {
							count++;
						}
					}
				}
				result.data.push({
					controller_id: jsonData.controller_id,
					sensor_id: sensors2[sensorIdx].sensor_id,
					light_state: count
				});
			}
			else {
				result.data.push({
					controller_id: jsonData.controller_id,
					sensor_id: sensors2[sensorIdx].sensor_id,
					light_state: 0
				});
			}
		}
		if(result.data.length) {
			client.publish('iotivityCtrl', JSON.stringify(result));
		}
		console.log(result);
	}
}
function isJson(str) {
    try {
        JSON.parse(JSON.stringify(str.toString()));
    } catch (e) {
        return false;
    }
    return true;
}