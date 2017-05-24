var express = require('express');
var router = express.Router();
var Sensor = require('../databases/sensor');

/* GET home page. */
router.get('/', function(req, res, next) {
	Sensor.find(function(err, items) {
		res.render('index', { title: '재난안전시스템', data: items});
	});
});

module.exports = router;
