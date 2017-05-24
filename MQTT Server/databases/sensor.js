var mongoose = require('mongoose');

var deviceSchema = mongoose.Schema({
	controller_id: {
		type: String,
		required: true
	},
	controller_name: {
		type: String,
		required: true
	},
	update_timestamp: {
		type: Number,
		required: true
	},
	admin_name: {
		type: String
	},
	admin_tel: {
		type: String
	},
	description: {
		type: String
	},
	fire_alarm: {
		type: Number,
		required: true,
		min: 0,
		max: 2
	},
	sensor_data: [{
		sensor_id: {
			type: String,
			required: true
		},
		sensor_uri: {
			type: String,
			required: true
		},
		light_state: {
			type: Number,
			required: true,
			min: 0
		},
		light_power: {
			type: Number,
			required: true,
			min: 0,
			max: 1
		},
		gas_state: {
			type: Number,
			required: true,
			min: 0,
			max: 2
		},
		gas_efflux: {
			type: Number,
			required: true
		},
		temp_state: {
			type: Number,
			required: true,
			min: 0,
			max: 2
		},
		temp: {
			type: Number,
			required: true
		},
		humi_state: {
			type: Number,
			required: true,
			min: 0,
			max: 2
		},
		humi: {
			type: Number,
			required: true
		},
		flame_state: {
			type: Number,
			required: true,
			min: 0,
			max: 2
		},
		flame_power: {
			type: Number,
			required: true
		},
		update_timestamp: {
			type: Number,
			required: true
		},
		isFire: {
			type: Number,
			required: true,
			min: 0,
			max: 2
		},
		person: {
			count: {
				type: Number
			},
			people: [{
				address: {
					type: String,
					required: true
				},
				major: {
					type: Number,
					required: true
				},
				minor: {
					type: Number,
					required: true
				},
				distance: {
					type: Number,
					required: true
				},
				timestamp: {
					type: Number,
					required: true
				}
			}]
		}
	}]
});

module.exports = mongoose.model('sensor', deviceSchema);