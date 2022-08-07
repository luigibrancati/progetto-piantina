#ifndef _WIFI_MQTT_GLOBAL_H_
#define _WIFI_MQTT_GLOBAL_H_

#include <WiFi.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include <ArduinoJson.h>
#include <regex>

static const char SSID[] = "ssid";    // Network SSID (name)
static const char PASS[] = "pass";    // Network password (use for WPA, or use as key for WEP)
static const char MQTTBrokerIP[] = "192.168.1.84";
static const char MQTTUser[] = "luigi";
static const char MQTTPass[] = "pass";
static const short MQTTBrokerPort = 8883;
static const unsigned char DSTroot_CA[] PROGMEM = R"EOF(
COPY HERE THE CONTENT OF CA.CRT
)EOF";

StaticJsonDocument<250> sensorJson;
char buffer[250];
esp_mqtt_client_config_t mqtt_cfg = {};
esp_mqtt_client_handle_t client = nullptr;
static bool mqttConnected = false;
static const short numPlants = 6;
RTC_DATA_ATTR int boot = 0;

template<typename T>
void createJson(T value) {
	sensorJson.clear();
	sensorJson["value"] = value;
	serializeJson(sensorJson, buffer);
}

String convertToString(char* text, int len){
	String out = "";
	for(char* l=text; l!=(text+len);l++){
		out+=*l;
	}
	return out;
}

#endif
