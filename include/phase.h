#ifndef YARC_PHASE_H
#define YARC_PHASE_H

enum phase
{
    IDLE = 0,
    PREHEAT = 1,
    SOAK = 2,
    REFLOWRAMP = 3,
    REFLOW = 4,
    COOLDOWN = 5

};
extern phase currentPhase;

extern unsigned long currentTime;
extern unsigned long previousIntervalEndTime;
extern unsigned long oneSecondInterval;
extern unsigned long temperatureUpdateInterval;
extern unsigned long previousFastIntervalEndTime;

extern int processIntervalCounter;
//To-Do: implement this to keep track of maximum temperature per process
extern float maxProcessTemperature;

//Messageindicators for serialprint
extern bool idleMessageSent;
extern bool preheatMessageSent;
extern bool soakMessageSent;
extern bool reflowRampMessageSent;
extern bool reflowMessageSent;
extern bool cooldownMessageSent;

void resetMessages();

#endif