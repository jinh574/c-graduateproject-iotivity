var WebSocketServer = require('websocket').server;
var http = require('http');
var mongoose = require('mongoose');
var mqtt = require('mqtt');
var client = mqtt.connect('mqtt://localhost');
require('events').EventEmitter.defaultMaxListeners = Infinity;

//mongoose configuration
mongoose.connect('mongodb://localhost:27017/iotivity');

var Sensor = require('./databases/sensor');

var server = http.createServer(function (req, res) {
    console.log('Received request for ' + req.url);
    res.writeHead(404);
    res.end();
});

server.listen(3001, function () {
    console.log('Server is listening on port 3001');
});

wsServer = new WebSocketServer({
    httpServer: server,
    autoAcceptConnections: false
});

wsServer.on('request', function (request) {
    var connection = request.accept('starnight-iotivity', request.origin);
    connection.on('message', function (message) {
        if(message.utf8Data === 'ready' || message.utf8Data === "all") {
            Sensor.aggregate([
                {
                    $group: {
                        originalId: {$last: '$_id'},
                        _id: '$controller_id',
                        admin_name: {$last: '$admin_name'},
                        admin_tel: {$last: '$admin_tel'},
                        description: {$last: '$description'},
                        controller_name: {$last: '$controller_name'},
                        update_timestamp: {$last: '$update_timestamp'},
                        sensor_data: {$last: '$sensor_data'},
                        fire_alarm: {$last: '$fire_alarm'}
                    }
                }, {
                    $project: {
                        _id: '$originalId',
                        controller_id: '$_id',
                        controller_name: '$controller_name',
                        update_timestamp: '$update_timestamp',
                        sensor_data: '$sensor_data',
                        admin_name: '$admin_name',
                        admin_tel: '$admin_tel',
                        description: '$description',
                        fire_alarm: '$fire_alarm'
                    }
                }
            ], function(err, result) {
                if(err) {
                    console.log(err);
                    return;
                }
                connection.sendUTF(JSON.stringify(result));
            });
        }
        else {
            client.publish('iotivityCtrl', message.utf8Data);
        }
        connection.on('close', function (reasonCode, description) {
            console.log('Peer ' + connection.remoteAddress + ' disconnected.');
        });
    });
});

client.on('connect', function() {
    console.log("Connected MQTT");
});