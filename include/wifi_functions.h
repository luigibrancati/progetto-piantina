#ifndef _WIFI_FUNCTIONS_H_
#define _WIFI_FUNCTIONS_H_

#include "memory.h"
#include "global.h"
#include <WiFi.h>
#include "esp_log.h"

void wifi_connect(){
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
