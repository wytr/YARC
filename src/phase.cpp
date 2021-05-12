#include "phase.h"

unsigned long currentTime = 0;
unsigned long previousIntervalEndTime = 0;
unsigned long previousFastIntervalEndTime = 0;
unsigned long oneSecondInterval = 1000;
unsigned long temperatureUpdateInterval = 220;

int processTimeCounter = 0;
//To-Do: implement this to keep track of maximum temperature per process
float maxProcessTemperature = 0;

bool idleMessageSent = false;
bool preheatMessageSent = false;
bool soakMessageSent = false;
bool reflowMessageSent = false;
bool cooldownMessageSent = false;

void resetMessages()
{
    idleMessageSent = false;
    preheatMessageSent = false;
    soakMessageSent = false;
    reflowMessageSent = false;
    cooldownMessageSent = false;
    currentPhase = IDLE;
}
