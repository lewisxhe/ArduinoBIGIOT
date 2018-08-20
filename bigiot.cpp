#include "bigiot.h"
#include "MD5.h"

int BigIOT::waiting(void)
{
    int com = 0;
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
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.parseObject(pack);
    if (!root.success())
    {
        return BIGIOT_INVALD_COMMAND;
    }

    String mes = root["M"];
    if (mes == "say")
    {
        String comm = root["C"];
        if (comm == "play")
        {
            return BIGIOT_PLAY_COMMAND;
        }
        else if (comm == "stop")
        {
            return BIGIOT_STOP_COMMAND;
        }
        else if (comm == "offOn")
        {
            return BIGIOT_OFFON_COMMAND;
        }
        else if (comm == "minus")
        {
            return BIGIOT_MINUS_COMMAND;
        }
        else if (comm == "up")
        {
            return BIGIOT_UP_COMMAND;
        }
        else if (comm == "plus")
        {
            return BIGIOT_PLUS_COMMAND;
        }
        else if (comm == "left")
        {
            return BIGIOT_LEFT_COMMAND;
        }
        else if (comm == "pause")
        {
            return BIGIOT_PAUSE_COMMAND;
        }
        else if (comm == "right")
        {
            return BIGIOT_RIGHT_COMMAND;
        }
        else if (comm == "backward")
        {
            return BIGIOT_BACKWARD_COMMAND;
        }
        else if (comm == "down")
        {
            return BIGIOT_DOWN_COMMAND;
        }
        else if (comm == "forward")
        {
            return BIGIOT_FPRWARD_COMMAND;
        }
    }
    return BIGIOT_INVALD_COMMAND;
}

// recv {"M":"WELCOME TO BIGIOT"}
// recv {"M":"token","ID":"7081","K":"650f9f133285037a36ac9bbc6477a0f1"}
int BigIOT::login_parse(String pack)
{
    JsonObject &root = jsonBuffer.parseObject(pack);
    if (!root.success())
        return BIGIOT_INVALD_COMMAND;

    String mes = root["M"];

    if (mes == "WELCOME TO BIGIOT")
    {
        return BIGIOT_LOGINT_WELCOME;
    }
    else if (mes == "checkinok")
    {
        return BIGIOT_LOGINT_CHECK_IN;
    }
    else if (mes == "token" && _usrKey != "")
    {
        String t = root["K"];
        t += _usrKey;
        unsigned char *hash = MD5::make_hash((char *)t.c_str());
        char *key = MD5::make_digest(hash, 16);

        String token = get_login_packet(key);
        print(token);

        free(hash);
        free(key);
        return BIGIOT_LOGINT_TOKEN;
    }
    return 0;
}

bool BigIOT::login(const char *devId, const char *apiKey, const char *userKey)
{
    _dev = devId;
    _key = apiKey;
    _usrKey = userKey;

    print(get_login_packet(_key));
    uint64_t timeout = millis();
    for (;;)
    {
        if (millis() - timeout > 5000)
        {
            print(get_logout_packet());
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

String BigIOT::get_login_packet(String apiKey)
{
    String pack;
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "checkin";
    root["ID"] = _dev;
    root["K"] = apiKey;
    root.printTo(pack);
    pack += "\n";
    return pack;
}

String BigIOT::get_logout_packet(void)
{
    String pack;
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "checkout";
    root["ID"] = _dev;
    root["K"] = _key;
    root.printTo(pack);
    pack += "\n";
    return pack;
}

String BigIOT::get_heatrate_pack(void)
{
    String pack;
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "b";
    root.printTo(pack);
    pack += "\n";
    return pack;
}

//{"M":"alert","C":"xx1","B":"xx2"}\n
void BigIOT::send_alarm_message(alarm_method_t manner, String mes)
{
    if (!connected())
        return;
    String pack;
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "alert";
    root["C"] = mes;
    switch (manner)
    {
    case BIGIOT_EMAIL_M:
        root["B"] = "email";
        break;
    case BIGIOT_QQ_M:
        root["B"] = "qq";
        break;
    case BIGIOT_WEIBO_M:
        root["B"] = "weibo";
        break;
    default:
        return;
    }
    root.printTo(pack);
    pack += "\n";
    print(pack);
}

// {"M":"update","ID":"112","V":{"6":"1","36":"116"}}\n
void BigIOT::update_data_stream(BIGIOT_Data_t *data, int len)
{
    String pack;
    if (!connected())
        return;
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "update";
    root["ID"] = _dev;
    JsonObject &v = root.createNestedObject("V");
    for (int i = 0; i < len; ++i)
    {
        v[data->interfaceID] = data->data;
    }
    root.printTo(pack);
    pack += "\n";
    print(pack);
}

// {"M":"update","ID":"xx1","V":{"id1":"lng,lat",...}}\n
void BigIOT::update_location_data(BIGIOT_location_t *data)
{
    String pack;
    if (!connected())
        return;
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
    root["M"] = "update";
    root["ID"] = _dev;
    JsonObject &v = root.createNestedObject("V");
    v[data->interfaceID] = data->lo + "," + data->la;
    root.printTo(pack);
    pack += "\n";
    print(pack);
}