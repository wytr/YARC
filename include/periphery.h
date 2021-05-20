#ifndef YARC_PERIPHERY_H
#define YARC_PERIPHERY_H

#include <max6675.h>
#include <RTClib.h>
#include <TFT_eSPI.h>

extern RTC_DS3231 rtc;
extern MAX6675 thermocouple;
extern TFT_eSPI tft;

#define THERMOCOUPLE_DATA_OUT_PIN 26
#define THERMOCOUPLE_CHIP_SELECT_PIN 27
#define THERMOCOUPLE_CLOCK_PIN 14
#define THERMOCOUPLE_VCC_PIN 12

#define SOLID_STATE_RELAY_OUTPUT_PIN 33

#define BUZZER_PIN 999
#define BUZZER_CHANNEL 1

void espPinInit();
void tftInit();
void rtcConnect();

#endif