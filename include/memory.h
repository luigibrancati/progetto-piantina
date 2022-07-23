#ifndef _READ_MEMORY_H_
#define _READ_MEMORY_H_

#include <Preferences.h>
#include <mqtt_client.h>

Preferences preferences;
static constexpr int sToUs = 1000000;
static constexpr int connectionTimeoutSeconds = 10;
RTC_DATA_ATTR int boot = 0;
static constexpr short NumReadings = 10; // Read moisture 10 times
static constexpr int ReadingsInt = 1000; // Read every 30 seconds
static constexpr int sToMs = 1000; // Conversion from Seconds to mS
static constexpr long DefaultMS_minute = 60 * sToMs; //Milliseconds per minute
static constexpr long DefaultMS_hour = (long)DefaultMS_minute*60; //Milliseconds per hour
static constexpr String variablesNamespace = "variables";
static constexpr String timeVar = "time";

struct MQTTtopics {

	MQTTtopics():
		commandTopic("debug"),
		stateTopic("debug"),
		availabilityTopic("debug"),
		debugTopic("debug")
	{}

	String commandTopic;
	String stateTopic;
	String availabilityTopic;
	String debugTopic;
};

struct MemoryVarInt {
	String memoryKey;
	int value;

	MemoryVarInt(String memKey):
		memoryKey(memKey),
		value(0)
	{}

	int getFromMemory(){
		return preferences.getInt(this->memoryKey.c_str(), NAN);
	}

	void updateFromMemory(){
		logger.log("Getting "+memoryKey+" variable from memory");
		this->value = this->getFromMemory();
		logger.log(memoryKey+" value: "+String(this->value));
	}

	void saveToMemory(){
		logger.log("Saving "+memoryKey+" variable to memory");
		if(this->value != this->getFromMemory()) {
			preferences.putInt(this->memoryKey.c_str(), this->value);
			logger.log("Saved "+memoryKey+" value: "+String(this->value));
		} else {
			logger.log(memoryKey+" hasn't changed");
		}
	}

	void setValue(int val){
		this->value = val;
		this->saveToMemory();
	}
};

struct MemoryVarFloat {
	String memoryKey;
	float value;

	MemoryVarFloat(String memKey):
		memoryKey(memKey),
		value(0)
	{}

	float getFromMemory(){
		return preferences.getFloat(this->memoryKey.c_str(), NAN);
	}

	void updateFromMemory(){
		logger.log("Getting "+memoryKey+" variable from memory");
		this->value = this->getFromMemory();
		logger.log(memoryKey+" value: "+String(this->value));
	}

	void saveToMemory(){
		logger.log("Saving "+memoryKey+" variable to memory");
		if(this->value != this->getFromMemory()) {
			preferences.putFloat(this->memoryKey.c_str(), this->value);
			logger.log("Saved "+memoryKey+" value: "+String(this->value));
		} else {
			logger.log(memoryKey+" hasn't changed");
		}
	}

	void setValue(float val){
		this->value = val;
		this->saveToMemory();
	}
};

struct MemoryVarBool {
	String memoryKey;
	bool value;

	MemoryVarBool(String memKey):
		memoryKey(memKey),
		value(false)
	{}

	bool getFromMemory(){
		return preferences.getBool(this->memoryKey.c_str(), NAN);
	}

	void updateFromMemory(){
		logger.log("Getting "+memoryKey+" variable from memory");
		this->value = this->getFromMemory();
		logger.log(memoryKey+" value: "+String(this->value));
	}

	void saveToMemory(){
		logger.log("Saving "+memoryKey+" variable to memory");
		if(this->value != this->getFromMemory()) {
			preferences.putBool(this->memoryKey.c_str(), this->value);
			logger.log("Saved "+memoryKey+" value: "+String(this->value));
		} else {
			logger.log(memoryKey+" hasn't changed");
		}
	}

	void setValue(bool val){
		this->value = val;
		this->saveToMemory();
	}
};

struct Logger: MQTTtopics {
	bool serial;
	bool mqttConnected;
	esp_mqtt_client_handle_t client;
	
	Logger(bool serial):
		MQTTtopics()
	{
		this->serial = serial;
		this->mqttConnected = false;
		this->client = nullptr;
		stateTopic = "abegghome/logger/state";
	}

	void log(String msg, bool newline = true){
		if(this->serial){
			(newline?Serial.println(msg):Serial.print(msg));
		}
		else{
			if(this->mqttConnected){
				esp_mqtt_client_publish(this->client, this->stateTopic.c_str(), msg.c_str(), 0, 1, 1);
			} else {
				(newline?Serial.println(msg):Serial.print(msg));
			}
		}
	}
} logger(false);

struct AirValue: MQTTtopics {
	MemoryVarFloat memoryVar;

	AirValue():
		MQTTtopics(),
		memoryVar("AirValue")
	{
		stateTopic = "abegghome/air_value/state";
	}
} airValue;

struct WaterValue: MQTTtopics {
	MemoryVarFloat memoryVar;

	WaterValue():
		MQTTtopics(),
		memoryVar("WaterValue")
	{
		stateTopic = "abegghome/water_value/state";
	}
} waterValue;

void getGeneralVarsFromMemory(){
	logger.log("Getting all general variables from memory");
	preferences.begin(variablesNamespace.c_str(), false);
	airValue.memoryVar.updateFromMemory();
	waterValue.memoryVar.updateFromMemory();
	preferences.end();
}

#endif
