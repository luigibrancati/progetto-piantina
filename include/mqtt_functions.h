#ifndef _MQTT_FUNCTIONS_H_
#define _MQTT_FUNCTIONS_H_

#include "memory.h"
#include "pumps.h"
#include "sensors.h"
#include "global.h"
#include <WiFi.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include <ArduinoJson.h>
#include <Regexp.h>
#include "logging.h"

static esp_err_t messageHandler(esp_mqtt_event_handle_t event){
	String event_topic = convertToString(event->topic, event->topic_len);
	String event_data = convertToString(event->data, event->data_len);
	MatchState ms;
	ms.Target(event->topic);
	char result = ms.Match("[%a/]*/(%a+)/(%d+)/[%a/]*");
	if (result == REGEXP_MATCHED) {
		LogInfo("Received command/value from %s", event_topic);
		char buf[100];
		String match_string = ms.GetCapture(buf, 0);
		uint8_t index = atoi(ms.GetCapture(buf, 1));
		preferences.begin(variablesNamespace, false);
		if(match_string == PumpOverride::classId){
			if (event_data == "on") {
				LogInfo("Turning Pump Override %i on", index);
				pumpOverride[index].memoryVar.setValue(true);
				esp_mqtt_client_publish(client, pumpOverride[index].stateTopic.c_str(), "on", 2, 1, 1);
			}
			else if (event_data == "off") {
				LogInfo("Turning Pump Override %i off", index);
				pumpOverride[index].memoryVar.setValue(false);
				esp_mqtt_client_publish(client, pumpOverride[index].stateTopic.c_str(), "off", 3, 1, 1);
			}
			LogInfo("Pump Override %i value: %i", index, pumpOverride[index].memoryVar.value);
		}
		else if(match_string == PumpSwitch::classId){
			if (event_data == "on") {
				LogInfo("Turning Pump Switch %i on", index);
				pumpSwitch[index].memoryVar.setValue(true);
				esp_mqtt_client_publish(client, pumpSwitch[index].stateTopic.c_str(), "on", 2, 1, 1);
			}
			else if (event_data == "off") {
				LogInfo("Turning Pump Switch %i off", index);
				pumpSwitch[index].memoryVar.setValue(false);
				esp_mqtt_client_publish(client, pumpSwitch[index].stateTopic.c_str(), "off", 3, 1, 1);
			}
			LogInfo("Pump Switch %i value: %i", index, pumpSwitch[index].memoryVar.value);
		}
		else if(match_string == MoistureTresh::classId){
			moistureTresh[index].memoryVar.setValue(event_data.toFloat());
			LogInfo("Moisture Tresh %i value: %f", index, moistureTresh[index].memoryVar.value);
		}
		else if(match_string == AirValue::classId){
			airValue.memoryVar.setValue(event_data.toFloat());
			LogInfo("Air Value value: %f", airValue.memoryVar.value);
		}
		else if(match_string == WaterValue::classId){
			waterValue.memoryVar.setValue(event_data.toFloat());
			LogInfo("Water Value value: %f", waterValue.memoryVar.value);
		}
		else if(match_string == SamplingTime::classId){
			samplingTime.memoryVar.setValue(event_data.toInt());
			LogInfo("Sampling Time value: %i", samplingTime.memoryVar.value);
		}
		else if(match_string == PumpRuntime::classId){
			pumpRuntime[index].memoryVar.setValue(event_data.toInt());
			LogInfo("Pump Runtime %i value: %i", index, pumpRuntime[index].memoryVar.value);
		}
		else if(match_string == WateringTime::classId){
			wateringTime[index].memoryVar.setValue(event_data.toInt());
			LogInfo("Watering Time %i value: %i", index, wateringTime[index].memoryVar.value);
		}
		else{
			LogInfo("Received unknown topic %s", event_topic);
		}
		preferences.end();
	}
	return ESP_OK;
}

