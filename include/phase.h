#ifndef YARC_PHASE_H
#define YARC_PHASE_H

enum phase
{
    IDLE = 0,
    PREHEAT = 1,
    SOAK = 2,
    REFLOW = 3,
    COOLDOWN = 4

};
extern phase currentPhase;

extern unsigned long currentTime;
extern unsigned long previousIntervalEndTime;
extern unsigned long oneSecondInterval;
extern unsigned long temperatureUpdateInterval;
extern unsigned long previousFastIntervalEndTime;

extern int processTimeCounter;
//To-Do: implement this to keep track of maximum temperature per process
extern float maxProcessTemperature;

//Messageindicators for serialprint
extern bool idleMessageSent;
extern bool preheatMessageSent;
extern bool soakMessageSent;
extern bool reflowMessageSent;
extern bool cooldownMessageSent;

#endif