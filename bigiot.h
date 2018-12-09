
#ifndef __BIGIOT_HPP
#define __BIGIOT_HPP

#include "WiFiClient.h"
#include <ArduinoJson.h>
#include <MD5Builder.h>

#define BIGIOT_LOGINT_WELCOME 1
#define BIGIOT_LOGINT_CHECK_IN 2
#define BIGIOT_LOGINT_TOKEN 3

typedef enum
{
  BIGIOT_LOST_CONNECTED = -1,
  BIGIOT_INVALD_COMMAND = 0,
  BIGIOT_STOP_COMMAND,
  BIGIOT_PLAY_COMMAND,
  BIGIOT_OFFON_COMMAND,
  BIGIOT_MINUS_COMMAND,
  BIGIOT_UP_COMMAND,
  BIGIOT_PLUS_COMMAND,
  BIGIOT_LEFT_COMMAND,
  BIGIOT_PAUSE_COMMAND,
  BIGIOT_RIGHT_COMMAND,
  BIGIOT_BACKWARD_COMMAND,
  BIGIOT_DOWN_COMMAND,
  BIGIOT_FPRWARD_COMMAND,

} BIGIOT_command_t;

typedef enum
{
  BIGIOT_EMAIL_M,
  BIGIOT_QQ_M,
  BIGIOT_WEIBO_M
} alarm_method_t;

typedef struct
{
  String interfaceID;
  String data;
} BIGIOT_Data_t;

typedef struct
{
  String interfaceID;
  String la;
  String lo;
} BIGIOT_location_t;

typedef void (*callbackFunction)(const char *devName, const int c);

class BIGIOT : public WiFiClient
{
public:
  BIGIOT(String host, uint16_t port)
  {
    _host = host;
    _port = port;
  };

  BIGIOT(const char *host, uint16_t port)
  {
    _host = String(host);
    _port = port;
  };

  BIGIOT()
  {
    _host = "www.bigiot.net";
    _port = 8282;
  }
  void attach(callbackFunction func);
  bool login(const char *devId, const char *apiKey, const char *userKey = "", bool reconnect = true);
  int waiting(void);
  void update_data_stream(BIGIOT_Data_t *data, int len);
  void update_location_data(BIGIOT_location_t *data);
  void send_alarm_message(alarm_method_t manner, String mes);

  void updateData(int id, int data);
  void updateData(const char *id, const char *data, int len);
  void updateData(String id, String data);
  void updateData(String id[], String data[]);

  String getDeviceName() const { return _devName; };

private:
  bool loginToBigiot(void);
  int loginParse(String pack);
  int packetParse(String pack);
  String get_heatrate_pack(void);
  String get_login_packet(String apiKey);
  String get_logout_packet(void);

protected:
  MD5Builder md5;
  StaticJsonBuffer<1024> jsonBuffer;
  uint64_t _timeout = 0;
  uint16_t _port;
  String _host;
  String _dev;
  String _key;
  String _usrKey;
  String _token;
  String _devName;
  bool _reconnect;
  callbackFunction _callbackFunc = NULL;
};

#endif /*__BIGIOT_HPP*/