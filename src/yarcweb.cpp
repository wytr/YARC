#include "yarcweb.h"

const char *ssid = "YARC";
const char *password = "123456789";
const char *PARAM_MESSAGE = "message";

AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

String processor(const String &var)
{
    Serial.println(var);
    if (var == "STATE")
    {

        return "STATE";
    }
    else if (var == "TEMPERATURE")
    {
        return "20";
    }
    else if (var == "HUMIDITY")
    {
        return "20";
    }
    else if (var == "PRESSURE")
    {
        return "20";
    }
}
