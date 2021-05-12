#ifndef YARC_PROFILE_H
#define YARC_PROFILE_H

extern int profileDropdownOption;

extern float leaded[6];
extern float leadFree[6];
extern float temper[6];
extern float custom[6];
extern int dataPointIterator;

struct profile
{
    float preheatTime;
    float soakTime;
    float reflowTime;
    float cooldownTime;

    float preheatCounter;
    float soakCounter;
    float reflowCounter;
    float cooldownCounter;

    float IdleTemperature;
    float preheatTemperature;
    float soakTemperature;
    float reflowTemperature;
};

extern profile currentProfile;

void setProfile(float *profileArray);
float calculateTargetTemperature();
void resetStates();

#endif