#ifndef _WIFI_MQTT_H_
#define _WIFI_MQTT_H_

#include "memory.h"
#include "pumps.h"
#include "sensors.h"
#include "global.h"
#include <string>
#include <WiFi.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include <ArduinoJson.h>
#include <Regexp.h>

static esp_err_t messageHandler(esp_mqtt_event_handle_t event){
	String event_topic = convert_to_string(event->topic, event->topic_len);
	String event_data = convert_to_string(event->data, event->data_len);
	MatchState ms;
	ms.Target(event->topic);
	char result = ms.Match("[%a/]*/(%a+)/(%d+)/[%a/]*");
	if (result == REGEXP_MATCHED) {
		Serial.println("Received command/value from "+event_topic);
		char buf[100];
		String match_string = ms.GetCapture(buf, 0);
		uint8_t index = atoi(ms.GetCapture(buf, 1));
		preferences.begin(variablesNamespace, false);
		if(match_string == PumpOverride::classId){
			if (event_data == "on") {
				Serial.println("Turning Pump Override "+String(index)+" on");
				pumpOverride[index].memoryVar.setValue(true);
				esp_mqtt_client_publish(client, pumpOverride[index].stateTopic.c_str(), "on", 2, 1, 1);
			}
			else if (event_data == "off") {
				Serial.println("Turning Pump Override "+String(index)+" off");
				pumpOverride[index].memoryVar.setValue(false);
				esp_mqtt_client_publish(client, pumpOverride[index].stateTopic.c_str(), "off", 3, 1, 1);
			}
			Serial.println("Pump Override "+String(index)+" value: "+String(pumpOverride[index].memoryVar.value));
		}
		else if(match_string == PumpSwitch::classId){
			if (event_data == "on") {
				Serial.println("Turning Pump Switch "+String(index)+" on");
				pumpSwitch[index].memoryVar.setValue(true);
				esp_mqtt_client_publish(client, pumpSwitch[index].stateTopic.c_str(), "on", 2, 1, 1);
			}
			else if (event_data == "off") {
				Serial.println("Turning Pump Switch "+String(index)+" off");
				pumpSwitch[index].memoryVar.setValue(false);
				esp_mqtt_client_publish(client, pumpSwitch[index].stateTopic.c_str(), "off", 3, 1, 1);
			}
			Serial.println("Pump Switch "+String(index)+" value: "+String(pumpSwitch[index].memoryVar.value));
		}
		else if(match_string == MoistureTresh::classId){
			moistureTresh[index].memoryVar.setValue(event_data.toFloat());
			Serial.println("Moisture Tresh "+String(index)+" value: "+String(moistureTresh[index].memoryVar.value));
		}
		else if(match_string == AirValue::classId){
			airValue.memoryVar.setValue(event_data.toFloat());
			Serial.println("Air Value value: "+String(airValue.memoryVar.value));
		}
		else if(match_string == WaterValue::classId){
			waterValue.memoryVar.setValue(event_data.toFloat());
			Serial.println("Water Value value: "+String(waterValue.memoryVar.value));
		}
		else if(match_string == SamplingTime::classId){
			samplingTime.memoryVar.setValue(event_data.toInt());
			Serial.println("Sampling Time value: "+String(samplingTime.memoryVar.value));
		}
		else if(match_string == PumpRuntime::classId){
			pumpRuntime[index].memoryVar.setValue(event_data.toInt());
			Serial.println("Pump Runtime "+String(index)+" value: "+String(pumpRuntime[index].memoryVar.value));
		}
		else if(match_string == WateringTime::classId){
			wateringTime[index].memoryVar.setValue(event_data.toInt());
			Serial.println("Watering Time "+String(index)+" value: "+String(wateringTime[index].memoryVar.value));
		}
		else{
			Serial.println("Received unknown topic "+event_topic);
		}
		preferences.end();
	}
}

static esp_err_t mqtt_event_callback_handler(esp_mqtt_event_handle_t event) {
	switch(event->event_id){
		case MQTT_EVENT_CONNECTED:
			Serial.println("MQTT connection established");
			Serial.println("Fetching variables from the broker.");
			mqttConnected = true;
			logger.mqttConnected = mqttConnected;
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
			Serial.println("Subscribed to all topics");
			break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI("TEST", "MQTT event: %d. MQTT_EVENT_DISCONNECTED", event->event_id);
			mqttConnected = false;
			logger.mqttConnected = mqttConnected;
			break;
		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_SUBSCRIBED", event->msg_id, event->event_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_UNSUBSCRIBED", event->msg_id, event->event_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI("TEST", "MQTT event: %d. MQTT_EVENT_PUBLISHED", event->event_id);
			break;
		case MQTT_EVENT_DATA:
			ESP_LOGI("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_DATA", event->msg_id, event->event_id);
			ESP_LOGI("TEST", "Topic length %d. Data length %d", event->topic_len, event->data_len);
			ESP_LOGI("TEST","Incoming data: %.*s %.*s\n", event->topic_len, event->topic, event->data_len, event->data);
			messageHandler(event);
			break;
	}
	return ESP_OK;
}

void client_setup(){
	mqtt_cfg.host = MQTTBrokerIP;
	mqtt_cfg.port = MQTTBrokerPort;
	mqtt_cfg.username = MQTTUser;
	mqtt_cfg.password = MQTTPass;
	mqtt_cfg.keepalive = 15;
	mqtt_cfg.transport = MQTT_TRANSPORT_OVER_SSL;
	mqtt_cfg.event_handle = mqtt_event_callback_handler;
	mqtt_cfg.lwt_topic = "debug";
	mqtt_cfg.lwt_msg = "0";
	mqtt_cfg.lwt_msg_len = 1;
	WiFi.mode(WIFI_MODE_STA);
}

void wifi_mqtt_connect(){
	client_setup();
	WiFi.begin(SSID, PASS);
	Serial.println("Connecting");
	while(!WiFi.isConnected() && millis() < (connectionTimeoutSeconds * sToMs)) {
		Serial.print(".");
		delay(100);
	}
	Serial.println();
	esp_err_t err = esp_tls_set_global_ca_store(DSTroot_CA, sizeof(DSTroot_CA));
	client = esp_mqtt_client_init(&mqtt_cfg);
	logger.client = client;
	if(WiFi.isConnected()){
		Serial.println("Connected: "+WiFi.localIP().toString());
		err = esp_mqtt_client_start(client);
		delay(connectionTimeoutSeconds * sToMs);
		if(!mqttConnected){
			// Stop the client, otherwise it'll attempt to connect again
			esp_mqtt_client_stop(client);
			logger.mqttConnected = mqttConnected;
			Serial.println("Couldn't connect to the MQTT broker.");
			getSensorVarsFromMemory();
			getPumpVarsFromMemory();
			getGeneralVarsFromMemory();
		}
	} else {
		mqttConnected = false;
		logger.mqttConnected = mqttConnected;
		Serial.println("Couldn't connect to the wifi.");
		getSensorVarsFromMemory();
		getPumpVarsFromMemory();
		getGeneralVarsFromMemory();
	}
}

void wifi_mqtt_disconnect(){
	Serial.println("Disconnecting");
	esp_mqtt_client_destroy(client);
	WiFi.disconnect();
	logger.mqttConnected = false;
	Serial.println("Disconnecting");
	while(WiFi.isConnected()){
		Serial.print(".");
		delay(100);
	}
	Serial.println();
}

#endif
