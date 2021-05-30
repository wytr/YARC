#include "yarcweb.h"

const char *ssid = "YARC";
const char *password = "123456789";
const char *PARAM_MESSAGE = "message";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool webSocketConnected = false;
bool monitoring = false;

void notifyClients(const char *message)
{
    if (webSocketConnected && monitoring)
    {
        ws.textAll(message);
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        data[len] = 0;
        if (strcmp((char *)data, "startMonitoring") == 0)
        {
            monitoring = true;
        }
        else if (strcmp((char *)data, "stopMonitoring") == 0)
        {
            monitoring = false;
        }
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        webSocketConnected = true;
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        webSocketConnected = false;
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}