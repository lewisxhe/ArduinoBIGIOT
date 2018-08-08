
#ifndef __BIGIOT_HPP
#define __BIGIOT_HPP

#include "WiFiClient.h"
#include <ArduinoJson.h>

#define BIGIOT_LOGINT_WELCOME 1
#define BIGIOT_LOGINT_CHECK_IN 2

typedef enum
{
  BIGIOT_LOST_CONNECTED = -1,
  BIGIOT_INVALD_COMMAND = 0,
  BIGIOT_STOP_COMMAND,
  BIGIOT_PLAY_COMMAND
} BIGIOT_command_t;

class BigIOT : public WiFiClient
{
public:
  bool login(const char *devId, const char *apiKey);
  int waiting(void);

private:
  int packet_parse(String pack);
  int login_parse(String pack);
  String get_heatrate_pack(void);
  String get_login_packet(const char *devId, const char *apiKey);
  String get_logout_packet(const char *devId, const char *apiKey);

protected:
  StaticJsonDocument<1024> json;
};

#endif /*__BIGIOT_HPP*/