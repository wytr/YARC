#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <max6675.h>
#include <PID_v1.h>
#include <Plotter.h>

#include "pindefines.h"
#include "buzz.h"
#include "gui.h"
#include "phase.h"
#include "profile.h"
#include "pid_setup.h"

#define USE_PROCESSING_PLOTTER 0

#if USE_PROCESSING_PLOTTER != 0

double x;
double y;
Plotter p;

#endif

MAX6675 thermocouple(THERMOCOUPLE_CLOCK_PIN, THERMOCOUPLE_CHIP_SELECT_PIN, THERMOCOUPLE_DATA_OUT_PIN);

RTC_DS3231 rtc;

float currentTemperature;
float currentTargetTemperature;
boolean thermocoupleError = false;
boolean preTemperatureSet = false;

int dataPointIterator = 0;

void espPinInit()
{
    pinMode(THERMOCOUPLE_VCC_PIN, OUTPUT);
    digitalWrite(THERMOCOUPLE_VCC_PIN, HIGH);
    pinMode(SOLID_STATE_RELAY_OUTPUT_PIN, OUTPUT);
}

void pidSetup()
{
    windowStartTime = millis();
    Setpoint = 0;
    myPID.SetOutputLimits(0, WindowSize);
    myPID.SetSampleTime(PID_SAMPLE_TIME);
    myPID.SetMode(AUTOMATIC);
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
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }
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

void resetMessages()
{
    idleMessageSent = false;
    preheatMessageSent = false;
    soakMessageSent = false;
    reflowMessageSent = false;
    cooldownMessageSent = false;
    currentPhase = IDLE;
}

void serialPrintLog()
{
    Serial.print(currentTemperature);
    Serial.print("    ");
    Serial.print(currentTargetTemperature);
    Serial.print("    ");
    Serial.print(Output);
    Serial.print("    ");
    Serial.print("processtime = ");
    Serial.print(processTimeCounter);
    Serial.print("    ");
    Serial.print(" Kp = ");
    Serial.print(myPID.GetKp());
    Serial.print(" Ki = ");
    Serial.print(myPID.GetKi(), 4);
    Serial.print(" Kd = ");
    Serial.println(myPID.GetKd());
}

void updateClock()
{
    char timeBuffer[9] = "hh:mm:ss";
    DateTime now = rtc.now();
    lv_label_set_text(clockLabel, now.toString(timeBuffer));
}

float calculateTargetTemperature()
{
    float ret = currentProfile.preheatTemperature + ((currentProfile.soakTemperature - currentProfile.preheatTemperature) / currentProfile.preheatTime) * currentProfile.preheatCounter;

    return ret;
}

void setup()
{
#if USE_PROCESSING_PLOTTER != 0
    p.Begin();
    p.AddTimeGraph("PID-Regler", 1000, "momentane Temperatur", x, "Zieltemperatur", y);
#endif

#if USE_PROCESSING_PLOTTER == 0
    Serial.begin(115200);
#endif

    tftInit();
    lv_init();
    lvThemeInit();
    espPinInit();
    buzzStartup();

    initDisplay();

    initDriver();
    screenInit();
    guiInit();
    setCallbacks();

    rtcConnect();

    pidSetup();

    dataPointDuration = ((currentProfile.preheatTime + currentProfile.soakTime + currentProfile.reflowTime) / dataPoints);
}

