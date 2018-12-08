#if defined ESP32
#include <WiFi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#else
#error "Only support espressif esp32/8266 chip"
#endif
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
  const char gpio;
} bigiot_login_param_t;

const bigiot_login_param_t param[] =
    {
        {"remote device name", "your apikey", "your device id", "your usr key", LED_BUILTIN},
        {"remote device name", "your apikey", "your device id", "your usr key", LED_BUILTIN},
        {"remote device name", "your apikey", "your device id", "your usr key", LED_BUILTIN},
        {"remote device name", "your apikey", "your device id", "your usr key", LED_BUILTIN}};

#define MAX_CONNECT_COUNT sizeof(param) / sizeof(param[0])

BigIOT bigiot[MAX_CONNECT_COUNT];

void setup()
{
  Serial.begin(115200);
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

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

  for (int i = 0; i < MAX_CONNECT_COUNT; i++)
  {
    while (1)
    {
      Serial.print("connecting to ");
      Serial.println(host);

      if (bigiot[i].connect(host, port))
      {
        if (bigiot[i].login(param[i].id, param[i].key, param[i].usrkey))
        {
          Serial.println("login success");
          break;
        }
        Serial.println("login fail");
      }
    }
    delay(500);
  }
}


void loop()
{
  for (int i = 0; i < MAX_CONNECT_COUNT; i++)
  {
    int com = bigiot[i].waiting();

    //update data demo
    if (i == 0)
    {
      static uint32_t t;
      if (millis() - t > 3000)
      {
        sprintf(buf, "%d", rand() % 180);
        String ran = buf;
        BIGIOT_Data_t d = {"your interface id", ran};
        bigiot[i].update_data_stream(&d, 1);
        t = millis();
      }
    }

    //update loaction demo
    if (i == 1)
    {
      static uint32_t t;
      static float lo = 0;
      if (millis() - t > 60000)
      {
        BIGIOT_location_t d;
        d.interfaceID = "your interface id";
        d.lo = "114";
        sprintf(buf, "%f", 21.0 + lo++);
        d.la = buf;
        bigiot[i].update_location_data(&d);
        t = millis();
      }
    }

    switch (com)
    {
      //lost connected
    case BIGIOT_LOST_CONNECTED:
      Serial.println("lost connected");
      /*
        disconnect handle
        ....
      */
      break;

      //off
    case BIGIOT_STOP_COMMAND:
      Serial.println("BIGIOT_STOP_COMMAND - ");
      digitalWrite(param[i].gpio, 1);
      break;

      //on
    case BIGIOT_PLAY_COMMAND:
      Serial.println("BIGIOT_PLAY_COMMAND + ");
      digitalWrite(param[i].gpio, 0);
      break;

    /*
    case ... other control
    */
    default:
      break;
    }
  }
}
