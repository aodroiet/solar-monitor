#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// #include <ESP8266HTTPUpdateServer.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "html.h"

#define RTS_PIN D5
#define RX_PIN D6
#define TX_PIN D7

String ap_ssid;
bool wifiConfigured = false;

unsigned long previousMillis = 0;
const long interval = 1000;

ESP8266WebServer httpServer(80);
// ESP8266HTTPUpdateServer httpUpdater;

ModbusMaster node;
SoftwareSerial RS485Serial(RX_PIN, TX_PIN);

struct SensorData
{
  uint16_t Inverter_State;
  float PV_Voltage;
  float PV_Current;
  float PV_Power;
  float Bus_Voltage;
  float AC_Voltage;
  float AC_Current;
  float Grid_Frequency;
  float Active_Power;
  float Reactive_Power;
  float Daily_Production;
  uint32_t Total_Production;
  uint8_t Temperature_Module;
  uint8_t Temperature_Inverter;
  uint32_t Total_Running_Hour;
} sensorData;

void preTransmission()
{
  digitalWrite(RTS_PIN, HIGH);
}

void postTransmission()
{
  digitalWrite(RTS_PIN, LOW);
}

bool fetchData()
{
  uint8_t result = node.readHoldingRegisters(0x0000, 0x1F);
  if (result == node.ku8MBSuccess)
  {
    sensorData.Inverter_State = node.getResponseBuffer(0x00);
    sensorData.PV_Voltage = node.getResponseBuffer(0x06) * 0.1;
    sensorData.PV_Current = node.getResponseBuffer(0x07) * 0.01;
    sensorData.PV_Power = node.getResponseBuffer(0x0A) * 0.01;
    sensorData.Bus_Voltage = node.getResponseBuffer(0x1D) * 0.1;
    sensorData.AC_Voltage = node.getResponseBuffer(0x0F) * 0.1;
    sensorData.AC_Current = node.getResponseBuffer(0x10) * 0.01;
    sensorData.Grid_Frequency = node.getResponseBuffer(0x0E) * 0.01;
    sensorData.Active_Power = node.getResponseBuffer(0x0C) * 0.01;
    sensorData.Reactive_Power = node.getResponseBuffer(0x0D) * 0.01;
    sensorData.Daily_Production = node.getResponseBuffer(0x19) * 0.01;
    sensorData.Total_Production = (node.getResponseBuffer(0x15) << 16) + node.getResponseBuffer(0x16);
    sensorData.Temperature_Module = node.getResponseBuffer(0x1B);
    sensorData.Temperature_Inverter = node.getResponseBuffer(0x1C);
    sensorData.Total_Running_Hour = (node.getResponseBuffer(0x17) << 16) + node.getResponseBuffer(0x18);
    return true;
  }
  return false;
}

void handleReset()
{
  EEPROM.begin(512);
  for (int i = 0; i < 128; ++i)
    EEPROM.write(i, 0);
  EEPROM.commit();
  wifiConfigured = false;
  httpServer.send(200, "text/html", "<h1>WiFi settings reset! Rebooting...</h1>");
  delay(1000);
  ESP.restart();
}

String css(const String &var)
{
  if (var == "CSS_STYLES")
  {
    return CSS_styles;
  }
  return String();
}

String readStringFromEEPROM(int startAddr)
{
  String str = "";
  for (int i = 0; i < 32; ++i)
  {
    char c = EEPROM.read(startAddr + i);
    if (c != 0)
      str += c;
  }
  return str;
}

