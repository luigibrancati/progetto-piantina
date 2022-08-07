#ifndef _READ_BATTERY_H_
#define _READ_BATTERY_H_

#include <esp_adc_cal.h>

static const float CRITICALLY_LOW_BATTERY_VOLTAGE = 3.30;
static const adc2_channel_t BAT_DIV = ADC2_CHANNEL_8; // GPIO 25 corresponds to ADC2 channel 8

// Taken from https://github.com/Torxgewinde/Firebeetle-2-ESP32-E/blob/main/Firebeetle_DeepSleep.ino
float readBattery() {
  int raw_value = 0;
  int value = 0;
  uint8_t rounds = 10;
  esp_adc_cal_characteristics_t adc_chars;

  //battery voltage divided by 2 can be measured at GPIO25, which equals ADC2_CHANNEL8
  adc2_config_channel_atten(BAT_DIV, ADC_ATTEN_DB_11);
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

  //to avoid noise, sample the pin several times and average the result
  for(int i=0; i<rounds; i++) {
    adc2_get_raw(BAT_DIV, ADC_WIDTH_BIT_12, &raw_value);
    value += raw_value;
  }
  value /= (uint32_t)rounds;

  //due to the voltage divider (1M+1M) values must be multiplied by 2
  //and convert mV to V
  return esp_adc_cal_raw_to_voltage(value, &adc_chars)*2.0/1000.0;
}

#endif