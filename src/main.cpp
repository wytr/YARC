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
TaskHandle_t guiTaskHandler;

float currentTemperature;
float currentTargetTemperature;
boolean thermocoupleError = false;
boolean ambientTemperatureSet = false;

DateTime currentDateTime;

void mainSystemSetup()
{
    tftInit();
    espPinInit();
    buzzStartup();
    rtcConnect();
    currentDateTime = getDateTimeFromRtc();
    pidSetup();
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
        char currentTargetTemperatureString[64];
        sprintf(currentTargetTemperatureString, "%f", currentTargetTemperature);

        char currentTemperatureString[64];
        sprintf(currentTemperatureString, "%f", currentTemperature);

        char data[200] = "";
        char timeBuffer[8] = "mm:ss";
        sprintf(data, "%s;%s;%s", currentDateTime.toString(timeBuffer), currentTargetTemperatureString, currentTemperatureString);
        notifyClients(data);

        if (isnan(sqrt(thermocouple.readCelsius())) || (thermocouple.readCelsius() == 0.0))
        {
            currentPhase = COOLDOWN;
            Serial.println("THERMOCOUPLE NOT CONNECTED OR DAMAGED!");
            setStartButtonLabel(LV_SYMBOL_WARNING);
            setIndicatorLabel(LV_SYMBOL_WARNING);
        }
        if (processIntervalCounter != 0 && processIntervalCounter == dataPointIterator * dataPointDuration)
        {
            setNextChartPoints(currentTemperature, currentTargetTemperature);
            dataPointIterator++;
        }

        if (currentTemperature > maxProcessTemperature)
        {
            maxProcessTemperature = currentTemperature;
        }

        Input = currentTemperature;
        currentDateTime = currentDateTime + 1;
        updateClock(currentDateTime);

        switch (currentPhase)
        {

        case IDLE:
            setIndicatorLabel(LV_SYMBOL_MINUS);
            currentTargetTemperature = 0;
            setStatusLabel("Status: IDLE");
            processIntervalCounter = 0;
            if (idleMessageSent == false)
            {
                Serial.println("STATUS: IDLE");
                idleMessageSent = true;
            }

            break;

        case PREHEAT:
            processIntervalCounter++;
            myPID.SetTunings(PID_KP_PREHEAT, PID_KI_PREHEAT, PID_KD_PREHEAT);
            setStatusLabel("Status: PREHEAT");

            if (!ambientTemperatureSet)
            {
                Serial.println("ambientTemperatureSet");
                currentProfile.ambientTemperature = currentTemperature;
                currentProfile.soakRampDuration = (currentProfile.soakTemperature - currentProfile.ambientTemperature) / currentProfile.soakRampRate;
                int diagramCooldownDurationBuffer = 5;
                dataPointDuration = ((currentProfile.soakRampDuration + currentProfile.soakDuration + currentProfile.reflowDuration + currentProfile.reflowRampDuration + diagramCooldownDurationBuffer) / dataPoints);
                ambientTemperatureSet = true;
            }

            if (currentProfile.preheatCounter < currentProfile.soakRampDuration)
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
            processIntervalCounter++;
            myPID.SetTunings(PID_KP_SOAK, PID_KI_SOAK, PID_KD_SOAK);
            setStatusLabel("Status: SOAK");
            if (currentProfile.soakCounter < currentProfile.soakDuration)
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
                currentPhase = REFLOWRAMP;
            }
            Setpoint = currentTargetTemperature;
            break;

        case REFLOWRAMP:
            processIntervalCounter++;
            myPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
            setStatusLabel("Status: RAMPUP");
            if (currentProfile.reflowRampCounter < currentProfile.reflowRampDuration)
            {
                if (currentTargetTemperature < (currentProfile.reflowTemperature - 2))
                {
                    currentTargetTemperature = currentTargetTemperature + 2;
                }
                else
                {
                    currentTargetTemperature = currentProfile.reflowTemperature;
                }

                if (reflowRampMessageSent == false)
                {
                    Serial.println("STATUS: RAMPUP");
                    reflowRampMessageSent = true;
                }
                currentProfile.reflowRampCounter++;
            }
            else
            {
                currentProfile.reflowRampCounter = 0;
                reflowRampMessageSent = false;
                currentPhase = REFLOW;
            }
            Setpoint = currentTargetTemperature;
            break;

        case REFLOW:
            processIntervalCounter++;
            myPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
            setStatusLabel("Status: REFLOW");
            if (currentProfile.reflowCounter < currentProfile.reflowDuration)
            {
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
                currentTargetTemperature = 0;
                Serial.println("STATUS: COOLDOWN");
                cooldownMessageSent = true;
            }
            if (currentTemperature < 50.0)
            {
                resetMessages();
                ambientTemperatureSet = false;
                currentPhase = IDLE;
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
    initGui();
    WiFi.mode(WIFI_AP); //Access Point mode
    WiFi.softAP(ssid, password);

    // Print ESP32 Local IP Address
    Serial.println(WiFi.softAPIP());
    setWifiLabels(ssid, password, WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", String()); });

    server.on("/create-profile", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/create-profile.html", String()); });

    server.on("/profiles", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/profiles.html", String()); });

    server.on("/monitoring", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/monitoring.html", String()); });

    server.on("/materialize.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/materialize.min.css", "text/css"); });

    server.on("/diagram.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/diagram.css", "text/css"); });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/style.css", "text/css"); });

    server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/main.js", "text/javascript"); });

    server.on("/webSocket.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/webSocket.js", "text/javascript"); });

    server.on("/diagram.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/diagram.js", "text/javascript"); });

    server.on("/chart.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/chart.min.js", "text/javascript"); });

    server.on("/monitoring.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/monitoring.js", "text/javascript"); });

    server.on("/profiles.json", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/profiles.json", "application/json"); });

    AsyncCallbackJsonWebHandler *createHandler = new AsyncCallbackJsonWebHandler("/create-profile", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                 {
                                                                                     StaticJsonDocument<300> data;
                                                                                     data = json.as<JsonObject>();

                                                                                     String response;
                                                                                     updateProfilesJson(data);
                                                                                     refreshDropdownOptions();
                                                                                     serializeJson(data, response);
                                                                                     request->send(200, "application/json", response);
                                                                                     Serial.println(response);
                                                                                 });

    AsyncCallbackJsonWebHandler *removeHandler = new AsyncCallbackJsonWebHandler("/remove-profile", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                 {
                                                                                     StaticJsonDocument<300> data;
                                                                                     data = json.as<JsonObject>();

                                                                                     String response;
                                                                                     const char *option = data["name"];
                                                                                     removeProfileFromJson(option);
                                                                                     refreshDropdownOptions();
                                                                                     serializeJson(data, response);
                                                                                     request->send(200, "application/json", response);
                                                                                     Serial.println(response);
                                                                                 });

    server.addHandler(createHandler);
    server.addHandler(removeHandler);
    initWebSocket();
    server.begin();

    vTaskDelete(NULL);
}

void guiTask(void *parameter)
{
    Serial.println("hello");
}

void setup()
{
    Serial.begin(115200);
    xTaskCreate(systemTask, "systemTask", 4096 * 12, NULL, 1, NULL);
    vTaskDelay(500);
    xTaskCreate(webInterfaceTask, "webInterfaceTask", 4096 * 12, NULL, 1, &webInterfaceTaskHandler);
}

void loop() {}
