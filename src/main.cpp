#include "secrets.h" //ลบบรรทัดนี้

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include "html.h"

#define RTS_PIN D5
#define RX_PIN D6
#define TX_PIN D7

#ifndef SECRETS_H
const char *ssid = "your_wifi_ssid";
const char *password = "your_wifi_password";
const char *update_username = "your_update_username";
const char *update_password = "your_update_password";
#endif

unsigned long previousMillis = 0;
const long interval = 1000;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

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

void handleRoot()
{
  httpServer.send_P(200, "text/html", MAIN_page);
}

void handleData()
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

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  httpServer.on("/", HTTP_GET, handleRoot);
  httpServer.on("/data", HTTP_GET, handleData);
  httpUpdater.setup(&httpServer, "/update", update_username, update_password);
  httpServer.begin();
}

void loop(void)
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    fetchData();
  }
  httpServer.handleClient();
}