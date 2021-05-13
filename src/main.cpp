#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <PID_v1.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "periphery.h"
#include "buzz.h"
#include "gui.h"
#include "phase.h"
#include "profile.h"
#include "pid_setup.h"

TaskHandle_t webInterfaceTaskHandler;

float currentTemperature;
float currentTargetTemperature;
boolean thermocoupleError = false;
boolean preTemperatureSet = false;

AsyncWebServer server(80);

const char *ssid = "SSID";
const char *password = "PASSWORD";

const char *PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void mainSystemSetup()
{

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

void mainSystem()
{

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
}

void systemTask(void *parameter)
{
    mainSystemSetup();

    while (1)
    {
        mainSystem();
        lv_task_handler();
    }
}

void webInterfaceTask(void *parameter)
{

    Serial.println("webInterfaceTask");

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    lv_label_set_text(wifiLabel, WiFi.localIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hello, world"); });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  String message;
                  if (request->hasParam(PARAM_MESSAGE))
                  {
                      message = request->getParam(PARAM_MESSAGE)->value();
                  }
                  else
                  {
                      message = "No message sent";
                  }
                  request->send(200, "text/plain", "Hello, GET: " + message);
              });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  String message;
                  if (request->hasParam(PARAM_MESSAGE, true))
                  {
                      message = request->getParam(PARAM_MESSAGE, true)->value();
                  }
                  else
                  {
                      message = "No message sent";
                  }
                  request->send(200, "text/plain", "Hello, POST: " + message);
              });

    server.onNotFound(notFound);

    server.begin();

    vTaskDelete(NULL);
}

void webInterface()
{
    vTaskDelay(500);
    xTaskCreate(webInterfaceTask, "webInterfaceTask", 4096 * 2, NULL, 1, &webInterfaceTaskHandler);
}

void setup()
{
    Serial.begin(115200);
    xTaskCreate(systemTask, "systemTask", 4096 * 2, NULL, 1, NULL);
    webInterface();
}

void loop() {}
