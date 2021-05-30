#ifndef YARC_WEB_H
#define YARC_WEB_H

#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

extern AsyncWebServer server;
extern const char *ssid;
extern const char *password;
extern const char *PARAM_MESSAGE;

void initWebSocket();
void notifyClients(const char *message);
void notFound(AsyncWebServerRequest *request);

#endif