#include "pid_setup.h"

double Setpoint;
double Input;
double Output;

double Kp = PID_KP_PREHEAT;
double Ki = PID_KI_PREHEAT;
double Kd = PID_KD_PREHEAT;

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

int WindowSize = 2000;
unsigned long windowStartTime;