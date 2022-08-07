#ifndef _WIFI_FUNCTIONS_H_
#define _WIFI_FUNCTIONS_H_

#include "memory.h"
#include "global.h"
#include "esp_log.h"
#include "WiFi.h"
#include "esp_wps.h"

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
		Serial.println("Connected: "+WiFi.localIP().toString());
	} else {
		Serial.println("Couldn't connect to the wifi.");
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
    	Serial.println("WPS Enable Failed");
    } else if(esp_wifi_wps_start(0)){
    	Serial.println("WPS Start Failed");
    }
}

void wpsStop(){
    if(esp_wifi_wps_disable()){
    	Serial.println("WPS Disable Failed");
    }
}

void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info){
	switch(event){
		case ARDUINO_EVENT_WIFI_STA_START:
			Serial.println("Station Mode Started");
			break;
		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			Serial.println("Connected to :" + String(WiFi.SSID()));
			Serial.print("Got IP: ");
			Serial.println(WiFi.localIP());
			digitalWrite(LED_BUILTIN, HIGH);
			break;
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
			Serial.println("Disconnected from station, attempting reconnection");
			digitalWrite(LED_BUILTIN, LOW);
			WiFi.reconnect();
			break;
		case ARDUINO_EVENT_WPS_ER_SUCCESS:
			Serial.println("WPS Successfull, stopping WPS and connecting to: " + String(WiFi.SSID()));
			wpsStop();
			delay(10);
			WiFi.begin();
			break;
		case ARDUINO_EVENT_WPS_ER_FAILED:
			Serial.println("WPS Failed, retrying");
			wpsStop();
			wpsStart();
			break;
		case ARDUINO_EVENT_WPS_ER_TIMEOUT:
			Serial.println("WPS Timedout, retrying");
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
	Serial.println("Starting WPS");
	wpsStart();
	waitConnection();
}

// void wifiConnect(){
// 	wifiInit();
// 	Serial.println("Connecting to Wifi");
// 	WiFi.begin(SSID, PASS);
// 	waitConnection();
// }

void wifiDisconnect(){
	Serial.println("Disconnecting from Wifi");
	WiFi.disconnect();
	while(WiFi.isConnected()){
		Serial.print(".");
		delay(100);
	}
	Serial.println();
}

#endif
