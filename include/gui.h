#ifndef YARC_GUI_H
#define YARC_GUI_H
#include <lvgl.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern lv_disp_buf_t disp_buf;
extern lv_color_t buf[LV_HOR_RES_MAX * 10];
extern TFT_eSPI tft;
extern lv_obj_t *scr;
extern lv_theme_t *th;
extern lv_obj_t *tabview;
extern lv_obj_t *homeTab;
extern lv_obj_t *profileTab;
extern lv_obj_t *chartTab;
extern lv_obj_t *miscTab;
extern lv_obj_t *temperature_meter;
extern lv_obj_t *temperature_label;
extern lv_obj_t *table;
extern lv_obj_t *chart;
extern lv_obj_t *startbtn;
extern lv_obj_t *clockLabel;
extern lv_obj_t *startbtnlabel;
extern lv_obj_t *statuslabel;
extern lv_obj_t *indicatorlabel;
extern lv_obj_t *loadbtnlabel;
extern lv_obj_t *buttonmatrix;
extern lv_obj_t *loadbtn;
extern lv_chart_series_t *ser1;
extern lv_chart_series_t *ser2;
extern lv_style_t st;
extern lv_obj_t *ddlist;

extern const int datapoints;
extern int profile_chart[];
extern int actual_chart[];
extern int dataPointDuration;

void tft_init();
void initDisplay();
void initDriver();
void createTemperatureMeter(lv_obj_t *parent);
void lv_theme_init();
void screen_init();
void createTabview(lv_obj_t *parent);
void createTabs(lv_obj_t *parent);
void createTable(lv_obj_t *parent);
void createChart(lv_obj_t *parent);
void createTemperatureLabel(lv_obj_t *parent);
void createStatusLabel(lv_obj_t *parent);
void createIndicator(lv_obj_t *parent);
void createStartButton(lv_obj_t *parent);
void createStartButtonLabel(lv_obj_t *parent);
void createLoadButton(lv_obj_t *parent);
void createLoadButtonLabel(lv_obj_t *parent);
void createClockLabel(lv_obj_t *parent);
void createButtonMatrix(lv_obj_t *parent);
void createDropdown(lv_obj_t *parent);
void updateTemperatureLabel(float value);
void updateTableContent(float time1, float temp1, float time2, float temp2, float time3, float temp3);
void tv_event_cb(lv_obj_t *obj, lv_event_t event);
void guiInit();
void ButtonMatrixEvent(lv_obj_t *obj, lv_event_t event);
void StartEvent(lv_obj_t *obj, lv_event_t event);
void setCallbacks();
void profile_handler(lv_obj_t *obj, lv_event_t event);
void loaderEvent(lv_obj_t *obj, lv_event_t event);
void DisplayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
bool TouchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
#endif