#ifndef YARC_PROFILE_H
#define YARC_PROFILE_H
#include <ArduinoJSON.h>

extern int profileDropdownOption;

extern int dataPointIterator;

struct profile
{
    float soakRampRate;
    float soakRampDuration;
    float soakDuration;
    float peakRampRate;
    float reflowDuration;
    float cooldownDuration;

    float preheatCounter;
    float soakCounter;
    float reflowCounter;
    float cooldownCounter;

    float ambientTemperature;
    float soakTemperature;
    float reflowTemperature;
};

extern profile currentProfile;

void setProfile(JsonObject profileJsonObject);
float calculateTargetTemperature();
void resetStates();
JsonObject getProfileFromJson(char *name);
char const *getProfileNames();
void updateProfilesJson(StaticJsonDocument<256> newProfileJson);

#endif