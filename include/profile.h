#ifndef YARC_PROFILE_H
#define YARC_PROFILE_H

extern int profile_dropdown_option;

extern float leaded[6];
extern float leadFree[6];
extern float temper[6];
extern float custom[6];

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

    float IDLETemp;
    float preheatTemp;
    float soakTemp;
    float reflowTemp;
};

extern profile currentProfile;

void setProfile(float *profileArray);

#endif