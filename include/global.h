#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <ArduinoJson.h>
#include <mqtt_client.h>

#define DPS_ID_SCOPE "0ne006EFCD7"
#define IOT_CONFIG_DEVICE_ID "1t0g4iy75p3"
#define IOT_CONFIG_DEVICE_KEY "Mdnv+ekK2Z82JMY1LA5iSHAvrguhtyu241eG9oW4hfU="
// User-agent (url-encoded) provided by the MQTT client to Azure IoT Services.
// When developing for your own Arduino-based platform,
// please update the suffix with the format '(ard;<platform>)' as an url-encoded string.
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard%3Besp32)"
// For how long the MQTT password (SAS token) is valid, in minutes.
// After that, the sample automatically generates a new password and re-connects.
#define MQTT_PASSWORD_LIFETIME_IN_MINUTES 60

StaticJsonDocument<250> sensorJson;
char buffer[250];
static esp_mqtt_client_handle_t mqtt_client = nullptr; //MQTT client
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