static esp_err_t mqttEventCallbackHandler(esp_mqtt_event_handle_t event) {
	switch(event->event_id){
		case MQTT_EVENT_CONNECTED:
			LogInfo("MQTT connection established");
			LogInfo("Fetching variables from the broker.");
			mqttConnected = true;
			for(uint8_t i=0;i<numPlants;i++){
				// Send an on state to mean the pump/board has started and is connected
				esp_mqtt_client_publish(client, pumpState[i].availabilityTopic.c_str(), "on", 2, 1, 0);
				esp_mqtt_client_publish(client, pumpState[i].stateTopic.c_str(), "off", 3, 1, 1);
				// Subscriptions
				esp_mqtt_client_subscribe(client, wateringTime[i].stateTopic.c_str(), 1);
				esp_mqtt_client_subscribe(client, moistureTresh[i].stateTopic.c_str(), 1);
				esp_mqtt_client_subscribe(client, pumpOverride[i].commandTopic.c_str(), 1);
				esp_mqtt_client_subscribe(client, pumpSwitch[i].commandTopic.c_str(), 1);
				esp_mqtt_client_subscribe(client, pumpRuntime[i].stateTopic.c_str(), 1);
			}
			esp_mqtt_client_subscribe(client, airValue.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, waterValue.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, samplingTime.stateTopic.c_str(), 1);
			LogInfo("Subscribed to all topics");
			break;
		case MQTT_EVENT_DISCONNECTED:
			LogInfo("TEST", "MQTT event: %d. MQTT_EVENT_DISCONNECTED", event->event_id);
			mqttConnected = false;
			break;
		case MQTT_EVENT_SUBSCRIBED:
			LogInfo("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_SUBSCRIBED", event->msg_id, event->event_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			LogInfo("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_UNSUBSCRIBED", event->msg_id, event->event_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			LogInfo("TEST", "MQTT event: %d. MQTT_EVENT_PUBLISHED", event->event_id);
			break;
		case MQTT_EVENT_DATA:
			LogInfo("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_DATA", event->msg_id, event->event_id);
			LogInfo("TEST", "Topic length %d. Data length %d", event->topic_len, event->data_len);
			LogInfo("TEST","Incoming data: %.*s %.*s\n", event->topic_len, event->topic, event->data_len, event->data);
			messageHandler(event);
			break;
	}
	return ESP_OK;
}

void clientSetup(){
	mqtt_cfg.host = MQTTBrokerIP;
	mqtt_cfg.port = MQTTBrokerPort;
	mqtt_cfg.username = MQTTUser;
	mqtt_cfg.password = MQTTPass;
	mqtt_cfg.keepalive = 15;
	mqtt_cfg.transport = MQTT_TRANSPORT_OVER_SSL;
	mqtt_cfg.event_handle = mqttEventCallbackHandler;
	mqtt_cfg.lwt_topic = "debug";
	mqtt_cfg.lwt_msg = "0";
	mqtt_cfg.lwt_msg_len = 1;
}

void mqttDestroy(){
	LogInfo("Disconnecting MQTT");
	esp_mqtt_client_destroy(client);
	mqttConnected = false;
}

void mqttConnect(){
	clientSetup();
	esp_err_t err = esp_tls_set_global_ca_store(DSTroot_CA, sizeof(DSTroot_CA));
	client = esp_mqtt_client_init(&mqtt_cfg);
	mqtt_logger.client = client;
	if(WiFi.isConnected()){
		LogInfo("Connecting to MQTT");
		err = esp_mqtt_client_start(client);
		delay(connectionTimeoutSeconds * sToMs);
		if(!mqttConnected){
			// Stop the client, otherwise it'll attempt to connect again
			esp_mqtt_client_stop(client);
			LogInfo("Couldn't connect to the MQTT broker.");
		}
	} else {
		mqttDestroy();
		LogInfo("Couldn't connect to the MQTT broker because Wifi is off.");
	}
}



#endif
