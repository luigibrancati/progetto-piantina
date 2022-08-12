#include "global.h"
#include "logging.h"
#include <time.h>


// For hmac SHA256 encryption
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>

#include <azure_ca.h>
#include "Azure_IoT_PnP_Template.h"

#include "wifi_functions.h"
#include "mqtt_functions.h"
#include "battery.h"
#include "memory.h"
#include "pumps.h"
#include "sensors.h"

/* --- Sample-specific Settings --- */
#define SERIAL_LOGGER_BAUD_RATE 115200

/* --- Time and NTP Settings --- */
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF   1
#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

/* --- Function Declarations --- */
static void syncDeviceClockWithNtpServer();

/* --- Other Interface functions required by Azure IoT --- */
/*
 * See the documentation of `hmac_sha256_encryption_function_t` in AzureIoT.h for details.
 */
static int mbedtls_hmac_sha256(const uint8_t* key, size_t key_length, const uint8_t* payload, size_t payload_length, uint8_t* signed_payload, size_t signed_payload_size)
{
	(void)signed_payload_size;
	mbedtls_md_context_t ctx;
	mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
	mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key, key_length);
	mbedtls_md_hmac_update(&ctx, (const unsigned char*)payload, payload_length);
	mbedtls_md_hmac_finish(&ctx, (byte*)signed_payload);
	mbedtls_md_free(&ctx);
	return 0;
}

/*
 * See the documentation of `base64_decode_function_t` in AzureIoT.h for details.
 */
static int base64_decode(uint8_t* data, size_t data_length, uint8_t* decoded, size_t decoded_size, size_t* decoded_length)
{
  	return mbedtls_base64_decode(decoded, decoded_size, decoded_length, data, data_length);
}

/*
 * See the documentation of `base64_encode_function_t` in AzureIoT.h for details.
 */
static int base64_encode(uint8_t* data, size_t data_length, uint8_t* encoded, size_t encoded_size, size_t* encoded_length)
{
  	return mbedtls_base64_encode(encoded, encoded_size, encoded_length, data, data_length);
}

/*
 * See the documentation of `properties_update_completed_t` in AzureIoT.h for details.
 */
static void on_properties_update_completed(uint32_t request_id, az_iot_status status_code)
{
  	LogInfo("Properties update request completed (id=%d, status=%d)", request_id, status_code);
}

/*
 * See the documentation of `properties_received_t` in AzureIoT.h for details.
 */
void on_properties_received(az_span properties)
{
  	LogInfo("Properties update received: %.*s", az_span_size(properties), az_span_ptr(properties));
	// It is recommended not to perform work within callbacks.
	// The properties are being handled here to simplify the sample.
	if (azure_pnp_handle_properties_update(&azure_iot, properties, properties_request_id++) != 0)
	{
		LogError("Failed handling properties update.");
	}
}

/*
 * See the documentation of `command_request_received_t` in AzureIoT.h for details.
 */
static void on_command_request_received(command_request_t command)
{  
	az_span component_name = az_span_size(command.component_name) == 0 ? AZ_SPAN_FROM_STR("") : command.component_name;
	LogInfo(
		"Command request received (id=%.*s, component=%.*s, name=%.*s)", 
		az_span_size(command.request_id), az_span_ptr(command.request_id),
		az_span_size(component_name), az_span_ptr(component_name),
		az_span_size(command.command_name), az_span_ptr(command.command_name)
	);
	// Here the request is being processed within the callback that delivers the command request.
	// However, for production application the recommendation is to save `command` and process it outside
	// this callback, usually inside the main thread/task/loop.
	(void)azure_pnp_handle_command_request(&azure_iot, command);
}

