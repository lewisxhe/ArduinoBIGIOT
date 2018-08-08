#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <bigiot.h>

const char *host = "www.bigiot.net";
const int port = 8282;

const char *ssid = "your wifi ssid";
const char *password = "your wifi passwd";
const char *privateKey = "your bigiot platfrom apikey";
const char *deviceId = "your bigiot platfrom device id";

BigIOT bigiot;

void setup()
{
  Serial.begin(115200);
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);

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
      if (bigiot.login(deviceId, privateKey))
      {
        Serial.println("login success");
        break;
      }
      Serial.println("login fail");
    }
    delay(1000);
  }
}

void loop()
{
  int com = bigiot.waiting();
  switch (com)
  {
    //lost connected
  case BIGIOT_LOST_CONNECTED:
    Serial.println("lost connected");
    break;
    //off
  case BIGIOT_STOP_COMMAND:
    Serial.println("BIGIOT_STOP_COMMAND - ");
    digitalWrite(LED_BUILTIN, 1);
    break;
    //on
  case BIGIOT_PLAY_COMMAND:
    Serial.println("BIGIOT_PLAY_COMMAND + ");
    digitalWrite(LED_BUILTIN, 0);
    break;
  default:
    break;
  }

  delay(500);
}
