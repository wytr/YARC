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
extern unsigned long previous_thermocouple_check_time;
extern unsigned long previousTime3;
extern unsigned long oneSecondInterval;
extern unsigned long halfsecond;
extern unsigned long tenSecondInterval;
extern unsigned long quarterSecondInterval;
extern unsigned long previousFastIntervalEndTime;

extern int processTimeCounter;
//To-Do: implement this to keep track of maximum temperature per process
extern float max_process_temp;

//Messageindicators for serialprint
extern bool idleMessageSent;
extern bool preheatMessageSent;
extern bool soakMessageSent;
extern bool reflowMessageSent;
extern bool cooldownMessageSent;

#endif