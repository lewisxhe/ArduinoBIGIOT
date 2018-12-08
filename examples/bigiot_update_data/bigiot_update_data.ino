#include <ESP8266WiFi.h>
#include "bigiot.h"

const char *host = "www.bigiot.net";
const int port = 8282;

const char *ssid = "your wifi ssid";
const char *password = "your wifi password";

char buf[100];

typedef struct
{
  const char *name;
  const char *key;
  const char *id;
  const char *usrkey;
} bigiot_login_param_t;

const bigiot_login_param_t param = {"remote device name", "your apikey", "your device id", "your usr key"};

BigIOT bigiot;

void setup()
{
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  while (1)
  {
    Serial.print("connecting to ");
    Serial.println(host);

    if (bigiot.connect(host, port))
    {
      if (bigiot.login(param.id, param.key, param.usrkey))
      {
        Serial.println("login success");
        break;
      }
      Serial.println("login fail");
    }
  }
}

void loop()
{
  static uint32_t timestamp = 0;

  int com = bigiot.waiting();
  
  if (millis() - timestamp > 3000)
  {
    sprintf(buf, "%d", rand() % UINT16_MAX);
    String ran = buf;
    BIGIOT_Data_t d = {"your interface id", ran};
    bigiot.update_data_stream(&d, 1);
    timestamp = millis();
  }
}