void setup(void)
{
  Serial.begin(9600);
  RS485Serial.begin(9600);

  pinMode(RTS_PIN, OUTPUT);
  digitalWrite(RTS_PIN, LOW);

  node.begin(0x01, RS485Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  EEPROM.begin(512);
  char stored_ssid[32];
  char stored_password[32];
  for (int i = 0; i < 32; ++i)
  {
    stored_ssid[i] = EEPROM.read(i);
    stored_password[i] = EEPROM.read(32 + i);
  }
  bool isStaticIP = EEPROM.read(64);
  if (strlen(stored_ssid) > 0 && strlen(stored_password) > 0)
  {
    if (isStaticIP)
    {
      IPAddress static_ip, gateway, subnet;
      static_ip.fromString(readStringFromEEPROM(65));
      gateway.fromString(readStringFromEEPROM(97));
      subnet.fromString(readStringFromEEPROM(129));
      WiFi.config(static_ip, gateway, subnet);
    }
    WiFi.begin(stored_ssid, stored_password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected to ");
    Serial.println(stored_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    wifiConfigured = true;
  }
  else
  {
    WiFi.softAP(ap_ssid);
    Serial.println("Access Point started!");
  }

  httpServer.on("/", HTTP_GET, []()
                {
                  String html = wifiConfigured ? MAIN_page : WiFi_page;
                  html.replace("%CSS_STYLES%", css("CSS_STYLES"));
                  httpServer.send(200, "text/html", html);
                  //
                });

  httpServer.on("/data", HTTP_GET, []()
                {
                  String jsonResponse = "{";
                  jsonResponse += "\"Inverter_State\":" + String(sensorData.Inverter_State) + ",";
                  jsonResponse += "\"PV_Voltage\":" + String(sensorData.PV_Voltage) + ",";
                  jsonResponse += "\"PV_Current\":" + String(sensorData.PV_Current) + ",";
                  jsonResponse += "\"PV_Power\":" + String(sensorData.PV_Power) + ",";
                  jsonResponse += "\"Bus_Voltage\":" + String(sensorData.Bus_Voltage) + ",";
                  jsonResponse += "\"AC_Voltage\":" + String(sensorData.AC_Voltage) + ",";
                  jsonResponse += "\"AC_Current\":" + String(sensorData.AC_Current) + ",";
                  jsonResponse += "\"Grid_Frequency\":" + String(sensorData.Grid_Frequency) + ",";
                  jsonResponse += "\"Active_Power\":" + String(sensorData.Active_Power) + ",";
                  jsonResponse += "\"Reactive_Power\":" + String(sensorData.Reactive_Power) + ",";
                  jsonResponse += "\"Daily_Production\":" + String(sensorData.Daily_Production) + ",";
                  jsonResponse += "\"Total_Production\":" + String(sensorData.Total_Production) + ",";
                  jsonResponse += "\"Temperature_Module\":" + String(sensorData.Temperature_Module) + ",";
                  jsonResponse += "\"Temperature_Inverter\":" + String(sensorData.Temperature_Inverter) + ",";
                  jsonResponse += "\"Total_Running_Hour\":" + String(sensorData.Total_Running_Hour) + ",";
                  jsonResponse += "\"RSSI\":" + String(WiFi.RSSI());
                  jsonResponse += "}";
                  httpServer.send(200, "application/json", jsonResponse);
                  // httpServer.sendHeader("Access-Control-Allow-Origin", "*");
                });

  httpServer.on("/save", HTTP_POST, []()
                {
                  EEPROM.begin(512);
                  String ssid = httpServer.arg("ssid");
                  String password = httpServer.arg("password");
                  String ipconfig = httpServer.arg("ipconfig");
                  for (int i = 0; i < 32; ++i)
                    EEPROM.write(i, ssid[i]);
                  for (int i = 0; i < 32; ++i)
                    EEPROM.write(32 + i, password[i]);
                  EEPROM.write(64, ipconfig == "static" ? 1 : 0);
                  if (ipconfig == "static")
                  {
                    String static_ip = httpServer.arg("address");
                    String gateway = httpServer.arg("gateway");
                    String subnet = httpServer.arg("netmask");
                    for (int i = 0; i < 32; ++i)
                      EEPROM.write(65 + i, static_ip[i]);
                    for (int i = 0; i < 32; ++i)
                      EEPROM.write(97 + i, gateway[i]);
                    for (int i = 0; i < 32; ++i)
                      EEPROM.write(129 + i, subnet[i]);
                  }
                  EEPROM.commit();
                  wifiConfigured = true;
                  httpServer.send(200, "text/html", "<h1>WiFi settings saved! Rebooting...</h1>");
                  delay(1000);
                  ESP.restart();
                  //
                });

  httpServer.on("/reset", HTTP_POST, handleReset);
  // httpUpdater.setup(&httpServer, "/update", update_username, update_password);
  httpServer.begin();
  Serial.println("HTTP server started");
}

void loop(void)
{
  httpServer.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    fetchData();
  }
}