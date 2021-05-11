#ifndef YARC_GUI_H
#define YARC_GUI_H
#include <lvgl.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern lv_color_t buf[LV_HOR_RES_MAX * 10];
extern lv_obj_t *scr;
extern lv_theme_t *th;
extern lv_obj_t *tabview;
extern lv_obj_t *homeTab;
extern lv_obj_t *profileTab;
extern lv_obj_t *chartTab;
extern lv_obj_t *miscTab;
extern lv_obj_t *temperatureMeter;
extern lv_obj_t *temperatureLabel;
extern lv_obj_t *table;
extern lv_obj_t *chart;
extern lv_obj_t *startButton;
extern lv_obj_t *clockLabel;
extern lv_obj_t *startButtonlabel;
extern lv_obj_t *statusLabel;
extern lv_obj_t *indicatorLabel;
extern lv_obj_t *loadButtonLabel;
extern lv_obj_t *buttonMatrix;
extern lv_obj_t *loadButton;
extern lv_chart_series_t *chartSeriesOne;
extern lv_chart_series_t *chartSeriesTwo;
extern lv_style_t st;
extern lv_obj_t *dropdownList;

extern const int dataPoints;
extern int targetChart[];
extern int actualChart[];
extern int dataPointDuration;

void tftInit();
void initDisplay();
void initDriver();
void createTemperatureMeter(lv_obj_t *parent);
void lvThemeInit();
void screenInit();
void createTabview(lv_obj_t *parent);
void createTabs(lv_obj_t *parent);
void createTable(lv_obj_t *parent);
void createChart(lv_obj_t *parent);
void createTemperatureLabel(lv_obj_t *parent);
void createstatusLabel(lv_obj_t *parent);
void createIndicator(lv_obj_t *parent);
void createStartButton(lv_obj_t *parent);
void createStartButtonLabel(lv_obj_t *parent);
void createLoadButton(lv_obj_t *parent);
void createLoadButtonLabel(lv_obj_t *parent);
void createClockLabel(lv_obj_t *parent);
void createbuttonMatrix(lv_obj_t *parent);
void createDropdown(lv_obj_t *parent);
void updateTemperatureLabel(float value);
void updateTableContent(float time1, float temp1, float time2, float temp2, float time3, float temp3);
void tabviewEventCallback(lv_obj_t *obj, lv_event_t event);
void guiInit();
void buttonMatrixEvent(lv_obj_t *obj, lv_event_t event);
void startEvent(lv_obj_t *obj, lv_event_t event);
void setCallbacks();
void profileHandler(lv_obj_t *obj, lv_event_t event);
void loaderEvent(lv_obj_t *obj, lv_event_t event);
void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
bool touchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
#endif