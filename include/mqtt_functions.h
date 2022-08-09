#ifndef _MQTT_FUNCTIONS_H_
#define _MQTT_FUNCTIONS_H_

#include "global.h"
#include "logging.h"
#include "AzureIoT.h"
#include "memory.h"
#include "pumps.h"
#include "sensors.h"

#include <WiFi.h>
#include <esp_event.h>
#include <mqtt_client.h>
#include <ArduinoJson.h>
#include <Regexp.h>

#define MQTT_PROTOCOL_PREFIX "mqtts://"
/* --- Function Returns --- */
#define RESULT_OK 0
#define RESULT_ERROR __LINE__
#define MQTT_DO_NOT_RETAIN_MSG 0
#define AZ_IOT_DATA_BUFFER_SIZE 1500

static esp_mqtt_client_config_t mqtt_cfg = {};
static char mqtt_broker_uri[128];
/* --- Sample variables --- */
static azure_iot_config_t azure_iot_config;
static azure_iot_t azure_iot;
static uint8_t az_iot_data_buffer[AZ_IOT_DATA_BUFFER_SIZE];
static uint32_t properties_request_id = 0;
static bool send_device_info = true;

static esp_err_t messageHandler(esp_mqtt_event_handle_t event){
	String event_topic = convertToString(event->topic, event->topic_len);
	String event_data = convertToString(event->data, event->data_len);
	MatchState ms;
	ms.Target(event->topic);
	char result = ms.Match("[%a/]*/(%a+)/(%d+)/[%a/]*");
	if (result == REGEXP_MATCHED) {
		LogInfo("Received command/value from %s", event_topic);
		char buf[100];
		String match_string = ms.GetCapture(buf, 0);
		uint8_t index = atoi(ms.GetCapture(buf, 1));
		preferences.begin(variablesNamespace, false);
		if(match_string == PumpOverride::classId){
			if (event_data == "on") {
				LogInfo("Turning Pump Override %i on", index);
				pumpOverride[index].memoryVar.setValue(true);
				esp_mqtt_client_publish(mqtt_client, pumpOverride[index].stateTopic.c_str(), "on", 2, 1, 1);
			}
			else if (event_data == "off") {
				LogInfo("Turning Pump Override %i off", index);
				pumpOverride[index].memoryVar.setValue(false);
				esp_mqtt_client_publish(mqtt_client, pumpOverride[index].stateTopic.c_str(), "off", 3, 1, 1);
			}
			LogInfo("Pump Override %i value: %i", index, pumpOverride[index].memoryVar.value);
		}
		else if(match_string == PumpSwitch::classId){
			if (event_data == "on") {
				LogInfo("Turning Pump Switch %i on", index);
				pumpSwitch[index].memoryVar.setValue(true);
				esp_mqtt_client_publish(mqtt_client, pumpSwitch[index].stateTopic.c_str(), "on", 2, 1, 1);
			}
			else if (event_data == "off") {
				LogInfo("Turning Pump Switch %i off", index);
				pumpSwitch[index].memoryVar.setValue(false);
				esp_mqtt_client_publish(mqtt_client, pumpSwitch[index].stateTopic.c_str(), "off", 3, 1, 1);
			}
			LogInfo("Pump Switch %i value: %i", index, pumpSwitch[index].memoryVar.value);
		}
		else if(match_string == MoistureTresh::classId){
			moistureTresh[index].memoryVar.setValue(event_data.toFloat());
			LogInfo("Moisture Tresh %i value: %f", index, moistureTresh[index].memoryVar.value);
		}
		else if(match_string == AirValue::classId){
			airValue.memoryVar.setValue(event_data.toFloat());
			LogInfo("Air Value value: %f", airValue.memoryVar.value);
		}
		else if(match_string == WaterValue::classId){
			waterValue.memoryVar.setValue(event_data.toFloat());
			LogInfo("Water Value value: %f", waterValue.memoryVar.value);
		}
		else if(match_string == SamplingTime::classId){
			samplingTime.memoryVar.setValue(event_data.toInt());
			LogInfo("Sampling Time value: %i", samplingTime.memoryVar.value);
		}
		else if(match_string == PumpRuntime::classId){
			pumpRuntime[index].memoryVar.setValue(event_data.toInt());
			LogInfo("Pump Runtime %i value: %i", index, pumpRuntime[index].memoryVar.value);
		}
		else if(match_string == WateringTime::classId){
			wateringTime[index].memoryVar.setValue(event_data.toInt());
			LogInfo("Watering Time %i value: %i", index, wateringTime[index].memoryVar.value);
		}
		else{
			LogInfo("Received unknown topic %s", event_topic);
		}
		preferences.end();
	}
	return ESP_OK;
}

