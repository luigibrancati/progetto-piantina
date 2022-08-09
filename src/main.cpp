#include "wifi_functions.h"
#include "mqtt_functions.h"
#include "battery.h"
#include "memory.h"
#include "pumps.h"
#include "sensors.h"
#include <time.h>

void setup() {
	delay(500);
	setCpuFrequencyMhz(80);
	Serial.begin(115200);
	while(!Serial);
	for(uint8_t i=0;i<numPlants;i++){
		pinMode(pumpPins[i], OUTPUT);
		digitalWrite(pumpPins[i], LOW);
		pinMode(sensorPins[i], INPUT);
	}
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	pinMode(sensorsSwitch, OUTPUT);
	digitalWrite(sensorsSwitch, LOW);
	// Read battery voltage
	/*
		We didn't notice, but we connected the battery voltage to GPIO25 which is ADC2.
		ADC2 cannot be used when WIFI is connected, so I have to read battery before connecting to WIFI.
	*/ 
	float batteryVoltage = readBattery();
	LogInfo("Battery voltage: %f", batteryVoltage);
	// Connect to Wifi and MQTT broker
	wifiWpsConnect();
	mqttConnect();
	if(!WiFi.isConnected() | !mqttConnected){
		// Stop the client, otherwise it'll attempt to connect again
		getSensorVarsFromMemory();
		getPumpVarsFromMemory();
		getGeneralVarsFromMemory();
	}
	// Read all sensors at once
	readAllSensors();
	// Start memory
	preferences.begin(variablesNamespace, false);
	// Get the last time the pump was run
	// and compute the seconds since it happenend
	time_t diffTime[numPlants] {0};
	for(uint8_t i=0;i<numPlants;i++){
		diffTime[i] = difftime(time(NULL), pumpState[i].lastRunMemoryVar.value);
		Serial.println("Seconds since last run: "+String(diffTime[i]));
		Serial.println("diffTime >= wateringTime: "+String(diffTime[i] >= (wateringTime[i].memoryVar.value * 3600)));
		Serial.println("soilMoisture < moistureTresh: "+String(soilMoisture[i].percVoltage.value < moistureTresh[i].memoryVar.value));
		if(pumpOverride[i].memoryVar.value || (pumpSwitch[i].memoryVar.value && (diffTime[i] >= (wateringTime[i].memoryVar.value * 3600)) && (soilMoisture[i].percVoltage.value < moistureTresh[i].memoryVar.value))){
			Serial.println("Running pump for "+String(pumpRuntime[i].memoryVar.value)+" seconds");
			digitalWrite(pumpPins[i], HIGH);
			if(mqttConnected){
				esp_mqtt_client_publish(client, pumpState[i].stateTopic.c_str(), "on", 2, 1, 0);
				delay(pumpRuntime[i].memoryVar.value * sToMs);
				digitalWrite(pumpPins[i], LOW);
				esp_mqtt_client_publish(client, pumpState[i].stateTopic.c_str(), "off", 3, 1, 0);
			} else {
				delay(pumpRuntime[i].memoryVar.value * sToMs);
				digitalWrite(pumpPins[i], LOW);
			}
			// Set the last time the pump was run
			pumpState[i].lastRunMemoryVar.setValue(time(NULL));
		}
		else{
			Serial.println("Condition to water plant not met, pump is off");
			if(mqttConnected){
				esp_mqtt_client_publish(client, pumpState[i].stateTopic.c_str(), "off", 3, 1, 0);
			}
		}
	}
	preferences.end();
	delay(1000);
	if(mqttConnected){
		// Send an off state to mean the pump/board is going to sleep and disconnect
		for(uint8_t i=0;i<numPlants;i++){
			esp_mqtt_client_publish(client, pumpState[i].availabilityTopic.c_str(), "off", 3, 1, 0);
		}
		//Send battery voltage
		createJson<float>(batteryVoltage);
		esp_mqtt_client_publish(client, "battery/state", buffer, 0, 1, 1);
	}
	// Deep sleep
	// The following lines disable all RTC features except the timer
	esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
	// If battery voltage is too low, set deep sleep to be infinite (i.e. hibernate)
	if(batteryVoltage>CRITICALLY_LOW_BATTERY_VOLTAGE){
		Serial.println("Setting timer wakeup to "+String(samplingTime.memoryVar.value)+" seconds");
		esp_sleep_enable_timer_wakeup((unsigned long) samplingTime.memoryVar.value * sToUs);
		Serial.println("Going into hibernation for "+String(samplingTime.memoryVar.value)+" seconds");
	}
	else{
		Serial.println("Hibernating without timer due to low battery charge.");
	}
	// Disconnect
	if(mqttConnected){
		mqttDestroy();
	}
	if(WiFi.isConnected()){
		wifiDisconnect();
	}
	// Hibernate
	esp_deep_sleep_start();
}

void loop() {}
