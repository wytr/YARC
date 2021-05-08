#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <max6675.h>
#include <tone32.h>
#include <PID_v1.h>
#include <Plotter.h>
#include "pindefines.h"


#define PID_SAMPLE_TIME 1000
#define USE_PROCESSING_PLOTTER 0
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

#if USE_PROCESSING_PLOTTER != 0

double x;
double y;
Plotter p;

#endif

//PID
double Setpoint, Input, Output;
double Kp = PID_KP_PREHEAT, Ki = PID_KI_PREHEAT, Kd = PID_KD_PREHEAT;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
int WindowSize = 2000;
unsigned long windowStartTime;

MAX6675 thermocouple(THERMOCOUPLE_CLOCK_PIN, THERMOCOUPLE_CHIP_SELECT_PIN, THERMOCOUPLE_DATA_OUT_PIN);

//RTC
RTC_DS3231 rtc;

int profile_dropdown_option;
int processTimeCounter = 0;
//To-Do: implement this to keep track of maximum temperature per process
float max_process_temp = 0;

enum phase
{
    IDLE = 0,
    PREHEAT = 1,
    SOAK = 2,
    REFLOW = 3,
    COOLDOWN = 4

};
phase currentPhase = IDLE;

struct profileStruct
{
    float preheatTime = 60;
    float soakTime = 80;
    float reflowTime = 60;
    float cooldownTime = 10;

    float preheatCounter = 0;
    float soakCounter = 0;
    float reflowCounter = 0;
    float cooldownCounter = 0;

    float IDLETemp = 0;
    float preheatTemp = 30;
    float soakTemp = 150;
    float reflowTemp = 230;
} currentProfile;

char buffer_preheat_time[4];
char buffer_preheat_temp[4];
char buffer_soak_time[4];
char buffer_soak_temp[4];
char buffer_reflow_time[4];
char buffer_reflow_temp[4];

float leaded[6] = {60, 80, 60, 30, 150, 230};
float leadFree[6] = {60, 80, 60, 30, 150, 250};
float temper[6] = {120, 1000, 1000, 60, 60, 60};
float custom[6] = {60, 80, 60, 30, 150, 230};

static const char *btnm_map[] = {"1", "2", "3", "\n",
                                 "4", "5", "6", "\n",
                                 "7", "8", "9", "\n",
                                 "0", "<", "OK", ""};

//Messageindicator for serialprint
bool idleMessageSent = false;
bool preheatMessageSent = false;
bool soakMessageSent = false;
bool reflowMessageSent = false;
bool cooldownMessageSent = false;

const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//TFT Instance
TFT_eSPI tft = TFT_eSPI();
//LVGL UP IN HERE, UP IN HERE
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];
const int screenWidth = 240;
const int screenHeight = 320;
static lv_obj_t *scr;
static lv_theme_t *th;
static lv_obj_t *tabview;
static lv_obj_t *homeTab;
static lv_obj_t *profileTab;
static lv_obj_t *chartTab;
static lv_obj_t *miscTab;
static lv_obj_t *temperature_meter;
static lv_obj_t *temperature_label;
static lv_obj_t *table;
static lv_obj_t *chart;
static lv_obj_t *startbtn;
static lv_obj_t *clockLabel;
static lv_obj_t *startbtnlabel;
static lv_obj_t *statuslabel;
static lv_obj_t *indicatorlabel;
static lv_obj_t *loadbtnlabel;
static lv_obj_t *buttonmatrix;
static lv_obj_t *loadbtn;
static lv_chart_series_t *ser1;
static lv_chart_series_t *ser2;
static lv_style_t st;
static lv_obj_t *ddlist;

//Timerstuff
unsigned long currentTime = 0;
unsigned long previousIntervalEndTime = 0;
unsigned long previous_thermocouple_check_time = 0;
unsigned long previousTime3 = 0;
unsigned long oneSecondInterval = 1000;
unsigned long halfsecond = 500;
unsigned long tenSecondInterval = 10000;

float currentTemp;
float currentTargetTemp;
//Bool for checking if SOLID_STATE_RELAY_OUTPUT_PIN is on or off
boolean thermocoupleError = false;
//Indicator -> Set preheattemp to ambienttemp to get THAT SWEET GERADE
boolean preTempSet = false;

