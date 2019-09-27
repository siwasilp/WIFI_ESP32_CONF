void handleRoot()
{
  if (iotWebConf.handleCaptivePortal())
  {
    return;
  }

  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>WebConf</title></head><body>Web Config Now Value.";
  s += "<ul>";
  s += "<li>SSID_Connect : ";
  s += ipAddressValue;
  s += "</ul>";
  s += "<ul>";
  s += "<li>PASS_Connect : ";
  s += gatewayValue;
  s += "</ul>";
  s += "<ul>";
  s += "<li>MY Token_key : ";
  s += netmaskValue;
  s += "</ul>";
  s += "Go to <a href='config'>configure page</a>";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
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


  return valid;
}

boolean connectAp(const char* apName, const char* password)
{
  // -- Custom AP settings
  return WiFi.softAP(apName, password, 4);
}
void connectWifi(const char* ssid, const char* password)
{


  ipAddress.fromString(String(ipAddressValue));
  netmask.fromString(String(netmaskValue));
  gateway.fromString(String(gatewayValue));

  if (!WiFi.config(ipAddress, gateway, netmask)) {
    Serial.println("STA Failed to configure");
  }
  Serial.print("ip: ");
  Serial.println(ipAddress);
  Serial.print("gw: ");
  Serial.println(gateway);
  Serial.print("net: ");
  Serial.println(netmask);

  WiFi.begin(ssid, password);
}
