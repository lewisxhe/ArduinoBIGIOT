/////////////////////////////////////////////////////////////////
/*
  bigiot.cpp - Arduino library simplifies the use of connected BIGIOT platforms.
  Created by Lewis he on January 1, 2019.
*/
//03,10,2020 : Update WiFiClient to Cline , Adapt to all platforms
/////////////////////////////////////////////////////////////////

#include "bigiot.h"
#include <base64.h>
#if defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif
#include <MD5Builder.h>
#include <ArduinoJson.h>

#define ARDUINOJSON_V513   ((ARDUINOJSON_VERSION_MAJOR) == 5 && (ARDUINOJSON_VERSION_MINOR) == 13)
#define ARDUINOJSON_V611   ((ARDUINOJSON_VERSION_MAJOR) == 6 && (ARDUINOJSON_VERSION_MINOR) == 14)

#if ARDUINOJSON_V611
StaticJsonDocument<1024> root;
#elif ARDUINOJSON_V513
StaticJsonBuffer<1024> jsonBuffer;
#else
#error "No support ArduinoJson version ,please use ArduinoJsonV6.14.x or ArduinoJsonV5.13.x"
#endif

/////////////////////////////////////////////////////////////////
BIGIOT::BIGIOT(Client &client)
{
    _client = &client;
    _host = BIGIOT_PLATFORM_HOST;
    _port = BIGIOT_PLATFORM_PORT;
}

/////////////////////////////////////////////////////////////////
const char *BIGIOT::deviceName()
{
    return _devName.c_str();
}

