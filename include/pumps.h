#ifndef _READ_PUMPS_H_
#define _READ_PUMPS_H_

#include "global.h"
#include "logging.h"
#include "memory.h"

struct WateringTime: MQTTtopics {
	static String classId;
	MemoryVarInt memoryVar;

	WateringTime(uint8_t i):
		MQTTtopics(),
		memoryVar("wateringtime"+String(i))
	{
		stateTopic = "abegghome/"+this->classId+"/"+String(i)+"/state";
	}
};
String WateringTime::classId = "wateringtime";

struct MoistureTresh: MQTTtopics {
	static String classId;
	MemoryVarFloat memoryVar;

	MoistureTresh(uint8_t i):
		MQTTtopics(),
		memoryVar("moisturetresh"+String(i))
	{
		stateTopic = "abegghome/"+this->classId+"/"+String(i)+"/state";
	}
};
String MoistureTresh::classId = "moisturetresh";


struct PumpOverride: MQTTtopics {
	static String classId;
	MemoryVarBool memoryVar;

	PumpOverride(uint8_t i):
		MQTTtopics(),
		memoryVar("pumpoverride"+String(i))
	{
		commandTopic = "abegghome/"+this->classId+"/"+String(i)+"/com";
		stateTopic = "abegghome/"+this->classId+"/"+String(i)+"/state";
	}
};
String PumpOverride::classId = "pumpoverride";

struct PumpSwitch: MQTTtopics {
	static String classId;
	MemoryVarBool memoryVar;

	PumpSwitch(uint8_t i):
		MQTTtopics(),
		memoryVar("pumpswitch"+String(i))
	{
		commandTopic = "abegghome/"+this->classId+"/"+String(i)+"/com";
		stateTopic = "abegghome/"+this->classId+"/"+String(i)+"/state";
	}
};
String PumpSwitch::classId = "pumpswitch";

struct PumpState: MQTTtopics {
	static String classId;
	MemoryVarInt lastRunMemoryVar;

	PumpState(uint8_t i):
		MQTTtopics(),
		lastRunMemoryVar("lastrunmemoryvar"+String(i))
	{
		stateTopic = "abegghome/"+this->classId+"/"+String(i)+"/state";
		availabilityTopic = "abegghome/"+this->classId+"/"+String(i)+"/availability";
	}
};
String PumpState::classId = "pumpstate";

struct PumpRuntime: MQTTtopics {
	static String classId;
	MemoryVarInt memoryVar;

	PumpRuntime(uint8_t i):
		MQTTtopics(),
		memoryVar("pumpruntime"+String(i))
	{
		stateTopic = "abegghome/"+this->classId+"/"+String(i)+"/state";
	}
};
String PumpRuntime::classId = "pumpruntime";

static const gpio_num_t pumpPins[numPlants] {GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_17, GPIO_NUM_16, GPIO_NUM_4};
static WateringTime wateringTime[numPlants] {WateringTime(0), WateringTime(1), WateringTime(2), WateringTime(3), WateringTime(4), WateringTime(5)};
static MoistureTresh moistureTresh[numPlants] {MoistureTresh(0), MoistureTresh(1), MoistureTresh(2), MoistureTresh(3), MoistureTresh(4), MoistureTresh(5)};
static PumpOverride pumpOverride[numPlants] {PumpOverride(0), PumpOverride(1), PumpOverride(2), PumpOverride(3), PumpOverride(4), PumpOverride(5)};
static PumpSwitch pumpSwitch[numPlants] {PumpSwitch(0), PumpSwitch(1), PumpSwitch(2), PumpSwitch(3), PumpSwitch(4), PumpSwitch(5)};
static PumpState pumpState[numPlants] {PumpState(0), PumpState(1), PumpState(2), PumpState(3), PumpState(4), PumpState(5)};
static PumpRuntime pumpRuntime[numPlants] {PumpRuntime(0), PumpRuntime(1), PumpRuntime(2), PumpRuntime(3), PumpRuntime(4), PumpRuntime(5)};

void getPumpVarsFromMemory(){
	LogInfo("Getting all pump variables from memory");
	preferences.begin(variablesNamespace, false);
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