void azureIotSetup(){
	azure_pnp_init();
	/* 	
	* The configuration structure used by Azure IoT must remain unchanged (including data buffer) 
	* throughout the lifetime of the sample. This variable must also not lose context so other
	* components do not overwrite any information within this structure.
	*/
	azure_iot_config.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
	azure_iot_config.model_id = azure_pnp_get_model_id();
	azure_iot_config.use_device_provisioning = true; // Required for Azure IoT Central.
	azure_iot_config.iot_hub_fqdn = AZ_SPAN_EMPTY;
	azure_iot_config.device_id = AZ_SPAN_EMPTY;
	azure_iot_config.device_certificate = AZ_SPAN_EMPTY;
	azure_iot_config.device_certificate_private_key = AZ_SPAN_EMPTY;
	azure_iot_config.device_key = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY);
	azure_iot_config.dps_id_scope = AZ_SPAN_FROM_STR(DPS_ID_SCOPE);
	azure_iot_config.dps_registration_id = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID); // Use Device ID for Azure IoT Central.
	azure_iot_config.data_buffer = AZ_SPAN_FROM_BUFFER(az_iot_data_buffer);
	azure_iot_config.sas_token_lifetime_in_minutes = MQTT_PASSWORD_LIFETIME_IN_MINUTES;
	azure_iot_config.mqtt_client_interface.mqtt_client_init = mqtt_client_init_function;
	azure_iot_config.mqtt_client_interface.mqtt_client_deinit = mqtt_client_deinit_function;
	azure_iot_config.mqtt_client_interface.mqtt_client_subscribe = mqtt_client_subscribe_function;
	azure_iot_config.mqtt_client_interface.mqtt_client_publish = mqtt_client_publish_function;
	azure_iot_config.data_manipulation_functions.hmac_sha256_encrypt = mbedtls_hmac_sha256;
	azure_iot_config.data_manipulation_functions.base64_decode = base64_decode;
	azure_iot_config.data_manipulation_functions.base64_encode = base64_encode;
	azure_iot_config.on_properties_update_completed = on_properties_update_completed;
	azure_iot_config.on_properties_received = on_properties_received;
	azure_iot_config.on_command_request_received = on_command_request_received;

	azure_iot_init(&azure_iot, &azure_iot_config);
	azure_iot_start(&azure_iot);
	LogInfo("Azure IoT client initialized (state=%d)", azure_iot.state);
	// Connect
  	azure_iot_status_t status = azure_iot_status_t::azure_iot_connecting;
	while(status!=azure_iot_status_t::azure_iot_connected){
		if (WiFi.status() != WL_CONNECTED)
		{
			LogInfo("Wifi not connected");
			wifiWpsConnect();
			azure_iot_start(&azure_iot);
		}
		else
		{
			status = azure_iot_get_status(&azure_iot);
			switch(status)
			{
				case azure_iot_connected:
					LogInfo("Connected");
					mqttConnected = true;
					if (send_device_info)
					{
						LogInfo("Sending device infos");
						(void)azure_pnp_send_device_info(&azure_iot, properties_request_id++);
						send_device_info = false; // Only need to send once.
					}
					break;
				case azure_iot_error:
					LogError("Azure IoT client is in error state." );
					mqttConnected = false;
					azure_iot_stop(&azure_iot);
					wifiDisconnect();
					break;
				default:
					LogInfo("Status: %i", status);
					break;
			}
		}
		azure_iot_do_work(&azure_iot);
		delay(500);
	}
}

