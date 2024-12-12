#include "html.h"
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

#define RX_PIN 12
#define TX_PIN 13
#define RTS_PIN 14
#define LED_PIN 2
#define RESET_PIN 0

String ap_ssid;
ESP8266WebServer httpServer(80);
ModbusMaster node;
SoftwareSerial RS485Serial(RX_PIN, TX_PIN);

const long fetchInterval = 1000;
const long resetInterval = 5000;

bool resetPending = false;
bool wifiConfigured = false;

struct SensorData {
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

void preTransmission() {
  digitalWrite(RTS_PIN, HIGH);
}

void postTransmission() {
  digitalWrite(RTS_PIN, LOW);
}

bool fetchData() {
  static unsigned long lastFetchTime = 0;
  if (millis() - lastFetchTime < fetchInterval)
    return false;

  lastFetchTime = millis();
  uint8_t result = node.readHoldingRegisters(0x0000, 0x1F);
  if (result != node.ku8MBSuccess)
    return false;

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

void resetWiFi() {
  static unsigned long buttonPressTime = 0;
  if (digitalRead(RESET_PIN) == LOW) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis();
    } else if (millis() - buttonPressTime >= resetInterval && !resetPending) {
      resetPending = true;
      for (int i = 0; i < 128; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      wifiConfigured = false;
      Serial.println("\nWiFi settings reset! Rebooting...");
      for (int i = 0; i < 5; ++i) {
        digitalWrite(LED_PIN, LOW);
        delay(500);
        digitalWrite(LED_PIN, HIGH);
        delay(500);
      }
      ESP.restart();
    }
  } else {
    buttonPressTime = 0;
    resetPending = false;
  }
}

String trimString(const String &str) {
  int start = 0;
  int end = str.length() - 1;
  while (start <= end && isspace(str[start]))
    start++;
  while (end >= start && isspace(str[end]))
    end--;
  return str.substring(start, end + 1);
}

String readEEPROMString(int startAddr, int length) {
  String str;
  for (int i = 0; i < length; ++i) {
    char c = EEPROM.read(startAddr + i);
    if (c != 0)
      str += c;
  }
  return str;
}

void configureWiFi() {
  String ssid = readEEPROMString(0, 32);
  String password = readEEPROMString(32, 32);
  bool isStaticIP = EEPROM.read(64);
  if (ssid.length() > 0) {
    if (isStaticIP) {
      IPAddress address, gateway, netmask;
      address.fromString(readEEPROMString(65, 32));
      gateway.fromString(readEEPROMString(97, 32));
      netmask.fromString(readEEPROMString(129, 32));
      WiFi.config(address, gateway, netmask);
    }
    if (password.length() > 0) {
      WiFi.begin(ssid.c_str(), password.c_str());
    } else {
      WiFi.begin(ssid.c_str());
    }
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      resetWiFi();
      Serial.print(".");
    }
    Serial.println("\nConnected to " + ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    wifiConfigured = true;
  } else {
    WiFi.softAP(ap_ssid);
    Serial.println("\nAccess Point started!");
  }
}

void saveWiFiSettings() {
  String ssid = trimString(httpServer.arg("ssid"));
  String password = trimString(httpServer.arg("password"));
  String ipconfig = httpServer.arg("ipconfig");

  for (int i = 0; i < 32; ++i) {
    EEPROM.write(i, ssid[i]);
    EEPROM.write(32 + i, password[i]);
  }
  EEPROM.write(64, ipconfig == "static" ? 1 : 0);

  if (ipconfig == "static") {
    String address = trimString(httpServer.arg("address"));
    String gateway = trimString(httpServer.arg("gateway"));
    String netmask = trimString(httpServer.arg("netmask"));
    for (int i = 0; i < 32; ++i) {
      EEPROM.write(65 + i, address[i]);
      EEPROM.write(97 + i, gateway[i]);
      EEPROM.write(129 + i, netmask[i]);
    }
  }
  EEPROM.commit();
  wifiConfigured = true;
  httpServer.send(200, "text/html", "<h1>WiFi settings saved! Rebooting...</h1>");
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(RTS_PIN, OUTPUT);
  digitalWrite(RTS_PIN, LOW);
  pinMode(RESET_PIN, INPUT_PULLUP);

  RS485Serial.begin(9600);
  node.begin(0x01, RS485Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  EEPROM.begin(512);
  configureWiFi();

  httpServer.on("/", HTTP_GET, []() {
    String html = wifiConfigured ? MAIN_page : WiFi_page;
    html.replace("%CSS_STYLES%", CSS_styles);
    httpServer.send(200, "text/html", html);
  });

  httpServer.on("/data", HTTP_GET, []() {
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
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  });

  httpServer.on("/save", HTTP_POST, saveWiFiSettings);

  httpServer.begin();
}

void loop() {
  httpServer.handleClient();
  fetchData();
  resetWiFi();
}