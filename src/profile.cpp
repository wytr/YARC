#include <Arduino.h>
#include "SPIFFS.h"
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
    float targetTemperature = currentProfile.preheatTemperature + ((currentProfile.soakTemperature - currentProfile.preheatTemperature) / currentProfile.preheatTime) * currentProfile.preheatCounter;

    return targetTemperature;
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

    setStatusLabel("Status: COOLDOWN");
    setStartButtonLabel("START");
    setStartButtonState(LV_BTN_STATE_CHECKED_RELEASED);
}

JsonObject getProfileFromJson(char *name)
{
    File file = SPIFFS.open("/profiles.json", FILE_READ);
    JsonObject profile;
    if (file && file.size())
    {

        DynamicJsonDocument profilesJson(1300);
        JsonArray array = profilesJson.to<JsonArray>();
        DeserializationError err = deserializeJson(profilesJson, file);
        Serial.println(err.c_str());
        if (err)
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
        }
        else
        {
            Serial.println(String("search: ") + name);
            Serial.println(String("size: ") + array.size());
            for (auto value : array)
            {
                Serial.println("element");
                for (auto inner : value.to<JsonArray>())
                {
                    Serial.println("inner ");
                    //Serial.println(value);
                    if (value["name"] == name)
                    {
                        Serial.print(String("found") + name);
                    }
                }
            }
            // array.remove(0);

            String json;
            serializeJson(array, json);
            Serial.println(String("full: ") + json);
        }

        file.close();
    }
    return profile;
}

void updateProfilesJson(StaticJsonDocument<256> newProfileJson)
{
    DynamicJsonDocument doc(1024);
    JsonObject obj;
    File file = SPIFFS.open("/profiles.json", FILE_READ);
    if (!file)
    {
        Serial.println(F("Failed to create file, probably not exists"));
        Serial.println(F("Create an empty one!"));
        obj = doc.to<JsonObject>();
    }
    else
    {

        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            Serial.println(F("Error parsing JSON "));
            Serial.println(error.c_str());
            obj = doc.to<JsonObject>();
        }
        else
        {
            obj = doc.as<JsonObject>();
        }
    }
    file.close();

    JsonArray profiles;
    if (!obj.containsKey(F("profiles")))
    {
        Serial.println(F("Not find data array! Crete one!"));
        profiles = obj.createNestedArray(F("data"));
    }
    else
    {
        Serial.println(F("Find data array!"));
        profiles = obj[F("profiles")];
    }

    JsonObject objArrayProfiles = profiles.createNestedObject();

    objArrayProfiles["name"] = newProfileJson["name"];
    objArrayProfiles["soakRampDeltaTemperature"] = newProfileJson["soakRampDeltaTemperature"];
    objArrayProfiles["soakTemperature"] = newProfileJson["soakTemperature"];
    objArrayProfiles["soakDuration"] = newProfileJson["soakDuration"];
    objArrayProfiles["peakRampDeltaTemperature"] = newProfileJson["peakRampDeltaTemperature"];
    objArrayProfiles["reflowDuration"] = newProfileJson["reflowDuration"];

    file = SPIFFS.open("/profiles.json", FILE_WRITE);
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }
    file.close();
}