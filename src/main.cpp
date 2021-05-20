#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <PID_v1.h>

#include "periphery.h"
#include "buzz.h"
#include "gui.h"
#include "phase.h"
#include "profile.h"
#include "pid_setup.h"
#include "yarcweb.h"

TaskHandle_t webInterfaceTaskHandler;

float currentTemperature;
float currentTargetTemperature;
boolean thermocoupleError = false;
boolean preTemperatureSet = false;

void mainSystemSetup()
{
    tftInit();
    initGui();
    espPinInit();
    buzzStartup();
    rtcConnect();
    pidSetup();

    dataPointDuration = ((currentProfile.preheatTime + currentProfile.soakTime + currentProfile.reflowTime) / dataPoints);
}

void mainSystem()
{

    currentTime = millis();

    //Every 250 milliseconds (-> Datasheet MAX6675 -> max conversion time)
    if (currentTime - previousFastIntervalEndTime >= temperatureUpdateInterval)
    {

        currentTemperature = thermocouple.readCelsius();
        setTemperatureLabel(thermocouple.readCelsius(), currentTemperature);
        previousFastIntervalEndTime = currentTime;
    }

    //Every second
    if (currentTime - previousIntervalEndTime >= oneSecondInterval)
    {

        if (isnan(sqrt(thermocouple.readCelsius())) || (thermocouple.readCelsius() == 0.0))
        {
            currentPhase = COOLDOWN;
            Serial.println("THERMOCOUPLE NOT CONNECTED OR DAMAGED!");
            setStartButtonLabel(LV_SYMBOL_WARNING);
            setIndicatorLabel(LV_SYMBOL_WARNING);
        }

        if (processTimeCounter == dataPointIterator * dataPointDuration)
        {
            setNextChartPoints(currentTemperature, currentTargetTemperature);
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
            setIndicatorLabel(LV_SYMBOL_MINUS);
            currentTargetTemperature = currentProfile.IdleTemperature;
            setStatusLabel("Status: IDLE");
            processTimeCounter = 0;
            if (idleMessageSent == false)
            {
                Serial.println("STATUS: IDLE");
                idleMessageSent = true;
            }

            break;

        case PREHEAT:
            processTimeCounter++;
            myPID.SetTunings(PID_KP_PREHEAT, PID_KI_PREHEAT, PID_KD_PREHEAT);
            setStatusLabel("Status: PREHEAT");

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
            setStatusLabel("Status: SOAK");
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
            setStatusLabel("Status: REFLOW");
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
            setIndicatorLabel(LV_SYMBOL_UP);
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, HIGH);
        }
        else
        {
            setIndicatorLabel(LV_SYMBOL_DOWN);
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);
        }
    }
    else
    {
        digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);
    }
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

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    WiFi.mode(WIFI_AP); //Access Point mode
    WiFi.softAP(ssid, password);

    // Print ESP32 Local IP Address
    Serial.println(WiFi.softAPIP());
    setWifiLabels(ssid, password, WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", String(), false, processor); });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/style.css", "text/css"); });

    server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/main.js", "text/javascript"); });

    server.on("/profiles.json", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/profiles.json", "application/json"); });

    server.on("/create-profile", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/create-profile.html", String(), false, processor); });

    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/create-profile", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                           {
                                                                               StaticJsonDocument<300> data;
                                                                               if (json.is<JsonArray>())
                                                                               {
                                                                                   data = json.as<JsonArray>();
                                                                               }
                                                                               else if (json.is<JsonObject>())
                                                                               {
                                                                                   data = json.as<JsonObject>();
                                                                               }
                                                                               String response;
                                                                               const char *option = data["name"];
                                                                               addProfileDropdownOption(option);
                                                                               updateProfilesJson(data);
                                                                               serializeJson(data, response);
                                                                               request->send(200, "application/json", response);
                                                                               Serial.println(response);
                                                                           });
    server.addHandler(handler);

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
    xTaskCreate(systemTask, "systemTask", 4096 * 8, NULL, 1, NULL);
    webInterface();
}

void loop() {}
