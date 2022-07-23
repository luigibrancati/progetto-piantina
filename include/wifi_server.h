#ifndef _WIFI_SERVER_H_
#define _WIFI_SERVER_H_

#include "memory.h"
#include <WiFi.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include <ArduinoJson.h>

static constexpr char SSID[] = "ssid";    // Network SSID (name)
static constexpr char PASS[] = "pass";    // Network password (use for WPA, or use as key for WEP)
static constexpr char MQTTBrokerIP[] = "192.168.1.84";
static constexpr char MQTTUser[] = "luigi";
static constexpr char MQTTPass[] = "pass";
static constexpr short MQTTBrokerPort = 8883;
static constexpr unsigned char DSTroot_CA[] PROGMEM = R"EOF(
COPY HERE THE CONTENT OF CA.CRT
)EOF";

StaticJsonDocument<250> sensorJson;
char buffer[250];
esp_mqtt_client_config_t mqtt_cfg = {};
esp_mqtt_client_handle_t client = nullptr;
static bool mqttConnected = false;

template<typename T>
void createJson(T value) {
	sensorJson.clear();
	sensorJson["value"] = value;
	serializeJson(sensorJson, buffer);
}

String convert_to_string(char* text, int len){
	String out = "";
	for(char* l=text; l!=(text+len);l++){
		out+=*l;
	}
	return out;
}

static esp_err_t messageHandler(esp_mqtt_event_handle_t event){
	String event_topic = convert_to_string(event->topic, event->topic_len);
	String event_data = convert_to_string(event->data, event->data_len);
	preferences.begin(variablesNamespace, false);
	if(event_topic==pumpOverride.commandTopic){
		logger.log("Received command for Pump Override");
		if (event_data == "on") {
			logger.log("Turning Pump Override on");
			pumpOverride.value = true;
			esp_mqtt_client_publish(client, pumpOverride.stateTopic.c_str(), "on", 2, 1, 1);
		}
		else if (event_data == "off") {
			logger.log("Turning Pump Override off");
			pumpOverride.value = false;
			esp_mqtt_client_publish(client, pumpOverride.stateTopic.c_str(), "off", 3, 1, 1);
		}
		logger.log("pumpOverride value: "+String(pumpOverride.value));
		updateVar(pumpOverride.value, pumpOverride.key);
	}
	else if(event_topic==pumpSwitch.commandTopic){
		logger.log("Received command for Pump Switch");
		if (event_data == "on") {
			logger.log("Turning Pump Switch on");
			pumpSwitch.value = true;
			esp_mqtt_client_publish(client, pumpSwitch.stateTopic.c_str(), "on", 2, 1, 1);
		}
		else if (event_data == "off") {
			logger.log("Turning Pump Switch off");
			pumpSwitch.value = false;
			esp_mqtt_client_publish(client, pumpSwitch.stateTopic.c_str(), "off", 3, 1, 1);
		}
		logger.log("pumpSwitch value: "+String(pumpSwitch.value));
		updateVar(pumpSwitch.value, pumpSwitch.key);
	}
	else if(event_topic==moistureTresh.stateTopic){
		logger.log("Received moisture_tresh");
		moistureTresh.value = event_data.toFloat();
		logger.log("moisture_tresh value: "+String(moistureTresh.value));
		updateVar(moistureTresh.value, moistureTresh.key);
	}
	else if(event_topic==airValue.stateTopic){
		logger.log("Received air_value");
		airValue.value = event_data.toFloat();
		logger.log("air_value value: "+String(airValue.value));
		updateVar(airValue.value, airValue.key);
	}
	else if(event_topic==waterValue.stateTopic){
		logger.log("Received water_value");
		waterValue.value = event_data.toFloat();
		logger.log("water_value value: "+String(waterValue.value));
		updateVar(waterValue.value, waterValue.key);
	}
	else if(event_topic==samplingTime.stateTopic){
		logger.log("Received sampling_time");
		samplingTime.value = event_data.toInt();
		logger.log("sampling_time value: "+String(samplingTime.value));
		updateVar(samplingTime.value, samplingTime.key);
	}
	else if(event_topic==pumpRuntime.stateTopic){
		logger.log("Received pump_runtime");
		pumpRuntime.value = event_data.toInt();
		logger.log("pump_runtime value: "+String(pumpRuntime.value));
		updateVar(pumpRuntime.value, pumpRuntime.key);
	}
	else if(event_topic==wateringTime.stateTopic){
	    logger.log("Received watering_time");
	    wateringTime.value = event_data.toInt();
	    logger.log("watering_time value: "+String(wateringTime.value));
		updateVar(wateringTime.value, wateringTime.key);
	}
	else{
		logger.log("Received unknown topic "+event_topic);
	}
	preferences.end();
}

static esp_err_t mqtt_event_callback_handler(esp_mqtt_event_handle_t event) {
	switch(event->event_id){
		case MQTT_EVENT_CONNECTED:
			logger.log("MQTT connection established");
			logger.log("Fetching variables from the broker.");
			mqttConnected = true;
			logger.mqttConnected = mqttConnected;
			// Send an on state to mean the pump/board has started and is connected
			esp_mqtt_client_publish(client, pump.availabilityTopic.c_str(), "on", 2, 1, 0);
			esp_mqtt_client_publish(client, soilMoisture.availabilityTopic.c_str(), "on", 2, 1, 0);
			esp_mqtt_client_publish(client, pump.stateTopic.c_str(), "off", 3, 1, 1);
			// Subscriptions
			esp_mqtt_client_subscribe(client, pumpOverride.commandTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, pumpSwitch.commandTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, moistureTresh.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, airValue.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, waterValue.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, samplingTime.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, pumpRuntime.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(client, wateringTime.stateTopic.c_str(), 1);
			logger.log("Subscribed to all topics");
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
	logger.log("Connecting", false);
	while(!WiFi.isConnected() && millis() < (connectionTimeoutSeconds * sToMs)) {
		logger.log(".", false);
		delay(100);
	}
	esp_err_t err = esp_tls_set_global_ca_store(DSTroot_CA, sizeof(DSTroot_CA));
	client = esp_mqtt_client_init(&mqtt_cfg);
	logger.client = client;
	if(WiFi.isConnected()){
		logger.log("Connected: "+WiFi.localIP().toString());
		err = esp_mqtt_client_start(client);
		delay(connectionTimeoutSeconds * sToMs);
		if(!mqttConnected){
			// Stop the client, otherwise it'll attempt to connect again
			esp_mqtt_client_stop(client);
			logger.mqttConnected = mqttConnected;
			logger.log("Couldn't connect to the MQTT broker.");
			getAllFromMemory();
		}
	} else {
		mqttConnected = false;
		logger.mqttConnected = mqttConnected;
		logger.log("Couldn't connect to the wifi.");
		getAllFromMemory();
	}
}

void wifi_mqtt_disconnect(){
	logger.log("Disconnecting");
	esp_mqtt_client_destroy(client);
	WiFi.disconnect();
	logger.mqttConnected = false;
	logger.log("Disconnecting", false);
	while(WiFi.isConnected()){
		logger.log(".", false);
		delay(100);
	}
}

#endif
