#include "periphery.h"

TFT_eSPI tft = TFT_eSPI();
RTC_DS3231 rtc;
MAX6675 thermocouple(THERMOCOUPLE_CLOCK_PIN, THERMOCOUPLE_CHIP_SELECT_PIN, THERMOCOUPLE_DATA_OUT_PIN);

void espPinInit()
{
    pinMode(THERMOCOUPLE_VCC_PIN, OUTPUT);
    digitalWrite(THERMOCOUPLE_VCC_PIN, HIGH);
    pinMode(SOLID_STATE_RELAY_OUTPUT_PIN, OUTPUT);
}

void tftInit()
{

    tft.begin();        /* TFT init */
    tft.setRotation(0); /* Landscape orientation  = 1*/
    uint16_t calData[5] = {275, 3620, 264, 3532, 2};
    tft.setTouch(calData);
}

void rtcConnect()
{

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, lets set the time!");
        // following line sets the RTC to the date &amp; time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // This line sets the RTC with an explicit date &amp; time, for example to set
        // May 20, 2021 at 7pm you would call:
        // rtc.adjust(DateTime(2021, 5, 30, 12, 16, 0));
    }
}

DateTime getDateTimeFromRtc()
{
    return rtc.now();
}