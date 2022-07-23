#include "wifi_server.h"
#include "battery.h"
#include "memory.h"
#include <time.h>

void setup() {
	delay(500);
	setCpuFrequencyMhz(80);
	pinMode(PUMP_PIN, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	/*
		Note about FULL_SWITCH: I'm using the Firebeetle 2 board for its low power consumption, but it has a small problem.
		Differently from all other ESP32 boards I've tried so far, when this board is put into hibernation mode it keeps sending power to the external components through its 3.3/VCC pin.
		This increases by a lot the overall system power consumption: the moisture sensor consumes a constant current of 10 mA which, compared to the uA consumption expected of the board, is a lot.
		To avoid this problem, I had to add a second relay which acts as a general switch and is switched off when the board goes into hibernation.
		This way, only the board and this single relay are powered during hibernation, keeping the current consumption in the uA range.
	*/
	pinMode(FULL_SWITCH, OUTPUT);
	pinMode(MOISTURE_READING_PIN, INPUT);
	digitalWrite(PUMP_PIN, LOW);
	digitalWrite(LED_BUILTIN, HIGH);
	digitalWrite(FULL_SWITCH, HIGH);
	Serial.begin(115200);
	while(!Serial);
	wifi_mqtt_connect();
	// Start code to wet the plant
	logger.log("Reading soil moisture");
	read_soil_moisture_percent_average();
	createJson<float>(soilMoisture.value);
	// Start memory
	preferences.begin(variablesNamespace, false);
	// Get the last time the pump was run
	// and compute the seconds since it happenend
	time_t diffTime = difftime(time(NULL), preferences.getInt(timeVar));
	logger.log("Seconds since last run: "+String(diffTime));
	preferences.end();
	logger.log("diffTime >= wateringTime: "+String(diffTime >= (wateringTime.value * 3600)));
	logger.log("soilMoisture < moistureTresh: "+String(soilMoisture.value < moistureTresh.value));
	if(pumpOverride.value || (pumpSwitch.value && (diffTime >= (wateringTime.value * 3600)) && (soilMoisture.value < moistureTresh.value))){
		logger.log("Running pump for "+String(pumpRuntime.value)+" seconds");
		digitalWrite(PUMP_PIN, HIGH);
		if(mqttConnected){
			esp_mqtt_client_publish(client, pump.stateTopic.c_str(), "on", 2, 1, 0);
			delay(pumpRuntime.value * sToMs);
			digitalWrite(PUMP_PIN, LOW);
			esp_mqtt_client_publish(client, pump.stateTopic.c_str(), "off", 3, 1, 0);
		} else {
			delay(pumpRuntime.value * sToMs);
			digitalWrite(PUMP_PIN, LOW);
		}
		preferences.begin(variablesNamespace, false);
		// Set the last time the pump was run
		preferences.putInt(timeVar, time(NULL));
		preferences.end();
	}
	else{
		logger.log("Condition to water plant not met, pump is off");
		if(mqttConnected){
			esp_mqtt_client_publish(client, pump.stateTopic.c_str(), "off", 3, 1, 0);
		}
	}
	delay(1000);
	// Read battery voltage
	float batteryVoltage = read_battery();
	logger.log("Battery voltage: "+String(batteryVoltage));
	if(mqttConnected){
		// Send an off state to mean the pump/board is going to sleep and disconnect
		esp_mqtt_client_publish(client, soilMoisture.stateTopic.c_str(), buffer, 0, 1, 1);
		esp_mqtt_client_publish(client, pump.availabilityTopic.c_str(), "off", 3, 1, 0);
		esp_mqtt_client_publish(client, soilMoisture.availabilityTopic.c_str(), "off", 3, 1, 0);
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
	//If battery voltage is too low, set deep sleep to be infinite (i.e. hibernate)
	if(batteryVoltage>CRITICALLY_LOW_BATTERY_VOLTAGE){
		logger.log("Setting timer wakeup to "+String(samplingTime.value)+" seconds");
		esp_sleep_enable_timer_wakeup((unsigned long) samplingTime.value * sToUs);
		logger.log("Going into hibernation for "+String(samplingTime.value)+" seconds");
	}
	else{
		logger.log("Hibernating without timer due to low battery charge.");
	}
	//disconnect
	wifi_mqtt_disconnect();
	// hibernation
	esp_deep_sleep_start();
}

void loop() {}