/////////////////////////////////////////////////////////////////
String BIGIOT::deviceName() const
{
    return _devName;
}
/////////////////////////////////////////////////////////////////
void BIGIOT::setHeartFreq(uint32_t f)
{
    _heartFreq = f;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::isOnline()
{
    return _isLogin;
}

/////////////////////////////////////////////////////////////////
int BIGIOT::handle(void)
{
    int com = DISCONNECT;
    static uint64_t hearTimeStamp = millis();
    static uint64_t reconnectTimeStamp = 0;

    if (_client->connected() && _isLogin) {
        if (_client->available()) {
            String pack = _client->readStringUntil('\n');
            com = packetParse(pack);
        }
        if (millis() - hearTimeStamp > _heartFreq) {
            _client->print(BIGIOT_PLATFORM_8282_HATERATE_PACK);
            hearTimeStamp = millis();
        }
    } else {
        _isLogin = false;
        if (_disconnectCallback && !_isCall) {
            _isCall = true;
            _disconnectCallback(*this);
        }
        if (_reconnect && millis() - reconnectTimeStamp > REC_TIMEOUT) {
            DEBUG_BIGIOT_CLIENT("RECONNECT :%s", _devName.c_str());
            reconnectTimeStamp = millis();
            loginToBigiot();
        }
    }
    return com;
}

/////////////////////////////////////////////////////////////////
int BIGIOT::packetParse(String pack)
{
    DEBUG_BIGIOT_CLIENT("%s", pack.c_str());

#if ARDUINOJSON_V611
    root.clear();
    DeserializationError error = deserializeJson(root, pack);
    if (error) {
        DEBUG_BIGIOT_CLIENT("[%d] DeserializationError code:%d \n", __LINE__, error);
        // Serial.println(pack);
        return 0;
    }
#elif ARDUINOJSON_V513
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.parseObject(pack);
    if (!root.success()) {
        return 0;
    }
#endif

    const char *m = (const char *)root["M"];
    const char *salve = (const char *)root["S"];
    if (!strcmp(m, "say")) {
        const char *s = (const char *)root["C"];
        for (int i = 0; i < PLATFORM_ARRAY_SIZE(platform_command); ++i) {
            if (!strcmp(s, platform_command[i])) {
                if (_eventCallback) {
                    _eventCallback(_dev.toInt(), i, platform_command[i], salve);
                }
                return i + 1;
            }
        }
        // const char *salve = (const char *)root["S"];
        if (_eventCallback) {
            _eventCallback(_dev.toInt(), CUSTOM, s, salve);
        }
    }
    /*else if (!strcmp(m, "checkout")) {
        const char *r = (const char *)root["IP"];
        DEBUG_BIGIOT_CLIENT("RECV CHECKOUT:%s\n", r);
    }*/
    return 0;
}

/////////////////////////////////////////////////////////////////
int BIGIOT::loginParse(String pack)
{

#if ARDUINOJSON_V611
    root.clear();
    DeserializationError error = deserializeJson(root, pack);
    if (error) {
        DEBUG_BIGIOT_CLIENT("[%d] DeserializationError code:%d \n", __LINE__, error);
        return INVALID;
    }
#elif ARDUINOJSON_V513
    JsonObject &root = jsonBuffer.parseObject(pack);
    if (!root.success())
        return INVALID;
#endif

    const char *m = (const char *)root["M"];
    if (!strcmp(m, "WELCOME TO BIGIOT")) {
        return BIGIOT_LOGINT_WELCOME;
    } else if (!strcmp(m, "checkinok")) {
        _isCall = false;
        _devName = (const char *)root["NAME"];
        DEBUG_BIGIOT_CLIENT("Checkin OK Device Name:%s\n", _devName.c_str());
        return BIGIOT_LOGINT_CHECK_IN;
    } else if (!strcmp(m, "token") && _usrKey.length()) {
        MD5Builder md5;
        md5.begin();
        md5.add(String((const char *)root["K"]) + _usrKey);
        md5.calculate();
        _token = md5.toString();
        _client->print(getLoginPacket(_token));
        return BIGIOT_LOGINT_TOKEN;
    }
    return INVALID;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::loginToBigiot(void)
{
    if (!_client->connect(_host.c_str(), _port)) {
        DEBUG_BIGIOT_CLIENT("CONNECT HOST FAIL\n");
        return false;
    }
    _client->print(getLoginPacket(_key));
    uint64_t timeStamp = millis();
    for (;;) {
        if (millis() - timeStamp > 5000) {
            DEBUG_BIGIOT_CLIENT("LOGIN_TIMEOUT\n");
            _client->print(getLogoutPacket());
            _client->stop();
            return false;
        }
        if (_client->available()) {
            String line = _client->readStringUntil('\n');
            DEBUG_BIGIOT_CLIENT("RECV:%s\n", line.c_str());
            if (loginParse(line) == BIGIOT_LOGINT_CHECK_IN) {
                _isLogin = true;
                if (_connectCallback) {
                    _connectCallback(*this);
                }
                return true;
            }
        }
        delay(100);
    }
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::login(const char *devId, const char *apiKey, const char *userKey, bool reconnect)
{
    _dev = devId;
    _key = apiKey;
    _usrKey = userKey;
    _reconnect = reconnect;
    int retry = 3;
    do {
        if (loginToBigiot())return true;
    } while (--retry);
    return false;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::operator == (BIGIOT &b)
{
    return (this == &b);
}

/////////////////////////////////////////////////////////////////

void BIGIOT::eventAttach(EventCallbackFunc f)
{
    _eventCallback = f;
}

/////////////////////////////////////////////////////////////////
void BIGIOT::disconnectAttack(GeneralCallbackFunc f)
{
    _disconnectCallback = f;
}

/////////////////////////////////////////////////////////////////
void BIGIOT::connectAttack(GeneralCallbackFunc f)
{
    _connectCallback = f;
}

/////////////////////////////////////////////////////////////////

bool BIGIOT::checkOnline()
{
    static uint64_t checkTimeStamp = 0;

    if (millis() - checkTimeStamp < 15 * 1000) {
        return true;
    }
    checkTimeStamp = millis();

    String pack;
#if ARDUINOJSON_V611
    root.clear();
#elif ARDUINOJSON_V513
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
#endif
    root["M"] = "isOL";
#if ARDUINOJSON_V611
    JsonArray v = root.createNestedArray("ID");
#elif ARDUINOJSON_V513
    JsonArray &v = root.createNestedArray("ID");
#endif

    v.add("D" + _dev);

#if ARDUINOJSON_V611
    serializeJson(root, pack);
#elif ARDUINOJSON_V513
    root.printTo(pack);
#endif
    pack += "\n";
    DEBUG_BIGIOT_CLIENT("SEND CHECK ONLINE COMMAND:%s", pack.c_str());

    _client->print(pack);

    uint64_t start = millis();

    while (1) {
        if (_client->connected() && _isLogin) {
            while (_client->available()) {
                pack = _client->readStringUntil('\n');
                // Serial.println(pack);
#if ARDUINOJSON_V611
                root.clear();
                DeserializationError error = deserializeJson(root, pack);
                if (error) {
                    DEBUG_BIGIOT_CLIENT("[%d] DeserializationError code:%d \n", __LINE__, error);
                    return false;
                }
#elif ARDUINOJSON_V513
                JsonObject &root = jsonBuffer.parseObject(pack);
                if (!root.success())
                    return false;
#endif
                String id = "D" + _dev;
                if (root["R"][id] == String("1")) {
                    DEBUG_BIGIOT_CLIENT("is Online ...");
                    return true;
                }
                DEBUG_BIGIOT_CLIENT("is No Online ...");
                _client->stop();
                _isLogin = false;
                return false;
            }
            if (millis() - start > 5000) {
                DEBUG_BIGIOT_CLIENT("[Timeout] is No Online ...\n");
                if (_client->connected()) {
                    Serial.println("Client is connect ...\n");
                }
                _client->stop();
                _isLogin = false;
                return false;
            }
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////
String BIGIOT::getLoginPacket(String apiKey)
{
    String pack;
#if ARDUINOJSON_V611
    root.clear();
#elif ARDUINOJSON_V513
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
#endif
    root["M"] = "checkin";
    root["ID"] = _dev;
    root["K"] = apiKey;

#if ARDUINOJSON_V611
    serializeJson(root, pack);
#elif ARDUINOJSON_V513
    root.printTo(pack);
#endif
    pack += "\n";
    DEBUG_BIGIOT_CLIENT("SEND LOGIN COMMAND:%s", pack.c_str());
    return pack;
}

/////////////////////////////////////////////////////////////////
String BIGIOT::getLogoutPacket(void)
{
    String pack;

#if ARDUINOJSON_V611
    root.clear();
#elif ARDUINOJSON_V513
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
#endif

    root["M"] = "checkout";
    root["ID"] = _dev;
    if (_token.length())
        root["K"] = _token;
    else
        root["K"] = _key;
#if ARDUINOJSON_V611
    serializeJson(root, pack);
#elif ARDUINOJSON_V513
    root.printTo(pack);
#endif
    pack   += "\n";
    DEBUG_BIGIOT_CLIENT("SEND CHECKOUT COMMAND:%s", pack.c_str());
    return pack;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::sendAlarm(String method, String message)
{
    return sendAlarm(method.c_str(), message.c_str());
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::sendAlarm(const char *method, const char *message)
{
    static uint64_t last_send_time = 0;

    if (!_isLogin || !method || !message)return false;

    if (last_send_time && millis() -  last_send_time < BIGIOT_PLATFORM_ALARM_INTERVAL)
        return false;

    if (!strcmp(method, "email") ||
            !strcmp(method, "qq") ||
            !strcmp(method, "weibo")) {
#if ARDUINOJSON_V611
        root.clear();
#elif ARDUINOJSON_V513
        jsonBuffer.clear();
        JsonObject &root = jsonBuffer.createObject();
#endif



        root["M"] = "alert";
        root["C"] = message;
        root["B"] = method;
        String json;

#if ARDUINOJSON_V611
        serializeJson(root, json);
#elif ARDUINOJSON_V513
        root.printTo(json);
#endif
        json += "\n";
        _client->print(json);
        DEBUG_BIGIOT_CLIENT("Send:%s", json);
        last_send_time = millis();
        return true;
    }
    return  false;
}

/////////////////////////////////////////////////////////////////
// Note:需要在HTTP Header中增加API-Key来授权写入操作,  支持一次传送一幅图像数据；
// Note:目前限定相邻图像数据上传间隔须大于等于10s；
// Note:图片大小不大于100K；
// Note:图片格式jpg、png、gif。
bool BIGIOT::uploadPhoto( const char *id, const char *type, const char *filename, uint8_t *image, size_t size)
{
    char buff[512];
    char status[64] = {0};

    if (!id || !type || !image || !size || !filename || size > 100 * 1024)return false;


    if (strcmp(type, "jpeg") &&
            strcmp(type, "jpg")  &&
            strcmp(type, "png")  &&
            strcmp(type, "gif")) {
        DEBUG_BIGIOT_CLIENT("Error data type\n");
        return false;
    }

    WiFiClientSecure client;

    if (!client.connect(BIGIOT_PLATFORM_HOST, BIGIOT_PLATFORM_HTTPS_PORT)) {
        DEBUG_BIGIOT_CLIENT("Connection failed");
        return false;
    }
    const char *request_content = "--------------------------ef73a32d43e7f04d\r\n"
                                  "Content-Disposition: form-data; name=\"data\"; filename=\"%s.%s\"\r\n"
                                  "Content-Type: image/%s\r\n\r\n";

    const char *request_end = "\r\n--------------------------ef73a32d43e7f04d--\r\n";

    snprintf(buff, sizeof(buff), request_content, filename, type, type);

    uint32_t content_len = strlen(request_end) + strlen(buff) + size;
    String request = "POST /pubapi/uploadImg/did/";
    request += _dev;
    request += "/inputid/";
    request += id;
    request += " HTTP/1.1\r\n";
    request += "Host: ";
    request += BIGIOT_PLATFORM_HOST;
    request += "\r\n";
    request += "API-KEY: ";
    request += _key;
    request += "\r\n";
    request += "Content-Length: " + String(content_len) + "\r\n";
    request += "Content-Type: multipart/form-data; boundary=------------------------ef73a32d43e7f04d\r\n";
    request += "Expect: 100-continue\r\n";
    request += "\r\n";

    client.println(request);
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 100 Continue") != 0) {
        DEBUG_BIGIOT_CLIENT("Unexpected response: %s\n", status);
        client.stop();
        return false;
    }

    client.print(buff);
    content_len = size;
    size_t offset = 0;
    size_t ret = 0;
    while (1) {
        ret = client.write(image + offset, content_len);
        offset += ret;
        content_len -= ret;
        if (size == offset) {
            break;
        }
        delay(1);
    }
    client.print(request_end);
    client.find("\r\n");
    bzero(status, sizeof(status));
    client.readBytesUntil('\r', status, sizeof(status));
    if (strncmp(status, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK"))) {
        DEBUG_BIGIOT_CLIENT("Unexpected response: %s\n", status);
        client.stop();
        return false;
    }

    if (!client.find("\r\n\r\n")) {
        DEBUG_BIGIOT_CLIENT("Invalid response\n");
        client.stop();
        return false;
    }

    request = client.readStringUntil('\n');
    client.stop();

    char *str = strdup(request.c_str());
    if (!str) {
        return false;
    }
    char *start = strchr(str, '{');

#if ARDUINOJSON_V611
    root.clear();
    DeserializationError error = deserializeJson(root, start);
    if (error) {
        free(str);
        return false;
    }
#elif ARDUINOJSON_V513
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.parseObject(start);
    if (!root.success()) {
        free(str);
        return false;
    }
#endif

    bool retVal = !strcmp((const char *)root["R"], "0") ? false : true;
    free(str);
    return retVal;
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::upload(String id, String data)
{
    return  upload(id.c_str(), data.c_str());
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::upload(const char *id, const char *data)
{
    return upload(&id, &data, 1);
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::upload(const char *id[], const char *data[], int len)
{
    if (!_isLogin || !data || !len)return false;

#if ARDUINOJSON_V611
    root.clear();
#elif ARDUINOJSON_V513
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
#endif

    root["M"] = "update";
    root["ID"] = _dev;

#if ARDUINOJSON_V611
    JsonObject v = root.createNestedObject("V");
#elif ARDUINOJSON_V513
    JsonObject &v = root.createNestedObject("V");
#endif
    for (int i = 0; i < len; ++i) {
        v[id[i]] = data[i];
    }
    String json;

#if ARDUINOJSON_V611
    serializeJson(root, json);
#elif ARDUINOJSON_V513
    root.printTo(json);
#endif
    json += "\n";
    _client->print(json);
    DEBUG_BIGIOT_CLIENT("Send:%s", json);
    return true;
}

/////////////////////////////////////////////////////////////////
bool location(String id, String longitude, String latitude)
{
    return location(id.c_str(), longitude.c_str(), latitude.c_str());
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::location(const char *id, float longitude, float latitude)
{
    char longitudeBf[16], latitudeBf[16];
    if (!id)return false;
    snprintf(longitudeBf, sizeof(longitudeBf), "%f", longitude);
    snprintf(latitudeBf, sizeof(latitudeBf), "%f", latitude);
    return location(id, longitudeBf, latitudeBf);
}

/////////////////////////////////////////////////////////////////
bool BIGIOT::location(const char *id, const char *longitude, const char *latitude)
{
    char buff[128];
    if (!_isLogin)return false;

#if ARDUINOJSON_V611
    root.clear();
#elif ARDUINOJSON_V513
    jsonBuffer.clear();
    JsonObject &root = jsonBuffer.createObject();
#endif
    root["M"] = "update";
    root["ID"] = _dev;

#if ARDUINOJSON_V611
    JsonObject v = root.createNestedObject("V");
#elif ARDUINOJSON_V513
    JsonObject &v = root.createNestedObject("V");
#endif

    snprintf(buff, sizeof(buff), "%s,%s", longitude, latitude);
    v[id] = buff;
    String json;

#if ARDUINOJSON_V611
    serializeJson(root, json);
#elif ARDUINOJSON_V513
    root.printTo(json);
#endif
    json += "\n";
    _client->print(json);
    DEBUG_BIGIOT_CLIENT("Send:%s", json);
    return true;
}

/////////////////////////////////////////////////////////////////
// xEmail::xEmail(Client &client)
// {
//     _client = &client;
// }

/////////////////////////////////////////////////////////////////
void xEmail::setEmailHost(const char *host, uint16_t port)
{
    _emailHost = host;
    _emailPort = port;
}

/////////////////////////////////////////////////////////////////
void xEmail::setRecipient(const char *email)
{
    _recipient = email;
}

/////////////////////////////////////////////////////////////////
bool xEmail::setSender(const char *user, const char *password)
{
    _emailUser = user;
    _emailPasswd = password;

    _base64User = base64::encode(_emailUser);
    _base64Pass = base64::encode(_emailPasswd);
    if (_base64User == "-FAIL-" || _base64Pass == "-FAIL-") {
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////
bool xEmail::sendEmail(const char *subject, const char *content)
{
    String ip = WiFiClient::localIP().toString();
    struct {
        int cnt;
        int skip;
        const char *prefix;
        const char *arg;
    }   stream[] = {
        {1, 0, "EHLO %s", ip.c_str()},
        {0, 0, "auth login",  NULL},
        {2, 0, NULL,  _base64User.c_str()},
        {2, 0, NULL,  _base64Pass.c_str()},
        {1, 0, "MAIL From: <%s>",  _emailUser.c_str()},
        {1, 0, "RCPT To: <%s>", _recipient.c_str()},
        {0, 0, "DATA",  NULL},
        {1, 1, "To: <%s>",  _recipient.c_str()},
        {1, 1, "From: <%s>",  _emailUser.c_str()},
        {1, 1, "Subject: %s\r\n",  subject},
        {2, 1, NULL,  content},
        {0, 0, ".",  NULL},
        {0, 0, "QUIT", NULL}
    };

    char buff[256];
    if (!connect(_emailHost.c_str(), _emailPort))
        return false;
    if (!emailRecv())
        return false;

    for (int i = 0; i < PLATFORM_ARRAY_SIZE(stream); ++i) {
        switch (stream[i].cnt) {
        case 0:
            DEBUG_BIGIOT_CLIENT("%s\n", stream[i].prefix);
            println(stream[i].prefix);
            break;
        case 1:
            snprintf(buff, sizeof(buff), stream[i].prefix, stream[i].arg);
            DEBUG_BIGIOT_CLIENT("%s\n", buff);
            println(buff);
            break;
        case 2:
            DEBUG_BIGIOT_CLIENT("%s\n", stream[i].arg);
            println(stream[i].arg);
            break;
        }
        if (!stream[i].skip) {
            if (!emailRecv())
                return false;
        }
    }
    stop();
    return true;
}

/////////////////////////////////////////////////////////////////
bool xEmail::sendEmail(String &subject, String &content)
{
    return sendEmail(subject.c_str(), content.c_str());
}

/////////////////////////////////////////////////////////////////
bool xEmail::emailRecv()
{
    uint8_t code;
    int loopCount = 0;
    while (!available()) {
        delay(1);
        loopCount++;
        if (loopCount > 10000) {
            stop();
            DEBUG_BIGIOT_CLIENT("\r\nxEamil Timeout\n");
            return 0;
        }
    }
    code = peek();
    // Serial.print("MailRecv:");
    while (available()) {
        DEBUG_BIGIOT_WRITE(read());
    }
    if (code >= '4') {
        emailFail();
        return 0;
    }
    return 1;
}

/////////////////////////////////////////////////////////////////
void xEmail::emailFail()
{
    int loopCount = 0;
    println(F("QUIT"));
    while (!available()) {
        delay(1);
        loopCount++;
        if (loopCount > EMAIL_RECV_TIMEOUT) {
            stop();
            DEBUG_BIGIOT_CLIENT("\r\nxEmail Timeout\n");
            return;
        }
    }
    DEBUG_BIGIOT_CLIENT("MailRecv:");
    while (available()) {
        DEBUG_BIGIOT_WRITE(read());
    }
    stop();
}

/////////////////////////////////////////////////////////////////
void ServerChan::setSCKEY(String &key)
{
    _sckey = key;
}

/////////////////////////////////////////////////////////////////
void ServerChan::setSCKEY(const char *key)
{
    _sckey = key;
}

/////////////////////////////////////////////////////////////////
bool ServerChan::sendWechat(String text, String desp)
{
    return sendWechat(text.c_str(), desp.c_str());
}

/////////////////////////////////////////////////////////////////
bool ServerChan::sendWechat(const char *text, const char *desp)
{
    size_t size = strlen(text) + _sckey.length() + strlen(SERVERCHAN_LINK_FORMAT);
    if (desp) {
        size = strlen(desp) > SERVERCHAN_DESP_MAX_LENGTH ? SERVERCHAN_DESP_MAX_LENGTH : size + strlen(desp);
    }
    char *buff = NULL;
    buff = (char *)malloc(size);
    if (!buff) {
        return false;
    }

    snprintf(buff, size, SERVERCHAN_LINK_FORMAT, _sckey.c_str(), text);
    if (desp) {
        snprintf(buff, size, "%s&desp=%s", buff, desp);
    }

    for (char *p = buff; *p; p++) {
        if (*p == ' ')
            *p = '+';
    }

    DEBUG_BIGIOT_CLIENT("ServerChan Request:%s", buff);

    HTTPClient http;
#if defined(ESP32)
    http.begin(buff);
#elif defined(ESP8266)
    WiFiClient client;
    http.begin(client, String(buff));
#endif
    int err = http.GET();
    DEBUG_BIGIOT_CLIENT("[HTTP] GET.code: %d\n", err);
    http.end();
    free(buff);
    return err == HTTP_CODE_OK;
}

