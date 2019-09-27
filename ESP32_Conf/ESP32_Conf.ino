#include <IotWebConf.h>
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
//==========================================================================
#define CONFIG_VERSION               "PK007_620927"
const char thingName[]             = "ESP32_CONFIG";
const char wifiInitialApPassword[] = "12345678";
#define CONFIG_PIN 22
#define STATUS_PIN 4
//=========================================================================
#define STRING_LEN 128
#define NUMBER_LEN 32
void configSaved();
boolean formValidator();
boolean connectAp(const char* apName, const char* password);
void connectWifi(const char* ssid, const char* password);
DNSServer dnsServer;
WebServer server(80);
char ipAddressValue[STRING_LEN];
char gatewayValue[STRING_LEN];
char netmaskValue[STRING_LEN];
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfParameter ipAddressParam = IotWebConfParameter("SSID_Connect", "ipAddress", ipAddressValue, STRING_LEN, "text", NULL, "192.168.3.222");
IotWebConfParameter gatewayParam   = IotWebConfParameter("PASS_Connect", "gateway", gatewayValue, STRING_LEN, "text", NULL, "192.168.3.0");
IotWebConfParameter netmaskParam   = IotWebConfParameter("Token_key", "netmask", netmaskValue, STRING_LEN, "text", NULL, "255.255.255.0");
IPAddress ipAddress;
IPAddress gateway;
IPAddress netmask;
int sta = 0;

boolean needMqttConnect = false;
boolean needReset = false;
int pinState = HIGH;
unsigned long lastReport = 0;
unsigned long lastMqttConnectionAttempt = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  pinMode(STATUS_PIN, OUTPUT);
  for (int i = 0 ; i < 10 ; i++) {
    digitalWrite(4, LOW);
    delay(200);
    digitalWrite(4, HIGH);
    delay(200);
    if (digitalRead(CONFIG_PIN) == 0)sta = 0;
    else sta = 1;
  }

  if (sta == 0) {
    digitalWrite(STATUS_PIN, LOW);
    Serial.println("SETUP");
    Serial.println();
    Serial.println("Starting up...");
    iotWebConf.setStatusPin(STATUS_PIN);
    iotWebConf.setConfigPin(CONFIG_PIN);
    iotWebConf.addParameter(&ipAddressParam);
    iotWebConf.addParameter(&gatewayParam);
    iotWebConf.addParameter(&netmaskParam);
    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setFormValidator(&formValidator);
    iotWebConf.setApConnectionHandler(&connectAp);
    iotWebConf.setWifiConnectionHandler(&connectWifi);
    boolean validConfig = iotWebConf.init();
    server.on("/", handleRoot);
    server.on("/config", [] { iotWebConf.handleConfig(); });
    server.onNotFound([]() {
      iotWebConf.handleNotFound();
    });
    Serial.println("Ready.");
  } else {
    Serial.println("RUN");
    sta = 1;

    iotWebConf.doLoop();
    ipAddress.fromString(String(ipAddressValue));
    netmask.fromString(String(netmaskValue));
    gateway.fromString(String(gatewayValue));
    Serial.print("ip: ");
    Serial.println(iotWebConf.getThingName());
    Serial.print("gw: ");
    Serial.println(gateway);
    Serial.print("net: ");
    Serial.println(netmask);
    Serial.print(ipAddressValue);
    Blynk.begin(ipAddressValue, gatewayValue, netmaskValue);
  }

}

void loop()
{
  if (sta == 1) {
    //Serial.println("RUNING");
    Blynk.run();
    digitalWrite(STATUS_PIN, LOW);
    delay(100);
    digitalWrite(STATUS_PIN, HIGH);
    delay(100);
  }
  else {
    // Serial.println("setup..");
    iotWebConf.doLoop();
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
    Serial.print("TEST LED ->");
    Serial.println(pinState == LOW ? "ON" : "OFF");
  }
}
