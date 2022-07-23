#ifndef _READ_PUMPS_H_
#define _READ_PUMPS_H_

#include "wifi_mqtt.h"
#include "memory.h"

struct WateringTime: MQTTtopics {
	MemoryVarInt memoryVar;

	WateringTime(uint8_t i):
		MQTTtopics(),
		memoryVar("WateringTime"+String(i))
	{
		stateTopic = "abegghome/watering_time/"+String(i)+"/state";
	}
};

struct MoistureTresh: MQTTtopics {
	MemoryVarFloat memoryVar;

	MoistureTresh(uint8_t i):
		MQTTtopics(),
		memoryVar("MoistureTresh"+String(i))
	{
		stateTopic = "abegghome/moisture_tresh/"+String(i)+"/state";
	}
};

struct PumpOverride: MQTTtopics {
	MemoryVarBool memoryVar;

	PumpOverride(uint8_t i):
		MQTTtopics(),
		memoryVar("PumpOverride"+String(i))
	{
		commandTopic = "abegghome/pump_override/"+String(i)+"/com";
		stateTopic = "abegghome/pump_override/"+String(i)+"/state";
	}
};

struct PumpSwitch: MQTTtopics {
	MemoryVarBool memoryVar;

	PumpSwitch(uint8_t i):
		MQTTtopics(),
		memoryVar("PumpSwitch"+String(i))
	{
		commandTopic = "abegghome/pump_switch/"+String(i)+"/com";
		stateTopic = "abegghome/pump_switch/"+String(i)+"/state";
	}
};

struct PumpState: MQTTtopics {
	MemoryVarInt lastRunMemoryVar;

	PumpState(uint8_t i):
		MQTTtopics(),
		lastRunMemoryVar("lastRunMemoryVar"+String(i))
	{
		stateTopic = "abegghome/pump/"+String(i)+"/state";
		availabilityTopic = "abegghome/pump/"+String(i)+"/availability";
	}

	int getFromMemory() override {
		return preferences.getInt(this->memoryKey.c_str(), 0);
	}
};

struct PumpRuntime: MQTTtopics {
	MemoryVarInt memoryVar;

	PumpRuntime(uint8_t i):
		MQTTtopics(),
		memoryVar("PumpRuntime"+String(i))
	{
		stateTopic = "abegghome/pump_runtime/"+String(i)+"/state";
	}
};

static constexpr gpio_num_t pumpPins[numPlants] {GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_17, GPIO_NUM_16, GPIO_NUM_4};
static WateringTime wateringTime[numPlants] {WateringTime(0), WateringTime(1), WateringTime(2), WateringTime(3), WateringTime(4), WateringTime(5)};
static MoistureTresh moistureTresh[numPlants] {MoistureTresh(0), MoistureTresh(1), MoistureTresh(2), MoistureTresh(3), MoistureTresh(4), MoistureTresh(5)};
static PumpOverride pumpOverride[numPlants] {PumpOverride(0), PumpOverride(1), PumpOverride(2), PumpOverride(3), PumpOverride(4), PumpOverride(5)};
static PumpSwitch pumpSwitch[numPlants] {PumpSwitch(0), PumpSwitch(1), PumpSwitch(2), PumpSwitch(3), PumpSwitch(4), PumpSwitch(5)};
static PumpState pumpState[numPlants] {PumpState(0), PumpState(1), PumpState(2), PumpState(3), PumpState(4), PumpState(5)};
static PumpRuntime pumpRuntime[numPlants] {PumpRuntime(0), PumpRuntime(1), PumpRuntime(2), PumpRuntime(3), PumpRuntime(4), PumpRuntime(5)};

void getPumpVarsFromMemory(){
	Serial.println("Getting all pump variables from memory");
	preferences.begin(variablesNamespace.c_str(), false);
	for(uint8_t i = 0; i<numPlants; i++){
		wateringTime[i].memoryVar.updateFromMemory();
		moistureTresh[i].memoryVar.updateFromMemory();
		pumpOverride[i].memoryVar.updateFromMemory();
		pumpSwitch[i].memoryVar.updateFromMemory();
		pumpState[i].lastRunMemoryVar.updateFromMemory();
		pumpRuntime[i].memoryVar.updateFromMemory();
	}
	preferences.end();
}

#endif