static esp_err_t mqttEventCallbackHandler(esp_mqtt_event_handle_t event) {
	switch(event->event_id){
		case MQTT_EVENT_ERROR:
			LogError("MQTT client in ERROR state.");
			LogError( 
				"esp_tls_stack_err=%d; esp_tls_cert_verify_flags=%d;esp_transport_sock_errno=%d;error_type=%d;connect_return_code=%d",  
				event->error_handle->esp_tls_stack_err,
				event->error_handle->esp_tls_cert_verify_flags,
				event->error_handle->esp_transport_sock_errno,
				event->error_handle->error_type,
				event->error_handle->connect_return_code);
			switch (event->error_handle->connect_return_code) 
			{
				case MQTT_CONNECTION_ACCEPTED: 
				LogError("connect_return_code=MQTT_CONNECTION_ACCEPTED"); 
				break; 
				case MQTT_CONNECTION_REFUSE_PROTOCOL: 
				LogError("connect_return_code=MQTT_CONNECTION_REFUSE_PROTOCOL"); 
				break; 
				case MQTT_CONNECTION_REFUSE_ID_REJECTED: 
				LogError("connect_return_code=MQTT_CONNECTION_REFUSE_ID_REJECTED"); 
				break; 
				case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE: 
				LogError("connect_return_code=MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE"); 
				break; 
				case MQTT_CONNECTION_REFUSE_BAD_USERNAME: 
				LogError("connect_return_code=MQTT_CONNECTION_REFUSE_BAD_USERNAME"); 
				break; 
				case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED: 
				LogError("connect_return_code=MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED"); 
				break; 
				default: 
				LogError("connect_return_code=unknown (%d)", event->error_handle->connect_return_code); 
				break; 
			};
			break;
		case MQTT_EVENT_CONNECTED:
			LogInfo("MQTT client connected (session_present=%d).", event->session_present);
			if (azure_iot_mqtt_client_connected(&azure_iot) != 0)
			{
				LogError("azure_iot_mqtt_client_connected failed.");
			}
			LogInfo("Fetching variables from the broker.");
			mqttConnected = true;
			for(uint8_t i=0;i<numPlants;i++){
				// Send an on state to mean the pump/board has started and is connected
				esp_mqtt_client_publish(mqtt_client, pumpState[i].availabilityTopic.c_str(), "on", 2, 1, 0);
				esp_mqtt_client_publish(mqtt_client, pumpState[i].stateTopic.c_str(), "off", 3, 1, 1);
				// Subscriptions
				esp_mqtt_client_subscribe(mqtt_client, wateringTime[i].stateTopic.c_str(), 1);
				esp_mqtt_client_subscribe(mqtt_client, moistureTresh[i].stateTopic.c_str(), 1);
				esp_mqtt_client_subscribe(mqtt_client, pumpOverride[i].commandTopic.c_str(), 1);
				esp_mqtt_client_subscribe(mqtt_client, pumpSwitch[i].commandTopic.c_str(), 1);
				esp_mqtt_client_subscribe(mqtt_client, pumpRuntime[i].stateTopic.c_str(), 1);
			}
			esp_mqtt_client_subscribe(mqtt_client, airValue.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(mqtt_client, waterValue.stateTopic.c_str(), 1);
			esp_mqtt_client_subscribe(mqtt_client, samplingTime.stateTopic.c_str(), 1);
			LogInfo("Subscribed to all topics");
			break;
		case MQTT_EVENT_DISCONNECTED:
			LogInfo("TEST", "MQTT event: %d. MQTT_EVENT_DISCONNECTED", event->event_id);
			if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
			{
				LogError("azure_iot_mqtt_client_disconnected failed.");
      		}
			mqttConnected = false;
			break;
		case MQTT_EVENT_SUBSCRIBED:
			LogInfo("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_SUBSCRIBED", event->msg_id, event->event_id);
			if (azure_iot_mqtt_client_subscribe_completed(&azure_iot, event->msg_id) != 0)
			{
				LogError("azure_iot_mqtt_client_subscribe_completed failed.");
			}
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			LogInfo("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_UNSUBSCRIBED", event->msg_id, event->event_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			LogInfo("TEST", "MQTT event: %d. MQTT_EVENT_PUBLISHED", event->event_id);
			if (azure_iot_mqtt_client_publish_completed(&azure_iot, event->msg_id) != 0)
			{
				LogError("azure_iot_mqtt_client_publish_completed failed (message id=%d).", event->msg_id);
			}
			break;
		case MQTT_EVENT_DATA:
			LogInfo("TEST", "MQTT msgid= %d event: %d. MQTT_EVENT_DATA", event->msg_id, event->event_id);
			LogInfo("TEST", "Topic length %d. Data length %d", event->topic_len, event->data_len);
			LogInfo("TEST","Incoming data: %.*s %.*s\n", event->topic_len, event->topic, event->data_len, event->data);
			mqtt_message_t mqtt_message;
			mqtt_message.topic = az_span_create((uint8_t*)event->topic, event->topic_len);
			mqtt_message.payload = az_span_create((uint8_t*)event->data, event->data_len);
			mqtt_message.qos = mqtt_qos_at_most_once; // QoS is unused by azure_iot_mqtt_client_message_received.
			if (azure_iot_mqtt_client_message_received(&azure_iot, &mqtt_message) != 0)
			{
				LogError("azure_iot_mqtt_client_message_received failed (topic=%.*s).", event->topic_len, event->topic);
			}
			messageHandler(event);
			break;
		default:
			LogError("MQTT event UNKNOWN.");
			break;
	}
	return ESP_OK;
}

/* --- MQTT Interface Functions --- */
/*
 * These functions are used by Azure IoT to interact with whatever MQTT client used by the sample
 * (in this case, Espressif's ESP MQTT). Please see the documentation in AzureIoT.h for more details.
 */

/*
 * See the documentation of `mqtt_client_init_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_init_function(mqtt_client_config_t* mqtt_client_config, mqtt_client_handle_t *mqtt_client_handle)
{
	int result;
	esp_mqtt_client_config_t mqtt_config;
	memset(&mqtt_config, 0, sizeof(mqtt_config));  

	az_span mqtt_broker_uri_span = AZ_SPAN_FROM_BUFFER(mqtt_broker_uri);
	mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, AZ_SPAN_FROM_STR(MQTT_PROTOCOL_PREFIX));
	mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, mqtt_client_config->address);
	az_span_copy_u8(mqtt_broker_uri_span, null_terminator);

	mqtt_config.uri = mqtt_broker_uri;
	mqtt_config.port = mqtt_client_config->port;
	mqtt_config.client_id = (const char*)az_span_ptr(mqtt_client_config->client_id);
	mqtt_config.username = (const char*)az_span_ptr(mqtt_client_config->username);
	mqtt_config.password = (const char*)az_span_ptr(mqtt_client_config->password);	

	mqtt_config.keepalive = 30;
	mqtt_config.disable_clean_session = 0;
	mqtt_config.disable_auto_reconnect = false;
	mqtt_config.event_handle = mqttEventCallbackHandler;
	mqtt_config.user_context = NULL;
	LogInfo("MQTT client target uri set to '%s'", mqtt_broker_uri);
	mqtt_client = esp_mqtt_client_init(&mqtt_config);
	if (mqtt_client == NULL)
	{
		LogError("esp_mqtt_client_init failed.");
		result = 1;
	}
	else
	{
		esp_err_t start_result = esp_mqtt_client_start(mqtt_client);
		if (start_result != ESP_OK)
		{
			LogError("esp_mqtt_client_start failed (error code: 0x%08x).", start_result);
			result = 1;
		}
		else
		{
			*mqtt_client_handle = mqtt_client;
			result = 0;
		}
	}
	return result;
}

/*
 * See the documentation of `mqtt_client_deinit_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_deinit_function(mqtt_client_handle_t mqtt_client_handle)
{
	int result = 0;
	esp_mqtt_client_handle_t esp_mqtt_client_handle = (esp_mqtt_client_handle_t)mqtt_client_handle;
	LogInfo("MQTT client being disconnected.");
	if (esp_mqtt_client_stop(esp_mqtt_client_handle) != ESP_OK)
	{
		LogError("Failed stopping MQTT client.");
	}
	if (esp_mqtt_client_destroy(esp_mqtt_client_handle) != ESP_OK)
	{
		LogError("Failed destroying MQTT client.");
	}
	if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
	{
		LogError("Failed updating azure iot client of MQTT disconnection.");      
	}
	return 0;
}

/*
 * See the documentation of `mqtt_client_subscribe_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_subscribe_function(mqtt_client_handle_t mqtt_client_handle, az_span topic, mqtt_qos_t qos)
{
	LogInfo("MQTT client subscribing to '%.*s'", az_span_size(topic), az_span_ptr(topic));
	// As per documentation, `topic` always ends with a null-terminator.
	// esp_mqtt_client_subscribe returns the packet id or negative on error already, so no conversion is needed.
	int packet_id = esp_mqtt_client_subscribe((esp_mqtt_client_handle_t)mqtt_client_handle, (const char*)az_span_ptr(topic), (int)qos);
	return packet_id;
}

/*
 * See the documentation of `mqtt_client_publish_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_publish_function(mqtt_client_handle_t mqtt_client_handle, mqtt_message_t* mqtt_message)
{
  	LogInfo("MQTT client publishing to '%s'", az_span_ptr(mqtt_message->topic));
  	int mqtt_result = esp_mqtt_client_publish(
		(esp_mqtt_client_handle_t)mqtt_client_handle, 
		(const char*)az_span_ptr(mqtt_message->topic), // topic is always null-terminated.
		(const char*)az_span_ptr(mqtt_message->payload), 
		az_span_size(mqtt_message->payload),
		(int)mqtt_message->qos,
		MQTT_DO_NOT_RETAIN_MSG
	);
	if (mqtt_result == -1)
	{
		return RESULT_ERROR;
	}
	else
	{
		return RESULT_OK;
	}
}

#endif
