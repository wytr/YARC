#include <Arduino.h>
#include "SPIFFS.h"
#include "lvgl.h"
#include "profile.h"
#include "gui.h"
#include "pid_setup.h"
#include "buzz.h"
#include "periphery.h"

int dataPointIterator = 0;

profile currentProfile = {};

void setProfile(JsonObject profileJsonObject)
{
    currentProfile.soakRampRate = profileJsonObject["soakRampRate"].as<float>();
    currentProfile.soakTemperature = profileJsonObject["soakTemperature"].as<float>();
    currentProfile.soakDuration = profileJsonObject["soakDuration"].as<float>();
    currentProfile.peakRampRate = profileJsonObject["peakRampRate"].as<float>();
    currentProfile.reflowTemperature = profileJsonObject["reflowTemperature"].as<float>();
    currentProfile.reflowDuration = profileJsonObject["reflowDuration"].as<float>();
}

float calculateTargetTemperature()
{
    float targetTemperature = currentProfile.ambientTemperature + ((currentProfile.soakTemperature - currentProfile.ambientTemperature) / currentProfile.soakRampDuration) * currentProfile.preheatCounter;

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
    JsonObject targetProfile;
    if (file && file.size())
    {
        DynamicJsonDocument profilesJson(file.size() + 1024);
        JsonArray array = profilesJson.to<JsonArray>();
        DeserializationError err = deserializeJson(profilesJson, file);
        if (err)
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
        }
        else
        {
            for (JsonArray profiles : array)
            {
                for (JsonObject profile : profiles)
                {
                    if (profile["name"] == name)
                    {
                        Serial.print(String("found: ") + name);
                        targetProfile = profile;
                        break;
                    }
                }
            }
        }

        file.close();
    }
    return targetProfile;
}

char const *getProfileNames()
{
    File file = SPIFFS.open("/profiles.json", FILE_READ);
    String names;
    if (file && file.size())
    {
        Serial.println(file.size());
        DynamicJsonDocument profilesJson(file.size() + 1024);
        JsonArray array = profilesJson.to<JsonArray>();
        DeserializationError err = deserializeJson(profilesJson, file);
        if (err)
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
        }
        else
        {
            for (JsonArray profiles : array)
            {
                int iterator = 0;
                for (JsonObject profile : profiles)
                {
                    names += profile["name"].as<String>();
                    if (iterator < profiles.size() - 1)
                    {
                        names += String("\n");
                    }
                    iterator++;
                }
            }
        }
        file.close();
    }
    return names.c_str();
}

void updateProfilesJson(StaticJsonDocument<256> newProfileJson)
{
    JsonObject obj;
    File file = SPIFFS.open("/profiles.json", FILE_READ);
    DynamicJsonDocument doc(file.size());
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
    objArrayProfiles["soakRampRate"] = newProfileJson["soakRampRate"];
    objArrayProfiles["soakTemperature"] = newProfileJson["soakTemperature"];
    objArrayProfiles["soakDuration"] = newProfileJson["soakDuration"];
    objArrayProfiles["peakRampRate"] = newProfileJson["peakRampRate"];
    objArrayProfiles["reflowDuration"] = newProfileJson["reflowDuration"];

    file = SPIFFS.open("/profiles.json", FILE_WRITE);
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }
    file.close();
}