//chart
const int datapoints = 40;
int profile_chart[datapoints];
int actual_chart[datapoints];

int dataPointDuration = 2;
int dataPointIterator = 0;

#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char *file, uint32_t line, const char *dsc)
{

    Serial.printf("%s@%d->%s\r\n", file, line, dsc);
    Serial.flush();
}
#endif

void tft_init()
{

    tft.begin();        /* TFT init */
    tft.setRotation(0); /* Landscape orientation  = 1*/
    uint16_t calData[5] = {275, 3620, 264, 3532, 2};
    tft.setTouch(calData);
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
}

void esp_pin_init()
{
    pinMode(THERMOCOUPLE_VCC_PIN, OUTPUT);
    digitalWrite(THERMOCOUPLE_VCC_PIN, HIGH);
    pinMode(SOLID_STATE_RELAY_OUTPUT_PIN, OUTPUT);
}

void lv_theme_init()
{
    th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY, LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_DEFAULT_FLAG, LV_THEME_DEFAULT_FONT_SMALL, LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);
    lv_theme_set_act(th);
}

void screen_init()
{
    scr = lv_cont_create(NULL, NULL);
    lv_scr_load(scr);
}

void createTabview(lv_obj_t *parent)
{
    tabview = lv_tabview_create(parent, NULL);
    lv_tabview_set_btns_pos(tabview, LV_TABVIEW_TAB_POS_BOTTOM);
    lv_tabview_set_anim_time(tabview, 50);
    lv_obj_set_size(tabview, LV_HOR_RES_MAX, LV_VER_RES_MAX);
}

void createTabs(lv_obj_t *parent)
{

    homeTab = lv_tabview_add_tab(parent, LV_SYMBOL_HOME);
    profileTab = lv_tabview_add_tab(parent, LV_SYMBOL_SETTINGS);
    chartTab = lv_tabview_add_tab(parent, LV_SYMBOL_IMAGE);
    miscTab = lv_tabview_add_tab(parent, LV_SYMBOL_EDIT);

    lv_page_set_scroll_propagation(profileTab, false);
    lv_page_set_scrollbar_mode(profileTab, LV_SCROLLBAR_MODE_OFF);

    lv_page_set_scroll_propagation(homeTab, false);
    lv_page_set_scrollbar_mode(homeTab, LV_SCROLLBAR_MODE_OFF);

    lv_page_set_scroll_propagation(chartTab, false);
    lv_page_set_scrollbar_mode(chartTab, LV_SCROLLBAR_MODE_OFF);

    lv_page_set_scroll_propagation(miscTab, false);
    lv_page_set_scrollbar_mode(miscTab, LV_SCROLLBAR_MODE_OFF);
}

void createTable(lv_obj_t *parent)
{

    table = lv_table_create(parent, NULL);

    lv_obj_align(table, NULL, LV_ALIGN_IN_LEFT_MID, 0, -80);

    lv_obj_set_click(table, false);

    lv_obj_set_style_local_text_font(table, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_14);

    lv_table_set_col_width(table, 0, 70);
    lv_table_set_col_width(table, 1, 70);
    lv_table_set_col_width(table, 2, 70);

    lv_table_set_col_cnt(table, 3);
    lv_table_set_row_cnt(table, 2);
    /*Fill the first column*/
    lv_table_set_cell_value(table, 0, 0, "p_t");
    lv_table_set_cell_value(table, 1, 0, "p_T");

    /*Fill the second column*/
    lv_table_set_cell_value(table, 0, 1, "s_t");
    lv_table_set_cell_value(table, 1, 1, "s_T");

    /*Fill the third column*/
    lv_table_set_cell_value(table, 0, 2, "r_t");
    lv_table_set_cell_value(table, 1, 2, "r_T");
}

