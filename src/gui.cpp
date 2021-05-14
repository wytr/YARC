#include <Arduino.h>
#include <ArduinoJSON.h>
#include "SPIFFS.h"
#include "periphery.h"
#include "gui.h"
#include "Tone32.h"
#include "buzz.h"
#include "phase.h"
#include "profile.h"

lv_disp_buf_t displayBuffer;
lv_color_t buf[LV_HOR_RES_MAX * 10];
lv_obj_t *scr;
lv_theme_t *th;
lv_obj_t *tabview;
lv_obj_t *homeTab;
lv_obj_t *profileTab;
lv_obj_t *chartTab;
lv_obj_t *miscTab;
lv_obj_t *wifiTab;
lv_obj_t *temperatureMeter;
lv_obj_t *temperatureLabel;
lv_obj_t *wifiTitleLabel;
lv_obj_t *wifiSsidLabel;
lv_obj_t *wifiPasswordLabel;
lv_obj_t *webInterfaceIpLabel;
lv_obj_t *table;
lv_obj_t *chart;
lv_obj_t *startButton;
lv_obj_t *clockLabel;
lv_obj_t *startButtonlabel;
lv_obj_t *statusLabel;
lv_obj_t *indicatorLabel;
lv_obj_t *loadButtonLabel;
lv_obj_t *buttonMatrix;
lv_obj_t *loadButton;
lv_chart_series_t *chartSeriesActual;
lv_chart_series_t *chartSeriesTarget;
lv_style_t st;
lv_obj_t *dropdownList;

const int dataPoints = 40;
int targetChart[dataPoints];
int actualChart[dataPoints];
int profileDropdownOption;
int dataPointDuration = 2;

char buffer_preheat_time[4];
char buffer_preheat_temp[4];
char buffer_soak_time[4];
char buffer_soak_temp[4];
char buffer_reflow_time[4];
char buffer_reflow_temp[4];

phase currentPhase;

static const char *btnm_map[] = {"1", "2", "3", "\n",
                                 "4", "5", "6", "\n",
                                 "7", "8", "9", "\n",
                                 "0", "<", "OK", ""};

void setIndicatorLabel(const char *text)
{
    lv_label_set_text(indicatorLabel, text);
}

void setStartButtonLabel(const char *text)
{
    lv_label_set_text(startButtonlabel, text);
}

void setNextChartPoints(float current, float target)
{
    lv_chart_set_next(chart, chartSeriesActual, current);
    lv_chart_set_next(chart, chartSeriesTarget, target);
    lv_chart_refresh(chart);
}

void setStatusLabel(const char *text)
{
    lv_label_set_text(statusLabel, text);
}

void setTemperatureLabel(float value, float currentTemperature)
{
    char buffer[12] = {0};
    snprintf(buffer, sizeof(buffer), "%.2f°C", value);
    lv_label_set_text(temperatureLabel, buffer);
    lv_linemeter_set_value(temperatureMeter, currentTemperature);
}

void setStartButtonState(int state)
{
    lv_btn_set_state(startButton, state);
}

void addProfileDropdownOption(const char *name)
{
    lv_dropdown_add_option(dropdownList, name, LV_DROPDOWN_POS_LAST);
}

void createTemperatureMeter(lv_obj_t *parent)
{
    temperatureMeter = lv_linemeter_create(parent, NULL);
    lv_obj_set_size(temperatureMeter, 170, 170);
    lv_obj_align(temperatureMeter, NULL, LV_ALIGN_CENTER, 0, -20);
    lv_linemeter_set_value(temperatureMeter, 0);
    lv_linemeter_set_range(temperatureMeter, 10, 250);
    lv_obj_set_click(temperatureMeter, false);
}

void lvThemeInit()
{
    th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY, LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_DEFAULT_FLAG, LV_THEME_DEFAULT_FONT_SMALL, LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);
    lv_theme_set_act(th);
}

