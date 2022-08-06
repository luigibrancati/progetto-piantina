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

static esp_wps_config_t wps_config;

void wpsInitConfig(){
	wps_config.wps_type = ESP_WPS_MODE;
	strcpy(wps_config.factory_info.manufacturer, ESP_MANUFACTURER);
	strcpy(wps_config.factory_info.model_number, ESP_MODEL_NUMBER);
	strcpy(wps_config.factory_info.model_name, ESP_MODEL_NAME);
	strcpy(wps_config.factory_info.device_name, ESP_DEVICE_NAME);
}

void wpsStart(){
    if(esp_wifi_wps_enable(&wps_config)){
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

String wpspin2string(uint8_t a[]){
	char wps_pin[9];
	for(int i=0;i<8;i++){
		wps_pin[i] = a[i];
	}
	wps_pin[8] = '\0';
	return (String)wps_pin;
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
			WiFi.reconnect();
			digitalWrite(LED_BUILTIN, LOW);
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
		case ARDUINO_EVENT_WPS_ER_PIN:
			Serial.println("WPS_PIN = " + wpspin2string(info.wps_er_pin.pin_code));
			break;
		default:
			break;
	}
}

void wifi_wps_connect(){
	WiFi.onEvent(WiFiEvent);
	WiFi.mode(WIFI_MODE_STA);
	Serial.println("Starting WPS");
	wpsInitConfig();
	wpsStart();
}

////

void wifi_connect(){
	WiFi.onEvent(WiFiEvent);
	WiFi.mode(WIFI_MODE_STA);
	WiFi.begin(SSID, PASS);
	Serial.println("Connecting to Wifi");
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

void wifi_disconnect(){
	Serial.println("Disconnecting from Wifi");
	WiFi.disconnect();
	while(WiFi.isConnected()){
		Serial.print(".");
		delay(100);
	}
	Serial.println();
}

#endif
