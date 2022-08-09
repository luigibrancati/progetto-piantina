#ifndef _WIFI_FUNCTIONS_H_
#define _WIFI_FUNCTIONS_H_

#include "memory.h"
#include "global.h"
#include "esp_log.h"
#include "WiFi.h"
#include "esp_wps.h"
#include "logging.h"

/*
Change the definition of the WPS mode
from WPS_TYPE_PBC to WPS_TYPE_PIN in
the case that you are using pin type
WPS
*/
#define ESP_WPS_MODE      WPS_TYPE_PBC
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"
static const int connectionTimeoutSeconds = 60;

static esp_wps_config_t config;

void waitConnection(){
	while(!WiFi.isConnected() && millis() < (connectionTimeoutSeconds * sToMs)) {
		Serial.print(".");
		delay(100);
	}
	Serial.println();
	if(WiFi.isConnected()){
		LogInfo("Connected: %s", WiFi.localIP().toString());
	} else {
		LogInfo("Couldn't connect to the wifi.");
	}
}

void wpsInitConfig(){
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}

void wpsStart(){
    if(esp_wifi_wps_enable(&config)){
    	LogInfo("WPS Enable Failed");
    } else if(esp_wifi_wps_start(0)){
    	LogInfo("WPS Start Failed");
    }
}

void wpsStop(){
    if(esp_wifi_wps_disable()){
    	LogInfo("WPS Disable Failed");
    }
}

void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info){
	switch(event){
		case ARDUINO_EVENT_WIFI_STA_START:
			LogInfo("Station Mode Started");
			break;
		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			LogInfo("Connected to: %s", WiFi.SSID());
			LogInfo("Got IP: %s", WiFi.localIP().toString());
			digitalWrite(LED_BUILTIN, HIGH);
			break;
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
			LogInfo("Disconnected from station, attempting reconnection");
			digitalWrite(LED_BUILTIN, LOW);
			WiFi.reconnect();
			break;
		case ARDUINO_EVENT_WPS_ER_SUCCESS:
			LogInfo("WPS Successfull, stopping WPS and connecting to: %s", WiFi.SSID());
			wpsStop();
			delay(10);
			WiFi.begin();
			break;
		case ARDUINO_EVENT_WPS_ER_FAILED:
			LogInfo("WPS Failed, retrying");
			wpsStop();
			wpsStart();
			break;
		case ARDUINO_EVENT_WPS_ER_TIMEOUT:
			LogInfo("WPS Timedout, retrying");
			wpsStop();
			wpsStart();
			break;
		default:
			break;
	}
}

void wifiInit(){
	WiFi.onEvent(WiFiEvent);
	WiFi.mode(WIFI_MODE_STA);
	wpsInitConfig();
}

void wifiWpsConnect(){
	wifiInit();
	LogInfo("Starting WPS");
	wpsStart();
	waitConnection();
}

// void wifiConnect(){
// 	wifiInit();
// 	LogInfo("Connecting to Wifi");
// 	WiFi.begin(SSID, PASS);
// 	waitConnection();
// }

void wifiDisconnect(){
	LogInfo("Disconnecting from Wifi");
	WiFi.disconnect();
	while(WiFi.isConnected()){
		Serial.print(".");
		delay(100);
	}
	Serial.println();
}

#endif