void createChart(lv_obj_t *parent)
{

    for (int i = 0; i < datapoints; i++) // ...initialize it
    {
        profile_chart[i] = 0;
    }
    for (int i = 0; i < datapoints; i++) // ...initialize it
    {
        actual_chart[i] = 0;
    }

    chart = lv_chart_create(chartTab, NULL);
    lv_obj_set_size(chart, 400, 240);
    lv_obj_align(chart, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE); /*Show lines and points too*/
    lv_chart_set_range(chart, 0, 300);
    lv_chart_set_point_count(chart, datapoints);

    //nicht auswählbar -> scrollen funktioniert über diagramm
    lv_obj_set_click(chart, false);

    //margin test
    lv_obj_set_style_local_pad_left(chart, LV_CHART_PART_BG, LV_STATE_DEFAULT, 4 * (LV_DPI / 10));
    lv_obj_set_style_local_pad_top(chart, LV_CHART_PART_BG, LV_STATE_DEFAULT, 2 * (LV_DPI / 10));
    //lv_obj_set_style_local_pad_bottom(chart, LV_CHART_PART_BG, LV_STATE_DEFAULT, 1.5 * (LV_DPI / 10));

    //lv_chart_set_x_tick_texts(chart, "1\n2\n3\n4\n", 4, LV_CHART_AXIS_DRAW_LAST_TICK);
    //lv_chart_set_x_tick_length(chart, 2, 2);
    lv_chart_set_y_tick_texts(chart, "300\n225\n150\n75\n", 4, LV_CHART_AXIS_DRAW_LAST_TICK);
    lv_chart_set_y_tick_length(chart, 4, 4);

    /*Add two data series*/

    ser2 = lv_chart_add_series(chart, LV_COLOR_RED);
    ser1 = lv_chart_add_series(chart, LV_COLOR_BLACK);
    /*Set the next points on 'ser1'*/
    //const int SIZE = 20;

    for (int i = 0; i < datapoints; i++)
    {
        lv_chart_set_next(chart, ser1, profile_chart[i]);
        lv_chart_set_next(chart, ser2, actual_chart[i]);
    }

    /*Directly set points on 'ser2'
    for (int j = 0; j < SIZE; j++)
    {
        ser2->points[j] = actual_chart[j];
    }
    */
    lv_chart_refresh(chart); /*Required after direct set*/
    ;
}

void createTemperatureMeter(lv_obj_t *parent)
{
    temperature_meter = lv_linemeter_create(parent, NULL);
    lv_obj_set_size(temperature_meter, 170, 170);
    lv_obj_align(temperature_meter, NULL, LV_ALIGN_CENTER, 0, -20);
    lv_linemeter_set_value(temperature_meter, 0);
    lv_linemeter_set_range(temperature_meter, 10, 250);
    lv_obj_set_click(temperature_meter, false);
}

