#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <max6675.h>
#include <tone32.h>
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

float currentTemp;
float currentTargetTemp;
//Bool for checking if SOLID_STATE_RELAY_OUTPUT_PIN is on or off
boolean thermocoupleError = false;
//Indicator -> Set preheattemp to ambienttemp to get THAT SWEET GERADE
boolean preTempSet = false;

//chart

int dataPointIterator = 0;

#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char *file, uint32_t line, const char *dsc)
{

    Serial.printf("%s@%d->%s\r\n", file, line, dsc);
    Serial.flush();
}
#endif

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
    buzzeralarm();

    currentProfile.cooldownCounter = 0;
    currentProfile.preheatCounter = 0;
    currentProfile.soakCounter = 0;
    currentProfile.reflowCounter = 0;
    dataPointIterator = 0;

    lv_label_set_text(statuslabel, "Status: COOLDOWN");
    lv_label_set_text(startbtnlabel, "START");
    lv_btn_set_state(startbtn, LV_BTN_STATE_CHECKED_RELEASED);
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
    Serial.print(currentTemp);
    Serial.print("    ");
    Serial.print(currentTargetTemp);
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

float calculateTargetTemp()
{
    float ret = currentProfile.preheatTemp + ((currentProfile.soakTemp - currentProfile.preheatTemp) / currentProfile.preheatTime) * currentProfile.preheatCounter;

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

    tft_init();
    lv_init();
    lv_theme_init();
    espPinInit();
    buzz_startup();

    initDisplay();

    initDriver();
    screen_init();
    guiInit();
    setCallbacks();

    rtcConnect();

    pidSetup();

    dataPointDuration = ((currentProfile.preheatTime + currentProfile.soakTime + currentProfile.reflowTime) / datapoints);
}

void loop()
{
#if USE_PROCESSING_PLOTTER != 0
    x = currentTemp;
    y = currentTargetTemp;
    p.Plot(); // usually called within loop()
#endif

    currentTime = millis();

    //Every 220 milliseconds
    if (currentTime - previousFastIntervalEndTime >= quarterSecondInterval)
    {

        currentTemp = thermocouple.readCelsius();
        updateTemperatureLabel(thermocouple.readCelsius());
        lv_linemeter_set_value(temperature_meter, currentTemp);
        previousFastIntervalEndTime = currentTime;
    }

    //Every second
    if (currentTime - previousIntervalEndTime >= oneSecondInterval)
    {

        if (isnan(sqrt(thermocouple.readCelsius())) || (thermocouple.readCelsius() == 0.0))
        {
            currentPhase = COOLDOWN;
            Serial.println("THERMOCOUPLE NOT CONNECTED OR DAMAGED!");
            lv_label_set_text(startbtnlabel, LV_SYMBOL_WARNING);
            lv_label_set_text(indicatorlabel, LV_SYMBOL_WARNING);
        }

        if (processTimeCounter == dataPointIterator * dataPointDuration)
        {
            lv_chart_set_next(chart, ser1, currentTemp);
            lv_chart_set_next(chart, ser2, currentTargetTemp);
            lv_chart_refresh(chart);
            dataPointIterator++;
        }

        if (currentTemp > max_process_temp)
        {
            max_process_temp = currentTemp;
        }

        Input = currentTemp;

        updateClock();
        serialPrintLog();

        switch (currentPhase)
        {

        case IDLE:

            lv_label_set_text(indicatorlabel, LV_SYMBOL_MINUS);
            currentTargetTemp = currentProfile.IDLETemp;
            lv_label_set_text(statuslabel, "Status: IDLE");
            processTimeCounter = 0;
            lv_label_set_text(indicatorlabel, LV_SYMBOL_MINUS);
            if (idleMessageSent == false)
            {
                Serial.println("STATUS: IDLE");
                idleMessageSent = true;
            }

            break;

        case PREHEAT:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_PREHEAT, PID_KI_PREHEAT, PID_KD_PREHEAT);
            lv_label_set_text(statuslabel, "Status: PREHEAT");

            if (!preTempSet)
            {
                currentProfile.preheatTemp = currentTemp;
                preTempSet = true;
            }

            if (currentProfile.preheatCounter < currentProfile.preheatTime)
            {
                currentTargetTemp = calculateTargetTemp();
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
            Setpoint = currentTargetTemp;
            break;

        case SOAK:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_SOAK, PID_KI_SOAK, PID_KD_SOAK);
            lv_label_set_text(statuslabel, "Status: SOAK");
            if (currentProfile.soakCounter < currentProfile.soakTime)
            {
                currentTargetTemp = currentProfile.soakTemp;

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
            Setpoint = currentTargetTemp;
            break;

        case REFLOW:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
            lv_label_set_text(statuslabel, "Status: REFLOW");
            if (currentProfile.reflowCounter < currentProfile.reflowTime)
            {
                if (currentTargetTemp < (currentProfile.reflowTemp - 2))
                {
                    currentTargetTemp = currentTargetTemp + 2;
                }
                else
                {
                    currentTargetTemp = currentProfile.reflowTemp;
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
            Setpoint = currentTargetTemp;
            break;

        case COOLDOWN:
            resetStates();
            if (!cooldownMessageSent)
            {
                currentTargetTemp = currentProfile.IDLETemp;
                Serial.println("STATUS: COOLDOWN");
                cooldownMessageSent = true;
            }
            currentProfile.cooldownCounter++;
            if (currentProfile.cooldownCounter >= currentProfile.cooldownTime || currentTemp < 50.0)
            {
                currentProfile.cooldownCounter = 0;
                resetMessages();
            }
            Setpoint = currentTargetTemp;
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
            lv_label_set_text(indicatorlabel, LV_SYMBOL_UP);
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, HIGH);
        }
        else
        {
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);
            lv_label_set_text(indicatorlabel, LV_SYMBOL_DOWN);
        }
    }

    else
        digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);

    lv_task_handler();
}