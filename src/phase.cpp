#include "phase.h"

int processTimeCounter = 0;
//To-Do: implement this to keep track of maximum temperature per process
float max_process_temp = 0;

bool idleMessageSent = false;
bool preheatMessageSent = false;
bool soakMessageSent = false;
bool reflowMessageSent = false;
bool cooldownMessageSent = false;

unsigned long currentTime = 0;
unsigned long previousIntervalEndTime = 0;
unsigned long previous_thermocouple_check_time = 0;
unsigned long previousFastIntervalEndTime = 0;
unsigned long previousTime3 = 0;
unsigned long oneSecondInterval = 1000;
unsigned long halfsecond = 500;
unsigned long quarterSecondInterval = 220;
unsigned long tenSecondInterval = 10000;
