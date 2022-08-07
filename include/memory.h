#ifndef _READ_MEMORY_H_
#define _READ_MEMORY_H_

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
		Serial.println("Getting "+memoryKey+" variable from memory");
		this->value = this->getFromMemory();
		Serial.println(memoryKey+" value: "+String(this->value));
	}

	void saveToMemory(){
		Serial.println("Saving "+memoryKey+" variable to memory");
		if(this->value != this->getFromMemory()) {
			preferences.putInt(this->memoryKey.c_str(), this->value);
			Serial.println("Saved "+memoryKey+" value: "+String(this->value));
		} else {
			Serial.println(memoryKey+" hasn't changed");
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
		Serial.println("Getting "+memoryKey+" variable from memory");
		this->value = this->getFromMemory();
		Serial.println(memoryKey+" value: "+String(this->value));
	}

	void saveToMemory(){
		Serial.println("Saving "+memoryKey+" variable to memory");
		if(this->value != this->getFromMemory()) {
			preferences.putFloat(this->memoryKey.c_str(), this->value);
			Serial.println("Saved "+memoryKey+" value: "+String(this->value));
		} else {
			Serial.println(memoryKey+" hasn't changed");
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
		Serial.println("Getting "+memoryKey+" variable from memory");
		this->value = this->getFromMemory();
		Serial.println(memoryKey+" value: "+String(this->value));
	}

	void saveToMemory(){
		Serial.println("Saving "+memoryKey+" variable to memory");
		if(this->value != this->getFromMemory()) {
			preferences.putBool(this->memoryKey.c_str(), this->value);
			Serial.println("Saved "+memoryKey+" value: "+String(this->value));
		} else {
			Serial.println(memoryKey+" hasn't changed");
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

struct Logger: MQTTtopics {
	static bool serial;
	static bool mqttConnected;
	static String classId;
	static esp_mqtt_client_handle_t client;
	
	Logger():
		MQTTtopics()
	{
		stateTopic = "abegghome/"+this->classId+"/state";
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
} logger;
String Logger::classId = "logger";
bool Logger::serial = false;
bool Logger::mqttConnected = false;
esp_mqtt_client_handle_t Logger::client = nullptr;

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
	Serial.println("Getting all general variables from memory");
	preferences.begin(variablesNamespace, false);
	airValue.memoryVar.updateFromMemory();
	waterValue.memoryVar.updateFromMemory();
	preferences.end();
}

#endif
