#ifndef _READ_MEMORY_H_
#define _READ_MEMORY_H_

#include "global.h"
#include "logging.h"
#include <Preferences.h>
#include <mqtt_client.h>

#define sToMs 1000
#define sToUs 1000000
#define DefaultMS_minute 60*sToMs
#define DefaultMS_hour 60*DefaultMS_minute

Preferences preferences;
static const char* variablesNamespace = "variables";

struct MemoryVarInt {
	String memoryKey;
	int value;

	MemoryVarInt(String memKey):
		memoryKey(memKey),
		value(0)
	{}

	int getFromMemory(){
		return preferences.getInt(this->memoryKey.c_str(), 0);
	}

	void updateFromMemory(){
		LogInfo("Getting %s variable from memory", memoryKey);
		this->value = this->getFromMemory();
		LogInfo("%s value: %i", memoryKey, this->value);
	}

	void saveToMemory(){
		LogInfo("Saving %s variable to memory", memoryKey);
		if(this->value != this->getFromMemory()) {
			preferences.putInt(this->memoryKey.c_str(), this->value);
			LogInfo("Saved %s value: %i", memoryKey, this->value);
		} else {
			LogInfo("%s hasn't changed", memoryKey);
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
		return preferences.getFloat(this->memoryKey.c_str(), 0);
	}

	void updateFromMemory(){
		LogInfo("Getting %s variable from memory", memoryKey);
		this->value = this->getFromMemory();
		LogInfo("%s value: %f", memoryKey, this->value);
	}

	void saveToMemory(){
		LogInfo("Saving %s variable to memory", memoryKey);
		if(this->value != this->getFromMemory()) {
			preferences.putFloat(this->memoryKey.c_str(), this->value);
			LogInfo("Saved %s value: %f", memoryKey, this->value);
		} else {
			LogInfo("%s hasn't changed", memoryKey);
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
		return preferences.getBool(this->memoryKey.c_str(), 0);
	}

	void updateFromMemory(){
		LogInfo("Getting %s variable from memory", memoryKey);
		this->value = this->getFromMemory();
		LogInfo("%s value: %i", memoryKey, this->value);
	}

	void saveToMemory(){
		LogInfo("Saving %s variable to memory", memoryKey);
		if(this->value != this->getFromMemory()) {
			preferences.putBool(this->memoryKey.c_str(), this->value);
			LogInfo("Saved %s value: %i", memoryKey, this->value);
		} else {
			LogInfo("%s hasn't changed", memoryKey);
		}
	}

	void setValue(bool val){
		this->value = val;
		this->saveToMemory();
	}
};

struct MQTTtopics {
	String commandTopic;
	String stateTopic;
	String availabilityTopic;
	String debugTopic;

	MQTTtopics():
		commandTopic("debug"),
		stateTopic("debug"),
		availabilityTopic("debug"),
		debugTopic("debug")
	{}
};

struct MQTTLogger: MQTTtopics {
	static bool serial;
	static String classId;
	static esp_mqtt_client_handle_t client;
	
	MQTTLogger():
		MQTTtopics()
	{
		stateTopic = "abegghome/"+this->classId+"/state";
	}

	void log(String msg, bool newline = true){
		if(this->serial){
			(newline?Serial.println(msg):Serial.print(msg));
		}
		else{
			if(mqttConnected){
				esp_mqtt_client_publish(this->client, this->stateTopic.c_str(), msg.c_str(), 0, 1, 1);
			} else {
				(newline?Serial.println(msg):Serial.print(msg));
			}
		}
	}
} mqtt_logger;
String MQTTLogger::classId = "logger";
bool MQTTLogger::serial = false;
esp_mqtt_client_handle_t MQTTLogger::client = nullptr;

struct AirValue: MQTTtopics {
	static String classId;
	MemoryVarFloat memoryVar;

	AirValue():
		MQTTtopics(),
		memoryVar("airValue")
	{
		stateTopic = "abegghome/"+this->classId+"/state";
	}
} airValue;
String AirValue::classId = "airValue";

struct WaterValue: MQTTtopics {
	static String classId;
	MemoryVarFloat memoryVar;

	WaterValue():
		MQTTtopics(),
		memoryVar("waterValue")
	{
		stateTopic = "abegghome/"+this->classId+"/state";
	}
} waterValue;
String WaterValue::classId = "waterValue";

void getGeneralVarsFromMemory(){
	LogInfo("Getting all general variables from memory");
	preferences.begin(variablesNamespace, false);
	airValue.memoryVar.updateFromMemory();
	waterValue.memoryVar.updateFromMemory();
	preferences.end();
}

#endif
