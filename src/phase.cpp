#include "phase.h"

unsigned long currentTime = 0;
unsigned long previousIntervalEndTime = 0;
unsigned long previousFastIntervalEndTime = 0;
unsigned long oneSecondInterval = 1000;
unsigned long temperatureUpdateInterval = 250;

int processIntervalCounter = 0;
//To-Do: implement this to keep track of maximum temperature per process
float maxProcessTemperature = 0;

bool idleMessageSent = false;
bool preheatMessageSent = false;
bool soakMessageSent = false;
bool reflowRampMessageSent = false;
bool reflowMessageSent = false;
bool cooldownMessageSent = false;

void resetMessages()
{
    idleMessageSent = false;
    preheatMessageSent = false;
    soakMessageSent = false;
    reflowRampMessageSent = false;
    reflowMessageSent = false;
    cooldownMessageSent = false;
}
