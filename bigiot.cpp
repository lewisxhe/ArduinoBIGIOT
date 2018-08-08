#include "bigiot.h"


int BigIOT::waiting(void)
{
    int com = 0;
    static uint64_t timeout = 0;
    if (connected())
    {
        if (WiFiClient::available())
        {
            String pack = readStringUntil('\n');
            com = packet_parse(pack);
        }
        if (millis() - timeout > 30000)
        {
            print(get_heatrate_pack());
            timeout = millis();
        }
    }
    else
    {
        com = -1;
    }
    return com;
}

int BigIOT::packet_parse(String pack)
{
    DynamicJsonDocument doc(pack.length());
    if (deserializeJson(doc, pack))
        return BIGIOT_INVALD_COMMAND;
    JsonObject root = doc.as<JsonObject>();
    if (root["M"] == "say")
    {
        if (root["C"] == "play")
        {
            return BIGIOT_PLAY_COMMAND;
        }
        else if (root["C"] == "stop")
        {
            return BIGIOT_STOP_COMMAND;
        }
    }
    return BIGIOT_INVALD_COMMAND;
}

int BigIOT::login_parse(String pack)
{
    DynamicJsonDocument doc(pack.length());
    if (deserializeJson(doc, pack))
        return false;
    JsonObject root = doc.as<JsonObject>();

    if (root["M"] == "WELCOME TO BIGIOT")
    {
        return BIGIOT_LOGINT_WELCOME;
    }
    else if (root["M"] == "checkinok")
    {
        return BIGIOT_LOGINT_CHECK_IN;
    }
    return 0;
}

String BigIOT::get_login_packet(const char *devId, const char *apiKey)
{
    String pack;
    JsonObject root = json.to<JsonObject>();
    root["M"] = "checkin";
    root["ID"] = devId;
    root["K"] = apiKey;
    serializeJson(json, pack);
    pack += "\n";
    return pack;
}

String BigIOT::get_logout_packet(const char *devId, const char *apiKey)
{
    String pack;
    JsonObject root = json.to<JsonObject>();
    root["M"] = "checkout";
    root["ID"] = devId;
    root["K"] = apiKey;
    serializeJson(json, pack);
    pack += "\n";
    return pack;
}

String BigIOT::get_heatrate_pack(void)
{
    String pack;
    JsonObject root = json.to<JsonObject>();
    root["M"] = "b";
    serializeJson(json, pack);
    pack += "\n";
    return pack;
}

bool BigIOT::login(const char *devId, const char *apiKey)
{
    print(get_login_packet(devId, apiKey));
    uint64_t timeout = millis();
    for (;;)
    {
        if (millis() - timeout > 5000)
        {
            print(get_logout_packet(devId, apiKey));
            stop();
            return false;
        }
        if (WiFiClient::available())
        {
            String line = readStringUntil('\n');
            if (login_parse(line) == BIGIOT_LOGINT_CHECK_IN)
            {
                return true;
            }
        }
        delay(100);
    }
}
