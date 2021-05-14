#ifndef YARC_GUI_H
#define YARC_GUI_H
#include <lvgl.h>

extern const int dataPoints;
extern int targetChart[];
extern int actualChart[];
extern int dataPointDuration;

void initGui();

void setIndicatorLabel(const char *text);
void setStartButtonLabel(const char *text);
void setStatusLabel(const char *text);
void setNextChartPoints(float current, float target);
void setStartButtonState(int state);
void setTemperatureLabel(float value, float currentTemperature);
void setWifiLabels(const char *ssid, const char *pw, const char *ip);
void addProfileDropdownOption(const char *name);
void updateClock();

#endif