void loop()
{
#if USE_PROCESSING_PLOTTER != 0
    x = currentTemperature;
    y = currentTargetTemperature;
    p.Plot(); // usually called within loop()
#endif

    currentTime = millis();

    //Every 220 milliseconds (-> Datasheet MAX6675 -> max conversion time)
    if (currentTime - previousFastIntervalEndTime >= temperatureUpdateInterval)
    {

        currentTemperature = thermocouple.readCelsius();
        updateTemperatureLabel(thermocouple.readCelsius());
        lv_linemeter_set_value(temperatureMeter, currentTemperature);
        previousFastIntervalEndTime = currentTime;
    }

    //Every second
    if (currentTime - previousIntervalEndTime >= oneSecondInterval)
    {

        if (isnan(sqrt(thermocouple.readCelsius())) || (thermocouple.readCelsius() == 0.0))
        {
            currentPhase = COOLDOWN;
            Serial.println("THERMOCOUPLE NOT CONNECTED OR DAMAGED!");
            lv_label_set_text(startButtonlabel, LV_SYMBOL_WARNING);
            lv_label_set_text(indicatorLabel, LV_SYMBOL_WARNING);
        }

        if (processTimeCounter == dataPointIterator * dataPointDuration)
        {
            lv_chart_set_next(chart, chartSeriesOne, currentTemperature);
            lv_chart_set_next(chart, chartSeriesTwo, currentTargetTemperature);
            lv_chart_refresh(chart);
            dataPointIterator++;
        }

        if (currentTemperature > maxProcessTemperature)
        {
            maxProcessTemperature = currentTemperature;
        }

        Input = currentTemperature;

        updateClock();
        serialPrintLog();

        switch (currentPhase)
        {

        case IDLE:

            lv_label_set_text(indicatorLabel, LV_SYMBOL_MINUS);
            currentTargetTemperature = currentProfile.IdleTemperature;
            lv_label_set_text(statusLabel, "Status: IDLE");
            processTimeCounter = 0;
            lv_label_set_text(indicatorLabel, LV_SYMBOL_MINUS);
            if (idleMessageSent == false)
            {
                Serial.println("STATUS: IDLE");
                idleMessageSent = true;
            }

            break;

        case PREHEAT:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_PREHEAT, PID_KI_PREHEAT, PID_KD_PREHEAT);
            lv_label_set_text(statusLabel, "Status: PREHEAT");

            if (!preTemperatureSet)
            {
                currentProfile.preheatTemperature = currentTemperature;
                preTemperatureSet = true;
            }

            if (currentProfile.preheatCounter < currentProfile.preheatTime)
            {
                currentTargetTemperature = calculateTargetTemperature();
                if (!preheatMessageSent)
                {
                    Serial.println("STATUS: PREHEAT");
                    preheatMessageSent = true;
                }
                currentProfile.preheatCounter++;
            }
            else
            {
                currentProfile.preheatCounter = 0;
                preheatMessageSent = false;
                currentPhase = SOAK;
            }
            Setpoint = currentTargetTemperature;
            break;

        case SOAK:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_SOAK, PID_KI_SOAK, PID_KD_SOAK);
            lv_label_set_text(statusLabel, "Status: SOAK");
            if (currentProfile.soakCounter < currentProfile.soakTime)
            {
                currentTargetTemperature = currentProfile.soakTemperature;

                if (!soakMessageSent)
                {
                    Serial.println("STATUS: SOAK");
                    soakMessageSent = true;
                }

                currentProfile.soakCounter++;
            }
            else
            {
                currentProfile.soakCounter = 0;
                soakMessageSent = false;
                currentPhase = REFLOW;
            }
            Setpoint = currentTargetTemperature;
            break;

        case REFLOW:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
            lv_label_set_text(statusLabel, "Status: REFLOW");
            if (currentProfile.reflowCounter < currentProfile.reflowTime)
            {
                if (currentTargetTemperature < (currentProfile.reflowTemperature - 2))
                {
                    currentTargetTemperature = currentTargetTemperature + 2;
                }
                else
                {
                    currentTargetTemperature = currentProfile.reflowTemperature;
                }

                if (reflowMessageSent == false)
                {
                    Serial.println("STATUS: REFLOW");
                    reflowMessageSent = true;
                }
                currentProfile.reflowCounter++;
            }
            else
            {
                currentProfile.reflowCounter = 0;
                reflowMessageSent = false;
                currentPhase = COOLDOWN;
            }
            Setpoint = currentTargetTemperature;
            break;

        case COOLDOWN:
            resetStates();
            if (!cooldownMessageSent)
            {
                currentTargetTemperature = currentProfile.IdleTemperature;
                Serial.println("STATUS: COOLDOWN");
                cooldownMessageSent = true;
            }
            currentProfile.cooldownCounter++;
            if (currentProfile.cooldownCounter >= currentProfile.cooldownTime || currentTemperature < 50.0)
            {
                currentProfile.cooldownCounter = 0;
                resetMessages();
            }
            Setpoint = currentTargetTemperature;
            break;
        }

        previousIntervalEndTime = currentTime;
    }

    //While process is running
    if (currentPhase != IDLE && currentPhase != COOLDOWN)
    {

        myPID.Compute();

        if (millis() - windowStartTime > WindowSize)
        {
            windowStartTime += WindowSize;
        }
        if (Output > millis() - windowStartTime)
        {
            lv_label_set_text(indicatorLabel, LV_SYMBOL_UP);
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, HIGH);
        }
        else
        {
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);
            lv_label_set_text(indicatorLabel, LV_SYMBOL_DOWN);
        }
    }

    else
        digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);

    lv_task_handler();
}