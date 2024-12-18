#include "html.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

#define RX_PIN 12
#define TX_PIN 13
#define RTS_PIN 14

const long fetchInterval = 1000;

const char *ssid = "ssid";
const char *password = "password";

ModbusMaster node;
ESP8266WebServer WebServer(80);
SoftwareSerial RS485Serial(RX_PIN, TX_PIN);

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
  if (millis() - lastFetchTime < fetchInterval) {
    return false;
  }

  uint8_t result = node.readHoldingRegisters(0x0000, 0x1F);
  if (result != node.ku8MBSuccess) {
    return false;
  }

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

  lastFetchTime = millis();
  return true;
}

void setup() {
  Serial.begin(115200);

  RS485Serial.begin(9600);
  node.begin(0x01, RS485Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  WebServer.on("/", HTTP_GET, []() {
    WebServer.send(200, "text/html", MAIN_page);
  });

  WebServer.on("/data", HTTP_GET, []() {
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
    WebServer.send(200, "application/json", jsonResponse);
    // WebServer.sendHeader("Access-Control-Allow-Origin", "*");
  });

  WebServer.begin();
}

void loop() {
  WebServer.handleClient();
  fetchData();
}