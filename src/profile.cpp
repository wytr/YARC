#include <Arduino.h>
#include "lvgl.h"
#include "profile.h"
#include "gui.h"
#include "pid_setup.h"
#include "buzz.h"
#include "periphery.h"

float leaded[6] = {60, 80, 60, 30, 150, 230};
float leadFree[6] = {60, 80, 60, 30, 150, 250};
float temper[6] = {120, 1000, 1000, 60, 60, 60};
float custom[6] = {60, 80, 60, 30, 150, 230};
int dataPointIterator = 0;

profile currentProfile =
    {
        .preheatTime = 60,
        .soakTime = 80,
        .reflowTime = 60,
        .cooldownTime = 10,

        .preheatCounter = 0,
        .soakCounter = 0,
        .reflowCounter = 0,
        .cooldownCounter = 0,

        .IdleTemperature = 0,
        .preheatTemperature = 30,
        .soakTemperature = 150,
        .reflowTemperature = 230};

void setProfile(float profileArray[])
{
    currentProfile.preheatTime = profileArray[0];
    currentProfile.soakTime = profileArray[1];
    currentProfile.reflowTime = profileArray[2];
    currentProfile.preheatTemperature = profileArray[3];
    currentProfile.soakTemperature = profileArray[4];
    currentProfile.reflowTemperature = profileArray[5];
}

float calculateTargetTemperature()
{
    float ret = currentProfile.preheatTemperature + ((currentProfile.soakTemperature - currentProfile.preheatTemperature) / currentProfile.preheatTime) * currentProfile.preheatCounter;

    return ret;
}

void resetStates()
{
    Output = 0;

    digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);
    buzzAlarm();

    currentProfile.cooldownCounter = 0;
    currentProfile.preheatCounter = 0;
    currentProfile.soakCounter = 0;
    currentProfile.reflowCounter = 0;
    dataPointIterator = 0;

    lv_label_set_text(statusLabel, "Status: COOLDOWN");
    lv_label_set_text(startButtonlabel, "START");
    lv_btn_set_state(startButton, LV_BTN_STATE_CHECKED_RELEASED);
}