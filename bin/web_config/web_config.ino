#include <MQTT.h>
#include <IotWebConf.h>

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

//========================================================
#define CONFIG_VERSION               "PK_007 #62.09.26"
const char thingName[]             = "ESP_CONFIG";
const char wifiInitialApPassword[] = "12345678";
//========================================================
char auth[] = "YourAuthToken";


#define STRING_LEN 128
#define CONFIG_PIN 0
#define STATUS_PIN 4

void    wifiConnected();
void    configSaved();
boolean formValidator();

void mqttMessageReceived(String &topic, String &payload);

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
WiFiClient net;
MQTTClient mqttClient;

char mqttServerValue[STRING_LEN];
char tokenNameValue[STRING_LEN];
char wifiSsid[STRING_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfParameter tokenNameParam = IotWebConfParameter("*Token", "token", tokenNameValue, STRING_LEN);
IotWebConfParameter wifiSsidParameter = IotWebConfParameter("*WiFi SSID", "iwcWifiSsid",wifiSsid, STRING_LEN);

boolean needMqttConnect = false;
boolean needReset = false;
int pinState = HIGH;
unsigned long lastReport = 0;
unsigned long lastMqttConnectionAttempt = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...");

  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addParameter(&tokenNameParam);

  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.setupUpdateServer(&httpUpdater);

  // -- Initializing the configuration.
  boolean validConfig = iotWebConf.init();
  if (!validConfig)
  {
    tokenNameValue[0] = '\0';
  }
  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.onNotFound([]() {
    iotWebConf.handleNotFound();
  });
  mqttClient.begin(mqttServerValue, net);
  mqttClient.onMessage(mqttMessageReceived);


  Serial.println("===============================");
  Serial.println(wifiSsid);
  //  Blynk.begin(auth);
}

void loop()
{
  iotWebConf.doLoop();
  mqttClient.loop();

  if (needMqttConnect)
  {
    if (connectMqtt())
    {
      needMqttConnect = false;
    }
  }
  else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected()))
  {
    Serial.println("MQTT reconnect");
    connectMqtt();
  }

  if (needReset)
  {
    Serial.println("Rebooting after 1 second.");
    iotWebConf.delay(1000);
    ESP.restart();
  }

  unsigned long now = millis();
  if ((500 < now - lastReport) && (pinState != digitalRead(CONFIG_PIN)))
  {
    pinState = 1 - pinState; // invert pin state as it is changed
    lastReport = now;
    Serial.print("Sending on MQTT channel '/test/status' :");
    Serial.println(pinState == LOW ? "ON" : "OFF");
    mqttClient.publish("/test/status", pinState == LOW ? "ON" : "OFF");
  }
}

/**
   Handle web requests to "/" path.
*/
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>CONFIG_ESP</title></head><body>MQTT App demo";

  s += "<ul>";
  s += "<li>CONNEC_SSID : ";
  s += wifiSsid;
  s += "</ul>";

  s += "<ul>";
  s += "<li>BLYNK_TOKEN : ";
  s += tokenNameValue;
  s += "</ul>";

  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void wifiConnected()
{
  needMqttConnect = true;
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  needReset = true;
}

boolean formValidator()
{
  Serial.println("Validating form.");
  boolean valid = true;

  int l = server.arg(tokenNameParam.getId()).length();
  if (l < 3)
  {
    tokenNameParam.errorMessage = "Please provide characters!";
    valid = false;
  }

  return valid;
}

boolean connectMqtt() {
  unsigned long now = millis();
  if (1000 > now - lastMqttConnectionAttempt)
  {
    // Do not repeat within 1 sec.
    return false;
  }
  Serial.println("Connecting to MQTT server...");
  if (!connectMqttOptions()) {
    lastMqttConnectionAttempt = now;
    return false;
  }
  Serial.println("Connected!");

  mqttClient.subscribe("/test/action");
  return true;
}


boolean connectMqttOptions()
{
  boolean result;
  if (tokenNameValue[0] != '\0')
  {
    result = mqttClient.connect(iotWebConf.getThingName(), tokenNameValue);
  }
  else
  {
    result = mqttClient.connect(iotWebConf.getThingName());
  }
  return result;
}

void mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("Incoming: " + topic + " - " + payload);
}
