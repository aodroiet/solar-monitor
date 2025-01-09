#include "html.h"
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

#define MAX485_RO_PIN 13    // RX
#define MAX485_DI_PIN 14    // TX
#define MAX485_DE_RE_PIN 12 // DE/RE

#define LED_PIN 2
#define RESET_PIN 0

#define FETCH_INTERVAL 1000
#define REQUEST_TIMEOUT 10000
#define RESET_HOLD_TIME 5000

String ap_ssid;
ModbusMaster node;
ESP8266WebServer WebServer(80);
SoftwareSerial RS485(MAX485_RO_PIN, MAX485_DI_PIN);

bool isFetching = false;
bool resetPending = false;
bool dataRequested = false;
bool wifiConfigured = false;

unsigned long lastFetchTime = 0;
unsigned long lastRequestTime = 0;

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
  digitalWrite(MAX485_DE_RE_PIN, HIGH);
}

void postTransmission() {
  digitalWrite(MAX485_DE_RE_PIN, LOW);
}

bool fetchData() {
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

void blinkLED(int pin, int times, int onTime, int offTime) {
  for (int i = 0; i < times; ++i) {
    digitalWrite(pin, LOW);
    delay(onTime);
    digitalWrite(pin, HIGH);
    delay(offTime);
  }
}

void checkButton() {
  static unsigned long btnStart = 0;
  if (digitalRead(RESET_PIN) == LOW) {
    if (btnStart == 0) {
      btnStart = millis();
    } else if (millis() - btnStart >= RESET_HOLD_TIME && !resetPending) {
      resetPending = true;
      for (int i = 0; i < 128; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      wifiConfigured = false;
      Serial.println("\nWiFi settings reset! Rebooting...");
      blinkLED(LED_PIN, 5, 500, 500);
      ESP.restart();
    }
  } else {
    btnStart = 0;
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

void saveWiFiConfig() {
  String ssid = trimString(WebServer.arg("ssid"));
  String password = trimString(WebServer.arg("password"));
  String ipconfig = WebServer.arg("ipconfig");
  for (int i = 0; i < 32; ++i) {
    EEPROM.write(i, ssid[i]);
    EEPROM.write(32 + i, password[i]);
  }
  EEPROM.write(64, ipconfig == "static" ? 1 : 0);
  if (ipconfig == "static") {
    String address = trimString(WebServer.arg("address"));
    String gateway = trimString(WebServer.arg("gateway"));
    String netmask = trimString(WebServer.arg("netmask"));
    for (int i = 0; i < 32; ++i) {
      EEPROM.write(65 + i, address[i]);
      EEPROM.write(97 + i, gateway[i]);
      EEPROM.write(129 + i, netmask[i]);
    }
  }
  EEPROM.commit();
  wifiConfigured = true;
  WebServer.send(200, "text/html", "<h1>WiFi settings saved! Rebooting...</h1>");
  delay(1000);
  ESP.restart();
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

void connectToWiFi() {
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
      blinkLED(LED_PIN, 1, 20, 1000);
      Serial.print(".");
      checkButton();
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

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(RESET_PIN, INPUT_PULLUP);

  pinMode(MAX485_DE_RE_PIN, OUTPUT);
  digitalWrite(MAX485_DE_RE_PIN, LOW);

  EEPROM.begin(512);
  connectToWiFi();

  RS485.begin(9600);
  node.begin(0x01, RS485);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  WebServer.on("/", HTTP_GET, []() {
    String html = wifiConfigured ? MAIN_page : WiFi_page;
    html.replace("%CSS_STYLES%", CSS_styles);
    WebServer.send(200, "text/html", html);
  });

  WebServer.on("/data", HTTP_GET, []() {
    WebServer.sendHeader("Access-Control-Allow-Origin", "*");

    dataRequested = true;
    lastRequestTime = millis();

    String json = "{";
    json += "\"Inverter_State\":" + String(sensorData.Inverter_State) + ",";
    json += "\"PV_Voltage\":" + String(sensorData.PV_Voltage) + ",";
    json += "\"PV_Current\":" + String(sensorData.PV_Current) + ",";
    json += "\"PV_Power\":" + String(sensorData.PV_Power) + ",";
    json += "\"Bus_Voltage\":" + String(sensorData.Bus_Voltage) + ",";
    json += "\"AC_Voltage\":" + String(sensorData.AC_Voltage) + ",";
    json += "\"AC_Current\":" + String(sensorData.AC_Current) + ",";
    json += "\"Grid_Frequency\":" + String(sensorData.Grid_Frequency) + ",";
    json += "\"Active_Power\":" + String(sensorData.Active_Power) + ",";
    json += "\"Reactive_Power\":" + String(sensorData.Reactive_Power) + ",";
    json += "\"Daily_Production\":" + String(sensorData.Daily_Production) + ",";
    json += "\"Total_Production\":" + String(sensorData.Total_Production) + ",";
    json += "\"Temperature_Module\":" + String(sensorData.Temperature_Module) + ",";
    json += "\"Temperature_Inverter\":" + String(sensorData.Temperature_Inverter) + ",";
    json += "\"Total_Running_Hour\":" + String(sensorData.Total_Running_Hour) + ",";
    json += "\"RSSI\":" + String(WiFi.RSSI());
    json += "}";

    WebServer.send(200, "application/json", json);
  });

  WebServer.on("/save", HTTP_POST, saveWiFiConfig);
  WebServer.begin();
}

void loop() {
  WebServer.handleClient();

  if (WiFi.status() != WL_CONNECTED) {
    blinkLED(LED_PIN, 1, 20, 3000);
  }

  if (dataRequested && millis() - lastFetchTime >= FETCH_INTERVAL) {
    if (!isFetching) {
      Serial.println("Starting data fetching...");
      isFetching = true;
    }
    fetchData();
    lastFetchTime = millis();
  }

  if (millis() - lastRequestTime > REQUEST_TIMEOUT && dataRequested) {
    Serial.println("Stopping data fetching...");
    dataRequested = false;
    isFetching = false;
  }

  checkButton();
}