void screenInit()
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
    wifiTab = lv_tabview_add_tab(parent, LV_SYMBOL_WIFI);

    lv_page_set_scroll_propagation(profileTab, false);
    lv_page_set_scrollbar_mode(profileTab, LV_SCROLLBAR_MODE_OFF);

    lv_page_set_scroll_propagation(homeTab, false);
    lv_page_set_scrollbar_mode(homeTab, LV_SCROLLBAR_MODE_OFF);

    lv_page_set_scroll_propagation(chartTab, false);
    lv_page_set_scrollbar_mode(chartTab, LV_SCROLLBAR_MODE_OFF);

    lv_page_set_scroll_propagation(miscTab, false);
    lv_page_set_scrollbar_mode(miscTab, LV_SCROLLBAR_MODE_OFF);

    lv_page_set_scroll_propagation(wifiTab, false);
    lv_page_set_scrollbar_mode(wifiTab, LV_SCROLLBAR_MODE_OFF);
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

    for (int i = 0; i < dataPoints; i++) // ...initialize it
    {
        targetChart[i] = 0;
    }
    for (int i = 0; i < dataPoints; i++) // ...initialize it
    {
        actualChart[i] = 0;
    }

    chart = lv_chart_create(chartTab, NULL);
    lv_obj_set_size(chart, 400, 240);
    lv_obj_align(chart, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE); /*Show lines and points too*/
    lv_chart_set_range(chart, 0, 300);
    lv_chart_set_point_count(chart, dataPoints);

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

    chartSeriesTarget = lv_chart_add_series(chart, LV_COLOR_RED);
    chartSeriesActual = lv_chart_add_series(chart, LV_COLOR_BLACK);
    /*Set the next points on 'chartSeriesActual'*/
    //const int SIZE = 20;

    for (int i = 0; i < dataPoints; i++)
    {
        lv_chart_set_next(chart, chartSeriesActual, targetChart[i]);
        lv_chart_set_next(chart, chartSeriesTarget, actualChart[i]);
    }

    /*Directly set points on 'chartSeriesTarget'
    for (int j = 0; j < SIZE; j++)
    {
        chartSeriesTarget->points[j] = actualChart[j];
    }
    */
    lv_chart_refresh(chart); /*Required after direct set*/
}

void createTemperatureLabel(lv_obj_t *parent)
{

    temperatureLabel = lv_label_create(parent, NULL);
    lv_label_set_text(temperatureLabel, "N/A");
    lv_obj_align(temperatureLabel, NULL, LV_ALIGN_CENTER, -10, -20);
    lv_obj_set_style_local_text_font(temperatureLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
}

void createStatusLabel(lv_obj_t *parent)
{

    statusLabel = lv_label_create(parent, NULL);
    lv_label_set_text(statusLabel, "Status: IDLE");
    lv_obj_align(statusLabel, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
}

void createIndicator(lv_obj_t *parent)
{

    indicatorLabel = lv_label_create(parent, NULL);
    lv_label_set_text(indicatorLabel, LV_SYMBOL_MINUS);
    lv_obj_align(indicatorLabel, NULL, LV_ALIGN_IN_TOP_RIGHT, -5, 0);
}

void createStartButton(lv_obj_t *parent)
{

    startButton = lv_btn_create(parent, NULL);
    lv_obj_align(startButton, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);
    lv_btn_set_checkable(startButton, true);
    lv_btn_toggle(startButton);
}

void createStartButtonLabel(lv_obj_t *parent)
{

    startButtonlabel = lv_label_create(parent, NULL);
    lv_label_set_text(startButtonlabel, "START");
    lv_obj_set_style_local_text_font(startButtonlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
}

void createLoadButton(lv_obj_t *parent)
{

    loadButton = lv_btn_create(parent, NULL);
    lv_obj_align(loadButton, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);
}

void createLoadButtonLabel(lv_obj_t *parent)
{
    loadButtonLabel = lv_label_create(parent, NULL);
    lv_label_set_text(loadButtonLabel, "LOAD");
}

void createWifiTitleLabel(lv_obj_t *parent)
{
    wifiTitleLabel = lv_label_create(parent, NULL);
    lv_label_set_text(wifiTitleLabel, "WIFI");
    lv_obj_align(wifiTitleLabel, NULL, LV_ALIGN_IN_TOP_MID, 0, 40);
}

void createWifiSsidLabel(lv_obj_t *parent)
{
    wifiSsidLabel = lv_label_create(parent, NULL);
    lv_label_set_text(wifiSsidLabel, "SSID:");
    lv_obj_align(wifiSsidLabel, NULL, LV_ALIGN_IN_LEFT_MID, 10, 0);
}

void createWifiPasswordLabel(lv_obj_t *parent)
{
    wifiPasswordLabel = lv_label_create(parent, NULL);
    lv_label_set_text(wifiPasswordLabel, "PW:");
    lv_obj_align(wifiPasswordLabel, NULL, LV_ALIGN_IN_LEFT_MID, 10, 30);
}

void createWebInterfaceIpLabel(lv_obj_t *parent)
{
    webInterfaceIpLabel = lv_label_create(parent, NULL);
    lv_label_set_text(webInterfaceIpLabel, "IP:");
    lv_obj_align(webInterfaceIpLabel, NULL, LV_ALIGN_IN_LEFT_MID, 10, 60);
}

void setWifiLabels(const char *ssid, const char *pw, const char *ip)
{
    lv_label_set_text_fmt(wifiSsidLabel, "SSID: %s", ssid);
    lv_label_set_text_fmt(wifiPasswordLabel, "PW: %s", pw);
    lv_label_set_text_fmt(webInterfaceIpLabel, "IP: %s", ip);
}

void createClockLabel(lv_obj_t *parent)
{

    clockLabel = lv_label_create(parent, NULL);
    lv_obj_align(clockLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 3, 0);
}

void createbuttonMatrix(lv_obj_t *parent)
{

    buttonMatrix = lv_btnmatrix_create(parent, NULL);
    lv_obj_set_size(buttonMatrix, 240, 160);
    lv_btnmatrix_set_map(buttonMatrix, btnm_map);
    lv_btnmatrix_set_btn_width(buttonMatrix, 10, 1);
    lv_btnmatrix_set_btn_ctrl(buttonMatrix, 11, LV_BTNMATRIX_CTRL_CHECK_STATE);

    lv_obj_align(buttonMatrix, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_focus_parent(buttonMatrix, true);
}

void createDropdown(lv_obj_t *parent)
{
    dropdownList = lv_dropdown_create(parent, NULL);
    lv_dropdown_set_options(dropdownList, "leaded\n"
                                          "leadFree\n"
                                          "temper");

    lv_obj_align(dropdownList, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);
}

void updateClock()
{
    char timeBuffer[9] = "hh:mm:ss";
    DateTime now = rtc.now();
    lv_label_set_text(clockLabel, now.toString(timeBuffer));
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
    snprintf(temp_buffer1, sizeof(temp_buffer1), "ramp");
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

void tabviewEventCallback(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        buzzNote(NOTE_C6);
    }
}

void createGuiObjects()
{
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
    createStartButtonLabel(startButton);
    createLoadButtonLabel(loadButton);
    createWifiTitleLabel(wifiTab);
    createWifiSsidLabel(wifiTab);
    createWifiPasswordLabel(wifiTab);
    createWebInterfaceIpLabel(wifiTab);
    createClockLabel(scr);
    createbuttonMatrix(miscTab);
}

void buttonMatrixEvent(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        const char *txt = lv_btnmatrix_get_active_btn_text(obj);
        if (strcmp(txt, "OK") == 1)
        {
            for (int i = 0; i < 40; i++)
            {
                lv_chart_set_next(chart, chartSeriesActual, 0);
                lv_chart_set_next(chart, chartSeriesTarget, 0);
            }
            lv_chart_refresh(chart);
        }
        Serial.print(txt);
    }
}

void startEvent(lv_obj_t *obj, lv_event_t event)
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
            lv_label_set_text(startButtonlabel, "STOP");
        }
        else
        {
            currentPhase = COOLDOWN;
            Serial.println("aborted.. cooling down");
            lv_label_set_text(startButtonlabel, "START");
        }
    }
}

