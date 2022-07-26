#ifndef _READ_SENSORS_H_
#define _READ_SENSORS_H_

#include "memory.h"
#include "global.h"

struct SamplingTime: MQTTtopics {
	static String classId;
	MemoryVarInt memoryVar;

	SamplingTime():
		MQTTtopics(),
		memoryVar("samplingTime")
	{
		stateTopic = "abegghome/"+this->classId+"/state";
	}
} samplingTime;
String SamplingTime::classId = "samplingTime";

struct SoilMoisture: MQTTtopics {
	static String classId;
	MemoryVarFloat rawVoltage;
	MemoryVarFloat percVoltage;

	SoilMoisture(uint8_t i):
		MQTTtopics(),
		rawVoltage("SoilMoistureRaw"+String(i)),
		percVoltage("SoilMoisturePerc"+String(i))
	{
		stateTopic = "abegghome/"+this->classId+"/"+String(i)+"/state";
		debugTopic = "abegghome/"+this->classId+"/"+String(i)+"/debug";
	}
};
String SoilMoisture::classId = "soilMoisture";

static const gpio_num_t sensorPins[numPlants] {GPIO_NUM_36, GPIO_NUM_39, GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_32, GPIO_NUM_33};
/*
	Note about sensorsSwitch: I'm using the Firebeetle 2 board for its low power consumption, but it has a small problem.
	Differently from all other ESP32 boards I've tried so far, when this board is put into hibernation mode it keeps sending power to the external components through its 3.3/VCC pin.
	This increases by a lot the overall system power consumption: the moisture sensor consumes a constant current of 10 mA which, compared to the uA consumption expected of the board, is a lot.
	To avoid this problem, I had to add a second relay which acts as a general switch and is switched off when the board goes into hibernation.
	This way, only the board and this single relay are powered during hibernation, keeping the current consumption in the uA range.
*/
static const gpio_num_t sensorsSwitch = GPIO_NUM_2;
static SoilMoisture soilMoisture[numPlants] {SoilMoisture(0), SoilMoisture(1), SoilMoisture(2), SoilMoisture(3), SoilMoisture(4), SoilMoisture(5)};

void getSensorVarsFromMemory(){
	Serial.println("Getting all sensor variables from memory");
	preferences.begin(variablesNamespace, false);
	samplingTime.memoryVar.updateFromMemory();
	for(uint8_t i=0; i<numPlants; i++){
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
	Serial.println("Analog read (S"+String(i)+"): "+String(reading));
	return reading;
}

float soil_moisture_percent(float soilMoistureAverage){
	float soilMoisturePercent = map_range(soilMoistureAverage, airValue.memoryVar.value, waterValue.memoryVar.value, 0.0, 100.0);
	return constrain(soilMoisturePercent, 0.0, 100.0); //Returns the percentage of moisture
}

void read_soil_moisture_percent_average(uint8_t i){
	float soilMoistureAverage = 0;
	// Read the moisture NumberReadings times, separated by ReadingsIntervalMS milliseconds
	for(int j=0;j<NumReadings;j++){
		soilMoistureAverage += ((float)read_soil_moisture(i)/NumReadings); //Average of all the readings
		delay(ReadingsInt); //Pause between two readings
	}
	soilMoisture[i].rawVoltage.setValue(soilMoistureAverage);
	soilMoisture[i].percVoltage.setValue(soil_moisture_percent(soilMoistureAverage));
}

void read_all_sensors(){
	Serial.println("Reading soil moisture");
	digitalWrite(sensorsSwitch, HIGH);
	for(uint8_t i = 0; i<numPlants; i++){
		read_soil_moisture_percent_average(i);
		// publish readings to mqtt broker
		createJson<float>(soilMoisture[i].percVoltage.value);
		esp_mqtt_client_publish(client, soilMoisture[i].stateTopic.c_str(), buffer, 0, 1, 1);
	}
	digitalWrite(sensorsSwitch, LOW);
}

#endif