void createTemperatureLabel(lv_obj_t *parent)
{

    temperature_label = lv_label_create(parent, NULL);
    lv_label_set_text(temperature_label, "N/A");
    lv_obj_align(temperature_label, NULL, LV_ALIGN_CENTER, -10, -20);
    lv_obj_set_style_local_text_font(temperature_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
}

void createStatusLabel(lv_obj_t *parent)
{

    statuslabel = lv_label_create(parent, NULL);
    lv_label_set_text(statuslabel, "Status: IDLE");
    lv_obj_align(statuslabel, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
}

void createIndicator(lv_obj_t *parent)
{

    indicatorlabel = lv_label_create(parent, NULL);
    lv_label_set_text(indicatorlabel, LV_SYMBOL_MINUS);
    lv_obj_align(indicatorlabel, NULL, LV_ALIGN_IN_TOP_RIGHT, -5, 0);
}

void createStartButton(lv_obj_t *parent)
{

    startbtn = lv_btn_create(parent, NULL);
    lv_obj_align(startbtn, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);
    lv_btn_set_checkable(startbtn, true);
    lv_btn_toggle(startbtn);
}

void createStartButtonLabel(lv_obj_t *parent)
{

    startbtnlabel = lv_label_create(parent, NULL);
    lv_label_set_text(startbtnlabel, "START");
    lv_obj_set_style_local_text_font(startbtnlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
}

void createLoadButton(lv_obj_t *parent)
{

    loadbtn = lv_btn_create(parent, NULL);
    lv_obj_align(loadbtn, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);
}

void createLoadButtonLabel(lv_obj_t *parent)
{

    loadbtnlabel = lv_label_create(parent, NULL);
    lv_label_set_text(loadbtnlabel, "LOAD");
}

void createClockLabel(lv_obj_t *parent)
{

    clockLabel = lv_label_create(parent, NULL);
    lv_obj_align(clockLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 3, 0);
}

void createButtonMatrix(lv_obj_t *parent)
{

    buttonmatrix = lv_btnmatrix_create(parent, NULL);
    lv_obj_set_size(buttonmatrix, 240, 160);
    lv_btnmatrix_set_map(buttonmatrix, btnm_map);
    lv_btnmatrix_set_btn_width(buttonmatrix, 10, 1);
    lv_btnmatrix_set_btn_ctrl(buttonmatrix, 11, LV_BTNMATRIX_CTRL_CHECK_STATE);

    lv_obj_align(buttonmatrix, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_focus_parent(buttonmatrix, true);
}

void createDropdown(lv_obj_t *parent)
{
    ddlist = lv_dropdown_create(parent, NULL);
    lv_dropdown_set_options(ddlist, "leaded\n"
                                    "leadFree\n"
                                    "temper\n"
                                    "custom\n");

    lv_obj_align(ddlist, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);
}

void pid_setup()
{
    windowStartTime = millis();
    Setpoint = 0;
    myPID.SetOutputLimits(0, WindowSize);
    myPID.SetSampleTime(PID_SAMPLE_TIME);
    myPID.SetMode(AUTOMATIC);
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch(&touchX, &touchY, 600);

    if (!touched)
    {

        data->state = LV_INDEV_STATE_REL;
        return false;
    }
    else
    {
        //tone(BUZZER_PIN, NOTE_A4, 10, BUZZER_CHANNEL);
        //noTone(BUZZER_PIN, BUZZER_CHANNEL);
        data->state = LV_INDEV_STATE_PR;
    }
    if (touchX > screenWidth || touchY > screenHeight)
    {
        Serial.println("Y or y outside of expected parameters..");
        Serial.print("y:");
        Serial.print(touchX);
        Serial.print(" x:");
        Serial.print(touchY);
    }
    else
    {
        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;
    }

    return false; /*Return `false` because we are not buffering and no more data to read*/
}

void profile_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        // TODO load profile into table
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        profile_dropdown_option = lv_dropdown_get_selected(obj);
        printf("Option: %s\n", buf);
    }
}

void updateTemperatureLabel(float value)
{
    char buffer[12] = {0};
    snprintf(buffer, sizeof(buffer), "%.2f°C", value);
    lv_label_set_text(temperature_label, buffer);
}

void updateTableContent(float time1, float temp1, float time2, float temp2, float time3, float temp3)
{
    char time_buffer1[12] = {0};
    char temp_buffer1[12] = {0};
    char time_buffer2[12] = {0};
    char temp_buffer2[12] = {0};
    char time_buffer3[12] = {0};
    char temp_buffer3[12] = {0};

    snprintf(time_buffer1, sizeof(time_buffer1), "%.0fs", time1);
    //snprintf(temp_buffer1, sizeof(temp_buffer1), "%.0f°C", temp1);
    snprintf(temp_buffer1, sizeof(temp_buffer1), "ramp", temp1);
    snprintf(time_buffer2, sizeof(time_buffer2), "%.0fs", time2);
    snprintf(temp_buffer2, sizeof(temp_buffer2), "%.0f°C", temp2);
    snprintf(time_buffer3, sizeof(time_buffer3), "%.0fs", time3);
    snprintf(temp_buffer3, sizeof(temp_buffer3), "%.0f°C", temp3);

    /*Fill the first column*/
    lv_table_set_cell_value(table, 0, 0, time_buffer1);
    lv_table_set_cell_value(table, 1, 0, temp_buffer1);

    /*Fill the second column*/
    lv_table_set_cell_value(table, 0, 1, time_buffer2);
    lv_table_set_cell_value(table, 1, 1, temp_buffer2);

    /*Fill the third column*/
    lv_table_set_cell_value(table, 0, 2, time_buffer3);
    lv_table_set_cell_value(table, 1, 2, temp_buffer3);
}

void buzzer()
{

    tone(BUZZER_PIN, NOTE_C5, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
}

void buzz_startup()
{
    tone(BUZZER_PIN, NOTE_F5, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_A5, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_C6, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_E6, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_G6, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_F6, 100, BUZZER_CHANNEL);
}

void buzz_note(int note)
{

    tone(BUZZER_PIN, note, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
}

void buzz_multiple_times(int x)
{

    for (int i = 0; i < x; i++)
    {
        tone(BUZZER_PIN, NOTE_C5, 100, BUZZER_CHANNEL);
        noTone(BUZZER_PIN, BUZZER_CHANNEL);
        tone(BUZZER_PIN, NOTE_C1, 100, BUZZER_CHANNEL);
        noTone(BUZZER_PIN, BUZZER_CHANNEL);
    }
}

void buzzeralarm()
{

    tone(BUZZER_PIN, NOTE_A5, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_A5, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_A5, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
}

void loader_event_handler(lv_obj_t *obj, lv_event_t event)
{

    if (event == LV_EVENT_CLICKED)
    {
        if (currentPhase == IDLE)
        {

            if (profile_dropdown_option == 0)
            {
                currentProfile.preheatTime = leaded[0];
                currentProfile.soakTime = leaded[1];
                currentProfile.reflowTime = leaded[2];
                currentProfile.preheatTemp = leaded[3];
                currentProfile.soakTemp = leaded[4];
                currentProfile.reflowTemp = leaded[5];
            }
            if (profile_dropdown_option == 1)
            {
                currentProfile.preheatTime = leadFree[0];
                currentProfile.soakTime = leadFree[1];
                currentProfile.reflowTime = leadFree[2];
                currentProfile.preheatTemp = leadFree[3];
                currentProfile.soakTemp = leadFree[4];
                currentProfile.reflowTemp = leadFree[5];
            }
            if (profile_dropdown_option == 2)
            {
                currentProfile.preheatTime = temper[0];
                currentProfile.soakTime = temper[1];
                currentProfile.reflowTime = temper[2];
                currentProfile.preheatTemp = temper[3];
                currentProfile.soakTemp = temper[4];
                currentProfile.reflowTemp = temper[5];
            }
            if (profile_dropdown_option == 3)
            {
                currentProfile.preheatTime = custom[0];
                currentProfile.soakTime = custom[1];
                currentProfile.reflowTime = custom[2];
                currentProfile.preheatTemp = custom[3];
                currentProfile.soakTemp = custom[4];
                currentProfile.reflowTemp = custom[5];
            }
            buzz_multiple_times(profile_dropdown_option + 1);
            updateTableContent(currentProfile.preheatTime, currentProfile.preheatTemp, currentProfile.soakTime, currentProfile.soakTemp, currentProfile.reflowTime, currentProfile.reflowTemp);
            dataPointDuration = ((currentProfile.preheatTime + currentProfile.soakTime + currentProfile.reflowTime) / datapoints);
            //Serial.println(dataPointDuration);
        }
        else
        {

            buzzeralarm();
            Serial.println("CANT CHANGE PROFILE WHILE NOT IN IDLE!");
        }
    }
}

void start_event_handler(lv_obj_t *obj, lv_event_t event)
{

    if (event == LV_EVENT_CLICKED)
    {
        Serial.println("Clicked\n");
        buzzer();
    }
    else if (event == LV_EVENT_VALUE_CHANGED)
    {
        Serial.println("Toggled\n");

        if (currentPhase == IDLE)
        {
            currentPhase = PREHEAT;
            Serial.println("Process started.");
            lv_label_set_text(startbtnlabel, "STOP");
        }
        else
        {
            currentPhase = COOLDOWN;
            Serial.println("aborted.. cooling down");
            lv_label_set_text(startbtnlabel, "START");
        }
    }
}

void buttonmatrix_event_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        const char *txt = lv_btnmatrix_get_active_btn_text(obj);
        if (txt == "OK")
        {
            for (int i = 0; i < 40; i++)
            {
                lv_chart_set_next(chart, ser1, 0);
                lv_chart_set_next(chart, ser2, 0);
            }
            lv_chart_refresh(chart);
        }
        Serial.print(txt);
    }
}

void tv_event_cb(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        buzz_note(NOTE_C6);
    }
}

void rtc_connect()
{

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, lets set the time!");
        // following line sets the RTC to the date &amp; time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // This line sets the RTC with an explicit date &amp; time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }
}

void setup()
{
#if USE_PROCESSING_PLOTTER != 0
    p.Begin();
    p.AddTimeGraph("PID-Regler", 1000, "momentane Temperatur", x, "Zieltemperatur", y);
#endif

#if USE_PROCESSING_PLOTTER == 0
    Serial.begin(115200);
#endif

    tft_init();
    lv_init();
    lv_theme_init();
    esp_pin_init();
    buzz_startup();

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read; //callback funktion zum auslesen des touchscreens SKKKRRR SKKRR
    lv_indev_drv_register(&indev_drv);
    lv_style_init(&st);
    lv_style_set_text_font(&st, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    screen_init();
    createTabview(scr);
    createStatusLabel(scr);
    createIndicator(scr);
    createTabs(tabview);
    createTemperatureMeter(homeTab);
    createTemperatureLabel(homeTab);
    createStartButton(homeTab);
    createTable(profileTab);
    createDropdown(profileTab);
    createLoadButton(profileTab);
    createChart(chartTab);
    createStartButtonLabel(startbtn);
    createLoadButtonLabel(loadbtn);
    createClockLabel(scr);
    createButtonMatrix(miscTab);
    //CALLBACKS
    lv_obj_set_event_cb(tabview, tv_event_cb);
    lv_obj_set_event_cb(startbtn, start_event_handler);
    lv_obj_set_event_cb(loadbtn, loader_event_handler);
    lv_obj_set_event_cb(buttonmatrix, buttonmatrix_event_handler);
    lv_obj_set_event_cb(ddlist, profile_handler);

    rtc_connect();

    pid_setup();

    dataPointDuration = ((currentProfile.preheatTime + currentProfile.soakTime + currentProfile.reflowTime) / datapoints);
}

void loop()
{
#if USE_PROCESSING_PLOTTER != 0
    x = currentTemp;
    y = currentTargetTemp;
    p.Plot(); // usually called within loop()
#endif

    currentTime = millis();

    if (currentTime - previousIntervalEndTime >= oneSecondInterval)
    {

        if (isnan(sqrt(thermocouple.readCelsius())) || (thermocouple.readCelsius() == 0.0))
        {
            currentPhase = COOLDOWN;
            Serial.println("THERMOCOUPLE NOT CONNECTED OR DAMAGED!");
            lv_label_set_text(startbtnlabel, LV_SYMBOL_WARNING);
            lv_label_set_text(indicatorlabel, LV_SYMBOL_WARNING);
        }

        if (processTimeCounter == dataPointIterator * dataPointDuration)
        {
            lv_chart_set_next(chart, ser1, currentTemp);
            lv_chart_set_next(chart, ser2, currentTargetTemp);
            lv_chart_refresh(chart);
            dataPointIterator++;
        }

        currentTemp = thermocouple.readCelsius();

        if (currentTemp > max_process_temp)
        {
            max_process_temp = currentTemp;
        }

        Input = currentTemp;

        char buf1[9] = "hh:mm";
        DateTime now = rtc.now();

        lv_label_set_text(clockLabel, now.toString(buf1));

        Serial.print(currentTemp);
        Serial.print("    ");
        Serial.print(currentTargetTemp);
        Serial.print("    ");
        Serial.print(Output);
        Serial.print("    ");
        Serial.print("processtime = ");
        Serial.print(processTimeCounter);
        Serial.print("    ");
        Serial.print(" Kp = ");
        Serial.print(myPID.GetKp());
        Serial.print(" Ki = ");
        Serial.print(myPID.GetKi(), 4);
        Serial.print(" Kd = ");
        Serial.println(myPID.GetKd());

        lv_linemeter_set_value(temperature_meter, currentTemp);
        updateTemperatureLabel(thermocouple.readCelsius());

        switch (currentPhase)
        {

        case IDLE:

            lv_label_set_text(indicatorlabel, LV_SYMBOL_MINUS);
            currentTargetTemp = currentProfile.IDLETemp;
            lv_label_set_text(statuslabel, "Status: IDLE");
            processTimeCounter = 0;
            lv_label_set_text(indicatorlabel, LV_SYMBOL_MINUS);
            if (idleMessageSent == false)
            {
                Serial.println("STATUS: IDLE");
                idleMessageSent = true;
            }

            break;

        case PREHEAT:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_PREHEAT, PID_KI_PREHEAT, PID_KD_PREHEAT);
            lv_label_set_text(statuslabel, "Status: PREHEAT");

            if (!preTempSet)
            {
                currentProfile.preheatTemp = currentTemp;
                preTempSet = true;
            }

            if (currentProfile.preheatCounter < currentProfile.preheatTime)
            {
                currentTargetTemp = currentProfile.preheatTemp + ((currentProfile.soakTemp - currentProfile.preheatTemp) / currentProfile.preheatTime) * currentProfile.preheatCounter;
                if (!preheatMessageSent)
                {
                    Serial.println("STATUS: PREHEAT");
                    preheatMessageSent = true;
                }
                currentProfile.preheatCounter++;
            }
            else
            {
                currentProfile.preheatCounter = 0;
                preheatMessageSent = false;
                currentPhase = SOAK;
            }
            Setpoint = currentTargetTemp;
            break;

        case SOAK:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_SOAK, PID_KI_SOAK, PID_KD_SOAK);
            lv_label_set_text(statuslabel, "Status: SOAK");
            if (currentProfile.soakCounter < currentProfile.soakTime)
            {
                currentTargetTemp = currentProfile.soakTemp;

                if (!soakMessageSent)
                {
                    Serial.println("STATUS: SOAK");
                    soakMessageSent = true;
                }

                currentProfile.soakCounter++;
            }
            else
            {
                currentProfile.soakCounter = 0;
                soakMessageSent = false;
                currentPhase = REFLOW;
            }
            Setpoint = currentTargetTemp;
            break;

        case REFLOW:

            processTimeCounter++;
            myPID.SetTunings(PID_KP_REFLOW, PID_KI_REFLOW, PID_KD_REFLOW);
            lv_label_set_text(statuslabel, "Status: REFLOW");
            if (currentProfile.reflowCounter < currentProfile.reflowTime)
            {
                if (currentTargetTemp < (currentProfile.reflowTemp - 2))
                {
                    currentTargetTemp = currentTargetTemp + 2;
                }
                else
                {
                    currentTargetTemp = currentProfile.reflowTemp;
                }

                if (reflowMessageSent == false)
                {
                    Serial.println("STATUS: REFLOW");
                    reflowMessageSent = true;
                }
                currentProfile.reflowCounter++;
            }
            else
            {
                currentProfile.reflowCounter = 0;
                reflowMessageSent = false;
                currentPhase = COOLDOWN;
            }
            Setpoint = currentTargetTemp;
            break;

        case COOLDOWN:
            Output = 0;
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);
            buzzeralarm();
            currentProfile.cooldownCounter = 0;
            currentProfile.preheatCounter = 0;
            currentProfile.soakCounter = 0;
            currentProfile.reflowCounter = 0;
            dataPointIterator = 0;

            lv_label_set_text(statuslabel, "Status: COOLDOWN");
            lv_label_set_text(startbtnlabel, "START");
            lv_btn_set_state(startbtn, LV_BTN_STATE_CHECKED_RELEASED);

            if (!cooldownMessageSent)
            {
                currentTargetTemp = currentProfile.IDLETemp;
                Serial.println("STATUS: COOLDOWN");
                cooldownMessageSent = true;
            }
            currentProfile.cooldownCounter++;
            if (currentProfile.cooldownCounter >= currentProfile.cooldownTime || currentTemp < 50.0)
            {
                currentProfile.cooldownCounter = 0;
                idleMessageSent = false;
                preheatMessageSent = false;
                soakMessageSent = false;
                reflowMessageSent = false;
                cooldownMessageSent = false;
                currentPhase = IDLE;
            }
            Setpoint = currentTargetTemp;
            break;
        }

        previousIntervalEndTime = currentTime;
    }

    if (currentPhase != IDLE && currentPhase != COOLDOWN)
    {

        myPID.Compute();

        if (millis() - windowStartTime > WindowSize)
        {
            windowStartTime += WindowSize;
        }
        if (Output > millis() - windowStartTime)
        {
            lv_label_set_text(indicatorlabel, LV_SYMBOL_UP);
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, HIGH);
        }
        else
        {
            digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);
            lv_label_set_text(indicatorlabel, LV_SYMBOL_DOWN);
        }
    }

    else
        digitalWrite(SOLID_STATE_RELAY_OUTPUT_PIN, LOW);

    lv_task_handler();
}