void setup() {
	delay(500);
	setCpuFrequencyMhz(80);
	Serial.begin(SERIAL_LOGGER_BAUD_RATE);
	while(!Serial);
  	set_logging_function(logging_function);
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
	//Sync time
	syncDeviceClockWithNtpServer();
	// Connect Azure IoT
	azureIotSetup();
	// if(!WiFi.isConnected() | !mqttConnected){
	// 	// Stop the client, otherwise it'll attempt to connect again
	// 	getSensorVarsFromMemory();
	// 	getPumpVarsFromMemory();
	// 	getGeneralVarsFromMemory();
	// }
	// Read all sensors at once
	readAllSensors();
	if (azure_pnp_send_telemetry(&azure_iot) != 0)
	{
		LogError("Failed sending telemetry.");          
	}
	// // Start memory
	// preferences.begin(variablesNamespace, false);
	// // Get the last time the pump was run
	// // and compute the seconds since it happenend
	// time_t diffTime[numPlants] {0};
	// for(uint8_t i=0;i<numPlants;i++){
	// 	diffTime[i] = difftime(time(NULL), pumpState[i].lastRunMemoryVar.value);
	// 	LogInfo("Seconds since last run: %i", diffTime[i]);
	// 	LogInfo("diffTime >= wateringTime: %i", diffTime[i] >= (wateringTime[i].memoryVar.value * 3600));
	// 	LogInfo("soilMoisture < moistureTresh: %i", soilMoisture[i].percVoltage.value < moistureTresh[i].memoryVar.value);
	// 	if(pumpOverride[i].memoryVar.value || (pumpSwitch[i].memoryVar.value && (diffTime[i] >= (wateringTime[i].memoryVar.value * 3600)) && (soilMoisture[i].percVoltage.value < moistureTresh[i].memoryVar.value))){
	// 		LogInfo("Running pump for %i seconds", pumpRuntime[i].memoryVar.value);
	// 		digitalWrite(pumpPins[i], HIGH);
	// 		if(mqttConnected){
	// 			esp_mqtt_client_publish(mqtt_client, pumpState[i].stateTopic.c_str(), "on", 2, 1, 0);
	// 			delay(pumpRuntime[i].memoryVar.value * sToMs);
	// 			digitalWrite(pumpPins[i], LOW);
	// 			esp_mqtt_client_publish(mqtt_client, pumpState[i].stateTopic.c_str(), "off", 3, 1, 0);
	// 		} else {
	// 			delay(pumpRuntime[i].memoryVar.value * sToMs);
	// 			digitalWrite(pumpPins[i], LOW);
	// 		}
	// 		// Set the last time the pump was run
	// 		pumpState[i].lastRunMemoryVar.setValue(time(NULL));
	// 	}
	// 	else{
	// 		LogInfo("Condition to water plant not met, pump is off");
	// 		if(mqttConnected){
	// 			esp_mqtt_client_publish(mqtt_client, pumpState[i].stateTopic.c_str(), "off", 3, 1, 0);
	// 		}
	// 	}
	// }
	// preferences.end();
	// delay(1000);
	// if(mqttConnected){
	// 	// Send an off state to mean the pump/board is going to sleep and disconnect
	// 	for(uint8_t i=0;i<numPlants;i++){
	// 		esp_mqtt_client_publish(mqtt_client, pumpState[i].availabilityTopic.c_str(), "off", 3, 1, 0);
	// 	}
	// 	//Send battery voltage
	// 	createJson<float>(batteryVoltage);
	// 	esp_mqtt_client_publish(mqtt_client, "battery/state", buffer, 0, 1, 1);
	// }
	// // Deep sleep
	// // The following lines disable all RTC features except the timer
	// esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
	// esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
	// esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
	// esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
	// // If battery voltage is too low, set deep sleep to be infinite (i.e. hibernate)
	// if(batteryVoltage>CRITICALLY_LOW_BATTERY_VOLTAGE){
	// 	Serial.println("Setting timer wakeup to "+String(samplingTime.memoryVar.value)+" seconds");
	// 	esp_sleep_enable_timer_wakeup((unsigned long) samplingTime.memoryVar.value * sToUs);
	// 	Serial.println("Going into hibernation for "+String(samplingTime.memoryVar.value)+" seconds");
	// }
	// else{
	// 	Serial.println("Hibernating without timer due to low battery charge.");
	// }
	// // Disconnect
	// if(azure_iot_get_status(&azure_iot) == azure_iot_connected){
	// 	azure_iot_stop(&azure_iot);
	// }
	// if(WiFi.isConnected()){
	// 	wifiDisconnect();
	// }
	// // Hibernate
	// esp_deep_sleep_start();
}

void loop() {}

/* === Function Implementations === */

/*
 * These are support functions used by the sample itself to perform its basic tasks
 * of connecting to the internet, syncing the board clock, ESP MQTT client event handler 
 * and logging.
 */

/* --- System and Platform Functions --- */
static void syncDeviceClockWithNtpServer()
{
	LogInfo("Setting time using SNTP");
	configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
	time_t now = time(NULL);
	while (now < UNIX_TIME_NOV_13_2017)
	{
		delay(500);
		Serial.print(".");
		now = time(NULL);
	}
	Serial.println("");
	LogInfo("Time initialized!");
}
