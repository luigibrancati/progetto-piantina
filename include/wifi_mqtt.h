#ifndef _WIFI_SERVER_H_
#define _WIFI_SERVER_H_

#include "memory.h"
#include <string>
#include <WiFi.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include <ArduinoJson.h>
#include <regex.h>

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

std::string convert_to_string(char* text, int len){
	std::string out = "";
	for(char* l=text; l!=(text+len);l++){
		out+=*l;
	}
	return out;
}

static esp_err_t messageHandler(esp_mqtt_event_handle_t event){
	std::string event_topic = convert_to_string(event->topic, event->topic_len);
	std::string event_data = convert_to_string(event->data, event->data_len);
	std::regex topic_regex("[\\w/]*/(\\w+)/(\\d{1,1})/[\\w/]*", std::regex::ECMAScript | std::regex::icase);
	std::smatch matches;
	if (std::regex_search(event_topic, matches, topic_regex)) {
		Serial.println("Received command/value from "+event_topic);
		preferences.begin(variablesNamespace, false);
		uint8_t index = matches[2];
		if(matches[1]==pumpOverride[0].classId){
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
		else if(matches[1]==pumpSwitch[0].classId){
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
		else if(matches[1]==moistureTresh[0].classId){
			moistureTresh[index].memoryVar.setValue(event_data.toFloat());
			Serial.println("Moisture Tresh "+String(index)+" value: "+String(moistureTresh[index].memoryVar.value));
		}
		else if(matches[1]==airValue.classId){
			airValue.memoryVar.setValue(event_data.toFloat());
			Serial.println("Air Value value: "+String(airValue.memoryVar.value));
		}
		else if(matches[1]==waterValue.classId){
			waterValue.memoryVar.setValue(event_data.toFloat());
			Serial.println("Water Value value: "+String(waterValue.memoryVar.value));
		}
		else if(matches[1]==samplingTime.classId){
			samplingTime.memoryVar.setValue(event_data.toInt());
			Serial.println("Sampling Time value: "+String(samplingTime.memoryVar.value));
		}
		else if(matches[1]==pumpRuntime[0].classId){
			pumpRuntime[index].memoryVar.setValue(event_data.toInt());
			Serial.println("Pump Runtime "+String(index)+" value: "+String(pumpRuntime[index].memoryVar.value));
		}
		else if(matches[1]==wateringTime[0].classId){
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
	Serial.println("Connecting", false);
	while(!WiFi.isConnected() && millis() < (connectionTimeoutSeconds * sToMs)) {
		Serial.println(".", false);
		delay(100);
	}
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
			getAllFromMemory();
		}
	} else {
		mqttConnected = false;
		logger.mqttConnected = mqttConnected;
		Serial.println("Couldn't connect to the wifi.");
		getAllFromMemory();
	}
}

void wifi_mqtt_disconnect(){
	Serial.println("Disconnecting");
	esp_mqtt_client_destroy(client);
	WiFi.disconnect();
	logger.mqttConnected = false;
	Serial.println("Disconnecting", false);
	while(WiFi.isConnected()){
		Serial.println(".", false);
		delay(100);
	}
}

#endif