void profileHandler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        // TODO load profile into table
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        profileDropdownOption = lv_dropdown_get_selected(obj);
        printf("Option: %s\n", buf);
    }
}

void loaderEvent(lv_obj_t *obj, lv_event_t event)
{

    if (event == LV_EVENT_CLICKED)
    {
        if (currentPhase == IDLE)
        {

            if (profileDropdownOption == 0)
            {
                setProfile(leaded);
            }
            else if (profileDropdownOption == 1)
            {
                setProfile(leadFree);
            }
            else if (profileDropdownOption == 2)
            {
                setProfile(temper);
            }
            else
            {
                char buf[20];
                lv_dropdown_get_selected_str(dropdownList, buf, 20);
                getProfileFromJson(buf);
                setProfile(custom);
            }
            buzzMultipleTimes(profileDropdownOption + 1);
            updateTableContent(currentProfile.preheatTime, currentProfile.preheatTemperature, currentProfile.soakTime, currentProfile.soakTemperature, currentProfile.reflowTime, currentProfile.reflowTemperature);
            dataPointDuration = ((currentProfile.preheatTime + currentProfile.soakTime + currentProfile.reflowTime) / dataPoints);
            //Serial.println(dataPointDuration);
        }
        else
        {

            buzzAlarm();
            Serial.println("CANT CHANGE PROFILE WHILE NOT IN IDLE!");
        }
    }
}

void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

bool touchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
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
    if (touchX > LV_HOR_RES_MAX || touchY > LV_VER_RES_MAX)
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

void setCallbacks()
{
    lv_obj_set_event_cb(tabview, tabviewEventCallback);
    lv_obj_set_event_cb(startButton, startEvent);
    lv_obj_set_event_cb(loadButton, loaderEvent);
    lv_obj_set_event_cb(buttonMatrix, buttonMatrixEvent);
    lv_obj_set_event_cb(dropdownList, profileHandler);
}

void initDisplay()
{
    lv_disp_buf_init(&displayBuffer, buf, NULL, LV_HOR_RES_MAX * 10);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.flush_cb = displayFlush;
    disp_drv.buffer = &displayBuffer;
    lv_disp_drv_register(&disp_drv);
}

void initDriver()
{
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpadRead;
    lv_indev_drv_register(&indev_drv);
    lv_style_init(&st);
    lv_style_set_text_font(&st, LV_STATE_DEFAULT, &lv_font_montserrat_18);
}

void initGui()
{
    lv_init();
    lvThemeInit();
    initDisplay();
    initDriver();
    screenInit();
    createGuiObjects();
    setCallbacks();
}