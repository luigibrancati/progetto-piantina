#ifndef _READ_SENSORS_H_
#define _READ_SENSORS_H_

#include "memory.h"

struct SamplingTime: MQTTtopics {
	MemoryVarInt memoryVar;

	SamplingTime():
		MQTTtopics(),
		memoryVar("SamplingTime")
	{
		stateTopic = "abegghome/sampling_time/state";
	}
} samplingTime;

struct SoilMoisture: MQTTtopics {
	MemoryVarFloat rawVoltage;
	MemoryVarFloat percVoltage;

	SoilMoisture(uint8_t i):
		MQTTtopics(),
		rawVoltage("SoilMoistureRaw"+String(i)),
		percVoltage("SoilMoisturePerc"+String(i))
	{
		stateTopic = "abegghome/moisture"+String(i)+"/state";
		debugTopic = "abegghome/moisture"+String(i)+"/debug";
	}
};

static constexpr gpio_num_t sensorPins[6] {GPIO_NUM_36, GPIO_NUM_39, GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_32, GPIO_NUM_33};
static constexpr gpio_num_t sensorsSwitch = GPIO_NUM_2;
static SoilMoisture soilMoisture[6] {SoilMoisture(0), SoilMoisture(1), SoilMoisture(2), SoilMoisture(3), SoilMoisture(4), SoilMoisture(5)};

void getSensorVarsFromMemory(){
	logger.log("Getting all sensor variables from memory");
	preferences.begin(variablesNamespace.c_str(), false);
	samplingTime.memoryVar.updateFromMemory();
	for(uint8_t i=0; i<6; i++){
		soilMoisture[i].rawVoltage.updateFromMemory();
		soilMoisture[i].percVoltage.updateFromMemory();
	}
	preferences.end();
}

//Function to map a value on a range
float map_range(float x, float in_min, float in_max, float out_min, float out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Function to sample the soil moisture
//It performs NumberReadings number of readings of the soil moisture, at times ReadingsInt
//The final moisture sample value is the average of all the readings
//The sampled values are converted into percentages based on AirValue and WaterValue
float read_soil_moisture(uint8_t i){
	// Convert the analog read into voltage
	// ESP32 returns a value from 0 (0V) to 4095 (3.3V)
	float reading = ((float) analogRead(sensorPins[i])/4095.0)*3.3;
	logger.log("Analog read (S"+String(i)+"): "+String(reading));
	return reading;
}

float soil_moisture_percent(float soilMoistureAverage){
	float soilMoisturePercent = map_range(soilMoistureAverage, airValue.value, waterValue.value, 0.0, 100.0);
	return constrain(soilMoisturePercent, 0.0, 100.0); //Returns the percentage of moisture
}

void read_soil_moisture_percent_average(uint8_t i){
	float soilMoistureAverage = 0;
	// Read the moisture NumberReadings times, separated by ReadingsIntervalMS milliseconds
	for(int i=0;i<NumReadings;i++){
		soilMoistureAverage += ((float)read_soil_moisture(i)/NumReadings); //Average of all the readings
		delay(ReadingsInt); //Pause between two readings
	}
	soilMoisture[i].rawVoltage.setValue(soilMoistureAverage);
	soilMoisture[i].percVoltage.setValue(soil_moisture_percent(soilMoistureAverage));
}

void read_all_sensors(){
	if(digitalRead(sensorsSwitch) == HIGH){
		for(uint8_t i = 0; i<6; i++){
			read_soil_moisture_percent_average(i);
		}
	} else {
		Serial.println("Sensors are not powered, skipping moisture reading.");
	}
}

#endif