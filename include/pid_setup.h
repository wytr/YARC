#ifndef YARC_PID_SETUP_H
#define YARC_PID_SETUP_H
#include <PID_v1.h>

// ***** PRE-HEAT STAGE *****
#define PID_KP_PREHEAT 100
#define PID_KI_PREHEAT 0.025
#define PID_KD_PREHEAT 20
// ***** SOAKING STAGE *****
#define PID_KP_SOAK 300
#define PID_KI_SOAK 0.05
#define PID_KD_SOAK 250
// ***** REFLOW STAGE *****
#define PID_KP_REFLOW 300
#define PID_KI_REFLOW 0.05
#define PID_KD_REFLOW 350
#define PID_SAMPLE_TIME 1000

extern PID myPID;
extern double Setpoint, Input, Output, Kp, Ki, Kd;

extern int WindowSize;
extern unsigned long windowStartTime;

#endif