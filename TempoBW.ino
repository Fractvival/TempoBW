// TempoBW
// Outdoor bluetooth and wifi thermometer
// PROGMaxi software 2025

#include <LittleFS.h>
#include <U8g2lib.h>
#include <RTClib.h>
#include <DallasTemperature.h>
#include <TaskScheduler.h>
#include <OneWire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SHT4x.h>
#include "BluetoothSerial.h"

#define ONE_WIRE_BUS 19
#define SDA 21
#define SCL 22
#define WIDTH 128
#define HEIGHT 32


BluetoothSerial SerialBT;
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oled(U8G2_R0, SCL, SDA);
const char* ssid = "TempoBW";
const char* password = "";
WebServer server(80);
WebSocketsServer webSocket(81);
float temperature = 0.0f;
float pressure = 0.0f;
float humidity = 0.0f;
float deviceTemp = 0.0f;
float altitude = 250.0f;
int clientCount = 0;
unsigned long previousMillis = 0;
unsigned long interval = 5000;
unsigned int deltaDraw = 0;
Scheduler runner;


struct MMTemp 
{
  float temperature;
  int hour;
  int minute;
  int second;
  int day;
  int month;
  int year;
};

typedef struct EEData
{
  float temperature;
  float processorTemperature;
  float humidity;
  float pressure;
  uint8_t hour;
  uint8_t minute;
  uint8_t day;
  uint8_t month;
  uint16_t year;  
}EEData;

typedef struct EESet
{
  float altitude;
  float fset2;
  float fset3;
  float fset4;
  uint8_t position;
  uint8_t ui8set1;
  uint8_t ui8set2;
  uint8_t ui8set3;
  uint16_t crc;  
}EESet;


MMTemp minTemp = {55.5f,0,0,0,1,1,2025};
MMTemp maxTemp = {-55.5f,0,0,0,1,1,2025};

RTC_DS3231 rtc;
Adafruit_BME280 bme;
Adafruit_SHT4x sht = Adafruit_SHT4x();
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallas(&oneWire);

void ShowIntro(String text) 
{
  oled.setDrawColor(0);
  oled.drawBox(0, 0, WIDTH, HEIGHT);
  oled.sendBuffer();
  oled.setDrawColor(1);
  oled.drawBox(0, 0, WIDTH, HEIGHT);
  oled.sendBuffer();
  delay(1000);
  oled.setDrawColor(0);
  oled.drawBox(0, 0, WIDTH, HEIGHT);
  oled.sendBuffer();
  oled.setFont(u8g2_font_crox5hb_tf);
  int strWidth = oled.getStrWidth(text.c_str());
  int strHeight = oled.getMaxCharHeight();
  int textY = HEIGHT / 2;
  oled.setDrawColor(1);
  oled.drawStr((WIDTH - strWidth) / 2, textY + (strHeight/2), text.c_str());
  oled.sendBuffer();
  delay(3000);
  oled.setDrawColor(0);
  oled.drawBox(0, 0, WIDTH, HEIGHT);
  oled.sendBuffer();
}

void ShowData(String title, String text) 
{
  oled.setDrawColor(0);
  oled.drawBox(0, 0, WIDTH, HEIGHT);
  //oled.sendBuffer();
  oled.setDrawColor(1);
  oled.setFont(u8g2_font_lucasfont_alternate_tf);
  int strTitleWidth = oled.getStrWidth(title.c_str());
  int strTitleHeight = oled.getMaxCharHeight();
  int titleY = strTitleHeight;
  oled.drawStr((WIDTH - strTitleWidth) / 2, titleY, title.c_str());
  oled.setFont(u8g2_font_helvR14_tf);
  int strTextWidth = oled.getStrWidth(text.c_str());
  int strTextHeight = oled.getMaxCharHeight();
  int textY = strTitleHeight + strTextHeight;
  oled.drawStr((WIDTH - strTextWidth) / 2, textY-2, text.c_str());
  oled.sendBuffer();
}


void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) 
{
  //Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       Serial.println("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    Serial.println("Disconnected from WiFi access point"); break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    {
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        Serial.println("Lost IP address and IP address is reset to 0"); break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           Serial.println("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            Serial.println("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    {
      clientCount++;
      Serial.print("Client #");
      Serial.print(clientCount);
      Serial.print(" connected: ");
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
               info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
               info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
      Serial.println(macStr);
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    {
      Serial.println("Client disconnected!");
      clientCount--;
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    {
      IPAddress clientIP = IPAddress(info.wifi_ap_staipassigned.ip.addr);
      Serial.print("Client IP: ");
      Serial.println(clientIP);      
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:  Serial.println("Received probe request"); break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:         Serial.println("AP IPv6 is preferred"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:        Serial.println("STA IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_GOT_IP6:             Serial.println("Ethernet IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_START:               Serial.println("Ethernet started"); break;
    case ARDUINO_EVENT_ETH_STOP:                Serial.println("Ethernet stopped"); break;
    case ARDUINO_EVENT_ETH_CONNECTED:           Serial.println("Ethernet connected"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:        Serial.println("Ethernet disconnected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:              Serial.println("Obtained IP address"); break;
    default:                                    break;
  }
}

float calculateSeaLevelPressure(float pressure, float altitude) 
{
  float seaLevelPressure = pressure * pow(1 + (altitude / 44330.0), 5.255);
  return seaLevelPressure / 100.0;
}

bool writeToEprom(int index, EEData edata) 
{
    String filename = "/history_" + String(index) + ".dat";
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("WF: Error opening file for writing!");
        return false;
    }
    size_t bytesWritten = file.write((uint8_t*)&edata, sizeof(EEData));
    file.close();
    if (bytesWritten != sizeof(EEData)) {
        Serial.println("WF: Error writing data to file!");
        return false;
    }
    Serial.println("WF: Data successfully written to: " + filename);
    return true;
}

bool readFromEprom(int index, EEData &edata) 
{
    String filename = "/history_" + String(index) + ".dat";
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("RF: Error opening file for reading!");
        return false;
    }
    size_t bytesRead = file.read((uint8_t*)&edata, sizeof(EEData));
    file.close();
    if (bytesRead != sizeof(EEData)) {
        Serial.println("RF: Error reading data from file!");
        return false;
    }
    Serial.println("RF: Data successfully loaded from:" + filename);
    return true;
}

void clearHistory() 
{
    for (int i = 0; i < 260; i++) 
    {
        String filename = "/history_" + String(i) + ".dat";
        if (LittleFS.exists(filename)) 
        {
            LittleFS.remove(filename);
            Serial.println("File deleted: " + filename);
        }
    }
    Serial.println("History has been deleted.");
}

bool writeSettings(EESet eset) 
{
    File file = LittleFS.open("/settings.dat", "w");
    if (!file) {
        Serial.println("Error opening file to write settings!");
        return false;
    }
    size_t bytesWritten = file.write((uint8_t*)&eset, sizeof(EESet));
    file.close();
    return bytesWritten == sizeof(EESet);
}

EESet readSettings() 
{
    EESet eset = {0};
    File file = LittleFS.open("/settings.dat", "r");
    if (!file) {
        Serial.println("Error opening file to read settings!");
        return eset;
    }
    file.read((uint8_t*)&eset, sizeof(EESet));
    file.close();
    return eset;
}

void resetSettings() 
{
    EESet eset = {0};
    eset.altitude = 250.0f;
    eset.position = 255;
    eset.crc = 4444;
    writeSettings(eset);
    Serial.println("The settings have been reset.");
}

void writeAltitude(float altitude) 
{
    EESet eset = readSettings();
    eset.altitude = altitude;
    writeSettings(eset);
}

float readAltitude() 
{
    EESet eset = readSettings();
    return eset.altitude;
}

void writePosition(int index) 
{
    EESet eset = readSettings();
    eset.position = index;
    writeSettings(eset);
}

void increasePosition() 
{
    EESet eset = readSettings();
    eset.position++;
    if (eset.position > 239) 
    {
        eset.position = 0;
    }
    writeSettings(eset);
}

int readPosition() 
{
    EESet eset = readSettings();
    return eset.position;
}

void clearMinTemp() 
{
    EEData edata = {0};
    edata.temperature = 55.5f;
    LittleFS.remove("/minTemp.dat");
    writeMinTemp(edata);
}

void clearMaxTemp() 
{
    EEData edata = {0};
    edata.temperature = -55.5f;
    LittleFS.remove("/maxTemp.dat");
    writeMaxTemp(edata);
}

void writeMinTemp(EEData edata) 
{
    File file = LittleFS.open("/minTemp.dat", "w");
    if (file) 
    {
        file.write((uint8_t*)&edata, sizeof(EEData));
        file.close();
    }
    else 
    {
        Serial.println("Error while writing minimum temperature!");
    }
}

void writeMaxTemp(EEData edata) 
{
    File file = LittleFS.open("/maxTemp.dat", "w");
    if (file) 
    {
        file.write((uint8_t*)&edata, sizeof(EEData));
        file.close();
    }
    else
    {
        Serial.println("Error while writing maximum temperature!");
    }
}

EEData readMinTemp() 
{
    EEData edata = {0};
    File file = LittleFS.open("/minTemp.dat", "r");
    if (file) 
    {
        file.read((uint8_t*)&edata, sizeof(EEData));
        file.close();
    }
    else
    {
        Serial.println("Error reading minimum temperature!");
    }
    return edata;
}

EEData readMaxTemp() 
{
    EEData edata = {0};
    File file = LittleFS.open("/maxTemp.dat", "r");
    if (file) 
    {
        file.read((uint8_t*)&edata, sizeof(EEData));
        file.close();
    }
    else
    {
        Serial.println("Error reading maximum temperature!");
    }
    return edata;
}

void clearNamespace() 
{
    LittleFS.format();
    Serial.println("The file system has been formatted.");
}

void writeHistory()
{
  DateTime now = rtc.now();
  increasePosition();
  int pos = readPosition();
  EEData edata = {0};
  edata.temperature = temperature;
  edata.processorTemperature = deviceTemp;
  edata.humidity = humidity;
  edata.pressure = calculateSeaLevelPressure(pressure,altitude);//pressure;
  edata.hour = now.hour();
  edata.minute = now.minute();
  edata.day = now.day();
  edata.month = now.month();
  edata.year = now.year();
  writeToEprom(pos, edata);
  Serial.print("Current position: ");
  Serial.println(pos);
  EEData eminTemp = {0};
  eminTemp.temperature = minTemp.temperature;
  eminTemp.processorTemperature = deviceTemp;
  eminTemp.humidity = humidity;
  eminTemp.pressure = calculateSeaLevelPressure(pressure,altitude);
  eminTemp.hour = minTemp.hour;
  eminTemp.minute = minTemp.minute;
  eminTemp.day = minTemp.day;
  eminTemp.month = minTemp.month;
  eminTemp.year = minTemp.year;
  writeMinTemp(eminTemp);
  EEData emaxTemp = {0};
  emaxTemp.temperature = maxTemp.temperature;
  emaxTemp.processorTemperature = deviceTemp;
  emaxTemp.humidity = humidity;
  emaxTemp.pressure = calculateSeaLevelPressure(pressure,altitude);
  emaxTemp.hour = maxTemp.hour;
  emaxTemp.minute = maxTemp.minute;
  emaxTemp.day = maxTemp.day;
  emaxTemp.month = maxTemp.month;
  emaxTemp.year = maxTemp.year;
  writeMaxTemp(emaxTemp);
  Serial.println("Write history!");
}

Task measureTemp(360000, TASK_FOREVER, &writeHistory, &runner, false);


void handleRoot() 
{
  Serial.println("Index page!");
  String html = R"rawliteral(
  <!doctype html>
  <html lang="cs">
      <head>
          <meta charset="UTF-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1.0" />
          <title>TempoBW</title>
          <script>
              let socket;
              function formatWithLeadingZero(value) {
                  return value < 10 ? `0${value}` : value;
              }
              function initWebSocket() {
                  socket = new WebSocket("ws://" + location.host + ":81");
                  socket.onmessage = function (event) {
                      const data = JSON.parse(event.data);
                      document.getElementById("temp").innerText = `${data.temperature} °C`;
                      document.getElementById("press").innerText = `${data.pressure} hPa`;
                      document.getElementById("hum").innerText = `${data.humidity} %`;
                      document.getElementById("devTime").innerText = data.devTime;
                      document.getElementById("devDate").innerText = data.devDate;
                      document.getElementById("devTemp").innerText = `${data.devTemp} °C`;
                      document.getElementById("altitude").innerText = `${data.altitude} m.n.m`;
                      document.getElementById("clients").innerText = data.clientCount;
                      document.getElementById("minimum").innerText = `${data.minTemperature} °C`;
                      document.getElementById("minimumTime").innerText = 
                          `${formatWithLeadingZero(data.minTemperatureHour)}:${formatWithLeadingZero(data.minTemperatureMinute)}`;
                      document.getElementById("minimumDate").innerText = 
                          `${formatWithLeadingZero(data.minTemperatureDay)}/${formatWithLeadingZero(data.minTemperatureMonth)}/${data.minTemperatureYear}`;
                      document.getElementById("maximum").innerText = `${data.maxTemperature} °C`;
                      document.getElementById("maximumTime").innerText = 
                          `${formatWithLeadingZero(data.maxTemperatureHour)}:${formatWithLeadingZero(data.maxTemperatureMinute)}`;
                      document.getElementById("maximumDate").innerText = 
                          `${formatWithLeadingZero(data.maxTemperatureDay)}/${formatWithLeadingZero(data.maxTemperatureMonth)}/${data.maxTemperatureYear}`;
                  };
                  socket.onerror = function (error) {
                      console.error("WebSocket error:", error);
                      document.getElementById("temp").innerText = "WS CHYBA!";
                  };
                  socket.onclose = function () {
                      console.warn("WebSocket connection closed.");
                      document.getElementById("temp").innerText = "WS UZAVŘEN!";
                  };
                  window.addEventListener("beforeunload", () => {
                      // Uzavření WebSocket spojení
                      if (websocket.readyState === WebSocket.OPEN) {
                          websocket.close();
                      }
                  });                  
              }
              window.onload = initWebSocket;
          </script>
          <style>
              .center {
                  margin: 2px auto;
                  width: 80%;
                  border: 0px solid black;
                  text-align: center;
              }
              .menu {
                  display: flex;
                  justify-content: center;
                  gap: 15px;
              }
              .menu button {
                  padding: 15px 30px;
                  font-size: 15px;
                  border: none;
                  border-radius: 5px;
                  cursor: pointer;
                  transition: all 0.3s ease;
              }
              .menu button#home {
                  background-color: #4caf50;
                  color: white;
              }
              .menu button#history {
                  background-color: #2196f3;
                  color: white;
              }
              .menu button#settings {
                  background-color: #f44336;
                  color: white;
              }
              .menu button:hover {
                  opacity: 0.8;
              }
              h1 {
                  color: #4caf50;
                  font-size: 26px;
                  font-family: "Lucida Console", serif;
                  text-align: center;
                  text-transform: uppercase;
              }
              h2 {
                  color: #07efcc;
                  font-size: 15px;
                  font-family: "Lucida Console", serif;
                  text-align: center;
                  text-transform: uppercase;
              }
              h5 {
                  color: #f44336;
                  font-size: 12px;
                  font-family: "Lucida Console", serif;
                  text-align: center;
                  text-transform: uppercase;
              }
              .table-cell {
                  font-size: 16px;
                  color: #ffffff;
                  font-family: "Lucida Console";
                  text-align: center;
                  background-color: #000000;
              }
              .table-header {
                  color: #250550;
                  background-color: #6f767c;
              }
          </style>
      </head>
      <body style="background-color: #212f3c">
          <div class="menu">
              <button id="home" onclick="location.href='/'">Domů</button>
              <button id="history" onclick="location.href='/history'">Historie</button>
              <button id="settings" onclick="location.href='/settings'">Nastavení</button>
          </div>
          <br>
          <br>
          <table id="maintable" align="center" width="85%" cellspacing="5" cellpadding="5" border="1">
              <tbody>
                  <tr>
                      <td class="table-header">Aktuální teplota</td>
                      <td class="table-cell"><span id="temp">Načítám...</span></td>
                  </tr>
                  <tr>
                      <td class="table-header">Tlak vzduchu</td>
                      <td class="table-cell" id="press">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Vlhkost</td>
                      <td class="table-cell" id="hum">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Minimum</td>
                      <td class="table-cell" id="minimum">Načítám...</td>
                  </tr>
                  <tr>
                      <td style="border: none; background: none;"></td>
                      <td class="table-cell" id="minimumTime">Načítám...</td>
                  </tr>
                  <tr>
                      <td style="border: none; background: none;"></td>
                      <td class="table-cell" id="minimumDate">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Maximum</td>
                      <td class="table-cell" id="maximum">Načítám...</td>
                  </tr>
                  <tr>
                      <td style="border: none; background: none;"></td>
                      <td class="table-cell" id="maximumTime">Načítám...</td>
                  </tr>
                  <tr>
                      <td style="border: none; background: none;"></td>
                      <td class="table-cell" id="maximumDate">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Čas zařízení</td>
                      <td class="table-cell" id="devTime">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Datum zařízení</td>
                      <td class="table-cell" id="devDate">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Teplota zařízení</td>
                      <td class="table-cell" id="devTemp">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Nadmořská výška</td>
                      <td class="table-cell" id="altitude">Načítám...</td>
                  </tr>
                  <tr>
                      <td class="table-header">Počet klientů</td>
                      <td class="table-cell" id="clients">Načítám...</td>
                  </tr>
              </tbody>
          </table>
          <div class="center">
              <br />
              <h5>PROGMaxi software 2025</h5>
              <br />
              <br />
          </div>
      </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}


void handleHistory() 
{
  Serial.println("History page!");
  if (readPosition() == 255)
  {
    String emptyHistoryHtml = R"rawliteral(
    <!doctype html>
    <html lang="cs">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>TempoBW - Historie</title>
        <style>
            .center {
                margin: 2px auto;
                width: 80%;
                text-align: center;
            }
            .menu {
                display: flex;
                justify-content: center;
                gap: 15px;
            }
            .menu button {
                padding: 15px 30px;
                font-size: 15px;
                border: none;
                border-radius: 5px;
                cursor: pointer;
                transition: all 0.3s ease;
            }
            .menu button#home {
                background-color: #4caf50;
                color: white;
            }
            .menu button#history {
                background-color: #2196f3;
                color: white;
            }
            .menu button#settings {
                background-color: #f44336;
                color: white;
            }
            .menu button:hover {
                opacity: 0.8;
            }
            h1 {
                color: #4caf50;
                font-size: 26px;
                font-family: "Lucida Console", serif;
                text-align: center;
                text-transform: uppercase;
            }
            h2 {
                color: #07efcc;
                font-size: 15px;
                font-family: "Lucida Console", serif;
                text-align: center;
                text-transform: uppercase;
            }
            h5 {
                color: #f44336;
                font-size: 12px;
                font-family: "Lucida Console", serif;
                text-align: center;
                text-transform: uppercase;
            }
            .message {
                font-size: 18px;
                color: white;
                font-family: "Lucida Console", serif;
                text-align: center;
                margin: 20px 0;
            }
            .refresh-button {
                display: block;
                margin: 20px auto;
                padding: 10px 20px;
                font-size: 16px;
                color: white;
                background-color: #4caf50;
                border: none;
                border-radius: 5px;
                cursor: pointer;
                transition: all 0.3s ease;
            }
            .refresh-button:hover {
                opacity: 0.8;
            }
        </style>
    </head>
    <body style="background-color: #212f3c">
        <div class="menu">
            <button id="home" onclick="location.href='/'">Domů</button>
            <button id="history" onclick="location.href='/history'">Historie</button>
            <button id="settings" onclick="location.href='/settings'">Nastavení</button>
        </div>
        <div class="center">
            <br>
            <br>
            <p class="message">V HISTORII ZATÍM NENÍ ŽÁDNÝ ZÁZNAM</p>
            <br>
            <button class="refresh-button" onclick="location.href='/history'">OBNOVIT STRÁNKU</button>
        </div>
        <div class="center">
            <br />
            <h5>PROGMaxi software 2025</h5>
            <br />
            <br />
        </div>
    </body>
    </html>
    )rawliteral";
    server.send(200, "text/html", emptyHistoryHtml);    
  }
  else
  {
    String html = R"rawliteral(
    <!doctype html>
    <html lang="cs">
        <head>
            <meta charset="UTF-8" />
            <meta name="viewport" content="width=device-width, initial-scale=1.0" />
            <title>TempoBW - Historie</title>
            <style>
                .center {
                    margin: 2px auto;
                    width: 80%;
                    border: 0px solid black;
                    text-align: center;
                }
                .menu {
                    display: flex;
                    justify-content: center;
                    gap: 15px;
                }
                .menu button {
                    padding: 15px 30px;
                    font-size: 15px;
                    border: none;
                    border-radius: 5px;
                    cursor: pointer;
                    transition: all 0.3s ease;
                }
                .menu button#home {
                    background-color: #4caf50;
                    color: white;
                }
                .menu button#loadData {
                    background-color: #4caf50;
                    color: white;
                }
                .menu button#history {
                    background-color: #2196f3;
                    color: white;
                }
                .menu button#settings {
                    background-color: #f44336;
                    color: white;
                }
                .menu button:hover {
                    opacity: 0.8;
                }
                h1 {
                    color: #4caf50;
                    font-size: 26px;
                    font-family: "Lucida Console", serif;
                    text-align: center;
                    text-transform: uppercase;
                }
                h2 {
                    color: #07efcc;
                    font-size: 15px;
                    font-family: "Lucida Console", serif;
                    text-align: center;
                    text-transform: uppercase;
                }
                h5 {
                    color: #f44336;
                    font-size: 12px;
                    font-family: "Lucida Console", serif;
                    text-align: center;
                    text-transform: uppercase;
                }
                table {
                    width: 80%;
                    border-collapse: collapse;
                    margin: 10px auto;
                    justify-content: center;
                }
                th,
                td {
                    border: 1px solid #ddd;
                    padding: 8px;
                    text-align: center;
                    width: auto;
                }
                td {
                    background-color: #000000;
                    color: #ffffff;
                    font-family: "Lucida Console", serif;
                    font-size: 12px;
                }
                th {
                    background-color: #4caf50;
                    color: white;
                    font-size: 12px;
                }
                tr:nth-child(even) td {
                    background-color: #063406;
                }
                tr:nth-child(odd) td {
                    background-color: #043f5a;
                }
                tr.selected td {
                    background-color: #088e08 !important;
                }                
                .apos-label {
                    font-size: 16px;
                    color: #4caf50;
                    font-weight: bold;
                    font-family: "Lucida Console", serif;
                }
                .apos-value {
                    font-size: 20px;
                    color: #ffffff;
                    background-color: #000000;
                    font-weight: bold;
                    font-family: "Lucida Console", serif;
                    width: auto;
                    padding: 5px 15px;
                    border-radius: 5px;
                }
                .tooglebutton {
                    padding: 10px 20px;
                    font-size: 12px;
                    border: none;
                    border-radius: 5px;
                    cursor: pointer;
                    transition: all 0.3s ease;
                }
            </style>
            <script>
                let websocket = null;
                let currentPosition = 0;
                let idrow = 0;

                document.addEventListener("DOMContentLoaded", () => {
                    // Inicializace prvků po načtení DOM
                    const buttonToggle = document.getElementById("toggleWebSocket");
                    const buttonLoadData = document.getElementById("loadData");
                    const statusDisplay = document.getElementById("webSocketStatus");
                    // const positionValue = document.getElementById("posvalue");

                    // Funkce pro aktualizaci stavu WebSocket
                    function updateStatusDisplay() {
                        if (websocket) {
                            switch (websocket.readyState) {
                                case WebSocket.CONNECTING:
                                    statusDisplay.textContent = "Připojování...";
                                    break;
                                case WebSocket.OPEN:
                                    statusDisplay.textContent = "Připojeno";
                                    break;
                                case WebSocket.CLOSING:
                                    statusDisplay.textContent = "Odpojuji...";
                                    break;
                                case WebSocket.CLOSED:
                                    statusDisplay.textContent = "Odpojeno";
                                    break;
                            }
                        } else {
                            statusDisplay.textContent = "Odpojeno";
                        }
                    }

                    // Připojení WebSocket
                    function connectWebSocket() {
                        websocket = new WebSocket("ws://" + location.host + ":81");

                        websocket.onopen = () => {
                            console.log("WebSocket připojen.");
                            updateStatusDisplay();
                        };

                        websocket.onmessage = (event) => {
                            const message = JSON.parse(event.data);
                            if (message.type === "closeSocket")
                            {
                              disconnectWebSocket(() => {
                                  buttonToggle.textContent = "Připojit WebSocket";
                              });
                            }
                            else if (message.type === "position") 
                            {
                              currentPosition = message.position;
                              // positionValue.textContent = currentPosition;
                              idrow = 0;
                              sendMessage("loadhistory");
                            }
                            else if (message.type === "history") 
                            {
                              const data = message.payload;
                              if (data && typeof data.temperature === "number") {
                                  const tableBody = document.getElementById("dataBody");
                                  const row = document.createElement("tr");
                                  const date = `${String(data.day).padStart(2, "0")}.${String(data.month).padStart(2, "0")}.${data.year}`;
                                  const time = `${String(data.hour).padStart(2, "0")}:${String(data.minute).padStart(2, "0")}`;
                                  row.innerHTML = `
                                              <td>${++idrow}</td>
                                              <td>${date}</td>
                                              <td>${time}</td>
                                              <td>${data.temperature.toFixed(1)} &deg;C</td>
                                              <td>${data.humidity.toFixed(1)} %</td>
                                              <td>${data.pressure.toFixed(2)} hPa</td>
                                          `;

                                  if (idrow === currentPosition) {
                                      row.classList.add("selected");
                                  }
                                  tableBody.appendChild(row);
                                }
                            }
                        };

                        websocket.onclose = () => {
                            console.log("WebSocket uzavřen.");
                            updateStatusDisplay();
                        };

                        websocket.onerror = (error) => {
                            console.error("WebSocket chyba:", error);
                            updateStatusDisplay();
                        };
                    }

                    // Odpojení WebSocket
                    function disconnectWebSocket(callback) {
                        if (websocket && websocket.readyState === WebSocket.OPEN) {
                            websocket.close();
                            websocket.onclose = () => {
                                updateStatusDisplay();
                                if (callback) callback();
                            };
                        } else if (callback) {
                            callback();
                        }
                    }

                    // Odeslání zprávy
                    function sendMessage(message) {
                        if (websocket && websocket.readyState === WebSocket.OPEN) {
                            websocket.send(message);
                        } else {
                            alert("WebSocket není připojen.");
                        }
                    }

                    // Event listener pro tlačítko "Připojit/Odpojit"
                    buttonToggle.addEventListener("click", () => {
                        if (websocket && websocket.readyState === WebSocket.OPEN) {
                            disconnectWebSocket(() => {
                                buttonToggle.textContent = "Připojit WebSocket";
                            });
                        } else {
                            connectWebSocket();
                            buttonToggle.textContent = "Odpojit WebSocket";
                        }
                    });

                    // Event listener pro tlačítko "Načíst data"
                    buttonLoadData.addEventListener("click", () => {
                        sendMessage("getPosition");
                    });

                    // Kliknutí na tlačítka s atributem data-href
                    document.addEventListener("click", (event) => {
                        if (event.target.tagName === "BUTTON" && event.target.hasAttribute("data-href")) {
                            const targetHref = event.target.getAttribute("data-href");

                            // Kontrola, zda je WebSocket stále připojen
                            if (websocket && websocket.readyState === WebSocket.OPEN) {
                                event.preventDefault(); // Zastavíme přechod na jinou stránku
                                alert("WebSocket je stále připojen. Nejprve ho odpojte."); // Upozornění uživateli
                            } else {
                                // Pokud není WebSocket připojen, přejdeme na cílovou stránku
                                window.location.href = targetHref;
                            }
                        }
                    });
                });
            </script>
        </head>
        <body style="background-color: #212f3c">
            <div class="menu">
                <button id="home" data-href="/">Domů</button>
                <button id="history" data-href="/history">Historie</button>
                <button id="settings" data-href="/settings">Nastavení</button>
            </div>
            <div class="center">
                <br>
                <br>
                <br>
                <div>
                    <button class="tooglebutton" id="toggleWebSocket">Připojit WebSocket</button>
                    <br />
                    <br />
                    <span class="apos-value" id="webSocketStatus">Odpojeno</span>
                </div>
                <br>
                <br>
            </div>
            <div class="menu">
                <button id="loadData">Načíst data</button>
            </div>
            <br />
            <br />
            <table>
                <thead>
                    <tr>
                        <th>#</th>
                        <th>Datum</th>
                        <th>Čas</th>
                        <th>Teplota</th>
                        <th>Vlhkost</th>
                        <th>Tlak</th>
                    </tr>
                </thead>
                <tbody id="dataBody">
                    <!-- Data budou dynamicky načtena sem -->
                </tbody>
            </table>
            <div class="center">
                <br />
                <h5>PROGMaxi software 2025</h5>
                <br />
                <br />
            </div>
        </body>
    </html>
    )rawliteral";
    server.send(200, "text/html", html);
  }
}

String zeroPad(int number) 
{
  return (number < 10 ? "0" : "") + String(number);
}

void handleSettings() 
{
  Serial.println("Settings page!");
  String html = R"rawliteral(
  <!doctype html>
  <html lang="cs">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      <title>TempoBW - Nastavení</title>
      <style>
        body {
          background-color: #212f3c;
          font-family: Arial, sans-serif;
          color: white;
          margin: 5;
          padding: 5;
        }
        .center {
          margin: 2px auto;
          width: 80%;
          border: 0px solid black;
          text-align: center;
        }
        .menu {
          display: flex;
          justify-content: center;
          gap: 15px;
        }
        .menu button {
          padding: 15px 30px;
          font-size: 15px;
          border: none;
          border-radius: 5px;
          cursor: pointer;
          width: 50%;
          transition: all 0.3s ease;
        }
        .menu button#home {
          background-color: #4caf50;
          color: white;
          width: 33%;
        }
        .menu button#history {
          background-color: #2196f3;
          color: white;
          width: 33%;
        }
        .menu button#settings {
          background-color: #f44336;
          color: white;
          width: 33%;
        }
        .menu button:hover {
          opacity: 0.8;
        }
        h1 {
          color: #4caf50;
          font-size: 26px;
          font-family: "Lucida Console", serif;
          text-align: center;
          text-transform: uppercase;
        }
        h2 {
          color: #07efcc;
          font-size: 15px;
          font-family: "Lucida Console", serif;
          text-align: center;
          text-transform: uppercase;
        }
        h5 {
          color: #f44336;
          font-size: 12px;
          font-family: "Lucida Console", serif;
          text-align: center;
          text-transform: uppercase;
        }
        form {
          margin: 10px auto;
          display: flex;
          flex-direction: column;
          align-items: center;
        }
        label {
          font-size: 16px;
          margin: 8px 0;
          color: #ddd;
        }
        input {
          width: 60%;
          padding: 8px;
          margin: 8px 0;
          border: 1px solid #ccc;
          border-radius: 5px;
          text-align: center;
          font-size: 14px;
        }
        button {
          background-color: #4caf50;
          color: white;
          border: none;
          border-radius: 5px;
          padding: 8px 25px;
          font-size: 14px;
          cursor: pointer;
          transition: all 0.3s ease;
          width: 150px;
        }
        button:hover {
          background-color: #45a049;
        }
      </style>
      <script>
        function confirmReset() {
          if (confirm("Opravdu smazat historii? Je to nevratný krok.")) {
            document.getElementById("reset-history-form").submit();
          }
        }
        function confirmFactory() {
          if (confirm("Opravdu provést tovární nastavení ?")) {
            document.getElementById("factory-form").submit();
          }
        }
      </script>
    </head>
    <body style="background-color: #212f3c">
      <div class="menu">
        <button id="home" onclick="location.href='/'">Domů</button>
        <button id="history" onclick="location.href='/history'">Historie</button>
        <button id="settings" onclick="location.href='/settings'">Nastavení</button>
      </div>
      <div class="center">
        <form action="/apply-settings" method="POST">
          <label for="time">Nastavit čas:</label>
          <input type="time" id="time" name="time" />
          <label for="date">Nastavit datum:</label>
          <input type="date" id="date" name="date" />
          <label for="altitude">Výška nad mořem (m):</label>
          <input type="number" id="altitude" name="altitude" min="0" max="8848" step="1" placeholder="Zadejte výšku" />
          <button type="submit">Uložit nastavení</button>
        </form>
        <h2>Možnosti:</h2>
        <form id="reset-history-form" action="/reset-history" method="POST">
          <button type="button" onclick="confirmReset()">Smazat historii</button>
        </form>
        <form action="/restart" method="POST">
          <button type="submit">Restartovat zařízení</button>
        </form>
        <form id="factory-form" action="/factory" method="POST">
          <button type="button" onclick="confirmFactory()">Tovární nastavení (smaže EPROM s historií i nastavením)</button>
        </form>
      </div>
      <div class="center">
        <br>
        <h5>&copy; PROGMaxi software 2025</h5>
      </div>
    </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

bool setDeviceTime(String time) 
{
  if (time.length() != 5 || time.charAt(2) != ':') 
  {
    return false;
  }
  int hour = time.substring(0, 2).toInt();
  int minute = time.substring(3, 5).toInt();
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) 
  {
    return false;
  }
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, now.second()));
  return true;
}

bool setDeviceDate(String date) 
{
  if (date.length() != 10) 
  {
    return false;
  }
  char sep1 = date.charAt(4);
  char sep2 = date.charAt(7);
  if ((sep1 != sep2) || (sep1 != '-' && sep1 != '.' && sep1 != '/')) 
  {
    return false;
  }
  int year = date.substring(0, 4).toInt();
  int month = date.substring(5, 7).toInt();
  int day = date.substring(8, 10).toInt();
  if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31) 
  {
    return false;
  }
  DateTime now = rtc.now();
  rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
  return true;
}

float parseAltitude(String altitude) 
{
    altitude.trim();
    if (altitude.length() == 0) 
    {
        return NAN;
    }
    for (int i = 0; i < altitude.length(); i++) 
    {
        char c = altitude[i];
        if (!(isDigit(c) || c == '.' || (c == '-' && i == 0))) 
        {
            return NAN;
        }
    }
    float result = altitude.toFloat();
    if (result == 0.0 && altitude != "0" && altitude != "0.0") 
    {
        return NAN;
    }
    return result;
}

void handleApplySettings() 
{
  Serial.println("Apply settings!");
  String time = server.arg("time");
  String date = server.arg("date");
  String salt = server.arg("altitude");
  Serial.print("Nastavit čas: ");
  Serial.println(time);
  Serial.print("Nastavit datum: ");
  Serial.println(date);
  Serial.print("Nastavit vysku v metrech: '");
  Serial.print(salt);
  Serial.println("'");
  setDeviceTime(time);
  setDeviceDate(date);
  writeAltitude(parseAltitude(salt));
  altitude = parseAltitude(salt);
  String sendHtml = R"rawliteral(
  <!DOCTYPE html>
  <html lang="cs">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TempoBW</title>
    <style>
      body {
        background-color: #212f3c;
        font-family: Arial, sans-serif;
        color: white;
        margin: 0;
        padding: 0;
      }
      .center {
        margin: 20px auto;
        width: 80%;
        text-align: center;
      }
      .apos-label {
        font-size: 18px;
        color: #4CAF50;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
      }
      .apos-value {
        font-size: 24px;
        color: #FFFFFF;
        background-color: #000000;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
        width: auto;
        padding: 5px 15px;
        border-radius: 5px;
      }
    </style>
    <script>
      function startCountdown() {
        let value = 2;
        const span = document.querySelector('.apos-value');
        const interval = setInterval(() => {
          span.textContent = value;
          if (value === 0) {
            clearInterval(interval);
            history.back();
          }
          value--;
        }, 1000);
      }
      window.onload = startCountdown;
    </script>
  </head>
  <body>
    <div class="center">
      <br>
      <br>
      <p><span class="apos-value">3</span></p>
      <br>
    </div>
    <div class="center">
      <p><span class="apos-label">POŽADAVEK ÚSPĚŠNĚ ODESLÁN</span></p>
      <br>
      <br>
      <br>
    </div>
  </body>
  </html>    
  )rawliteral";
  server.send(200, "text/html", sendHtml);
}


void handleResetHistory() {
  Serial.println("Reset history!");
    // Funkce pro smazání historie
  clearHistory();
  clearMinTemp();
  clearMaxTemp();
  writePosition(255);
  String sendHtml = R"rawliteral(
  <!DOCTYPE html>
  <html lang="cs">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TempoBW</title>
    <style>
      body {
        background-color: #212f3c;
        font-family: Arial, sans-serif;
        color: white;
        margin: 0;
        padding: 0;
      }
      .center {
        margin: 20px auto;
        width: 80%;
        text-align: center;
      }
      .apos-label {
        font-size: 18px;
        color: #4CAF50;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
      }
      .apos-value {
        font-size: 24px;
        color: #FFFFFF;
        background-color: #000000;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
        width: auto;
        padding: 5px 15px;
        border-radius: 5px;
      }
      .button {
        background-color: #4CAF50;
        color: white;
        padding: 10px 20px;
        font-size: 18px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        text-decoration: none;
      }      
    </style>
  </head>
  <body>
    <div class="center">
      <br>
      <br>
    </div>
    <div class="center">
      <p><span class="apos-label">POŽADAVEK ÚSPĚŠNĚ ODESLÁN</span></p>
      <br>
      <button class="button" id="redirectButton" onclick="window.location.href='http://192.168.4.1'">PŘEJÍT NA HLAVNÍ STRÁNKU</button>
      <br>
      <br>
      <br>
    </div>
  </body>
  </html>    
  )rawliteral";
  server.send(200, "text/html", sendHtml);
}

void handleRestart() {
  Serial.println("Restart device!");
  // Funkce pro restart zařízení
  String sendHtml = R"rawliteral(
  <!DOCTYPE html>
  <html lang="cs">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TempoBW</title>
    <style>
      body {
        background-color: #212f3c;
        font-family: Arial, sans-serif;
        color: white;
        margin: 0;
        padding: 0;
      }
      .center {
        margin: 20px auto;
        width: 80%;
        text-align: center;
      }
      .apos-label {
        font-size: 18px;
        color: #4CAF50;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
      }
      .apos-value {
        font-size: 24px;
        color: #FFFFFF;
        background-color: #000000;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
        width: auto;
        padding: 5px 15px;
        border-radius: 5px;
      }
      .button {
        background-color: #4CAF50;
        color: white;
        padding: 10px 20px;
        font-size: 18px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        text-decoration: none;
      }      
    </style>
  </head>
  <body>
    <div class="center">
      <br>
      <br>
    </div>
    <div class="center">
      <p><span class="apos-label">POŽADAVEK ÚSPĚŠNĚ ODESLÁN</span></p>
      <p><span class="apos-label">ZNOVU SE PŘIPOJ K WIFI TEPLOMĚRU</span></p>
      <p><span class="apos-label">A POUŽIJ TLAČÍTKO K NÁVRATU NA HLAVNÍ STRÁNKU</span></p>
      <br>
      <button class="button" id="redirectButton" onclick="window.location.href='http://192.168.4.1'">PŘEJÍT NA HLAVNÍ STRÁNKU</button>
      <br>
      <br>
      <br>
    </div>
  </body>
  </html>    
  )rawliteral";
  server.send(200, "text/html", sendHtml);
  LittleFS.end();
  esp_restart();
}



void handleFactory()
{
  Serial.println("Factory set!");
  String sendHtml = R"rawliteral(
  <!DOCTYPE html>
  <html lang="cs">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TempoBW</title>
    <style>
      body {
        background-color: #212f3c;
        font-family: Arial, sans-serif;
        color: white;
        margin: 0;
        padding: 0;
      }
      .center {
        margin: 20px auto;
        width: 80%;
        text-align: center;
      }
      .apos-label {
        font-size: 18px;
        color: #4CAF50;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
      }
      .apos-value {
        font-size: 24px;
        color: #FFFFFF;
        background-color: #000000;
        font-weight: bold;
        font-family: 'Lucida Console', serif;
        width: auto;
        padding: 5px 15px;
        border-radius: 5px;
      }
      .button {
        background-color: #4CAF50;
        color: white;
        padding: 10px 20px;
        font-size: 18px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        text-decoration: none;
      }      
    </style>
  </head>
  <body>
    <div class="center">
      <br>
      <br>
    </div>
    <div class="center">
      <p><span class="apos-label">POŽADAVEK ÚSPĚŠNĚ ODESLÁN</span></p>
      <p><span class="apos-label">PO RESTARTU ZAŘÍZENÍ SE ZNOVU PŘIPOJ K WIFI TEPLOMĚRU</span></p>
      <p><span class="apos-label">POUŽIJ TLAČÍTKO K NÁVRATU NA HLAVNÍ STRÁNKU</span></p>
      <br>
      <button class="button" id="redirectButton" onclick="window.location.href='http://192.168.4.1'">PŘEJÍT NA HLAVNÍ STRÁNKU</button>
      <br>
      <br>
      <br>
    </div>
  </body>
  </html>    
  )rawliteral";
  server.send(200, "text/html", sendHtml);  
  measureTemp.cancel();
  measureTemp.disable();
  clearNamespace();
  LittleFS.end();
  esp_restart();
}

void sendHistory(uint8_t clientNum) 
{
  for (int i = 0; i < 240; i++) 
  {
      EEData data = {0};
      readFromEprom(i,data);
      DynamicJsonDocument doc(128);
      doc["type"] = "history";
      JsonObject payload = doc.createNestedObject("payload");
      payload["temperature"] = data.temperature;
      payload["humidity"] = data.humidity;
      payload["pressure"] = data.pressure;
      payload["hour"] = data.hour;
      payload["minute"] = data.minute;
      payload["day"] = data.day;
      payload["month"] = data.month;
      payload["year"] = data.year;
      String jsonString;
      serializeJson(doc, jsonString);
      webSocket.sendTXT(clientNum, jsonString);
  }
    DynamicJsonDocument doc(128);
    doc["type"] = "closeSocket";
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.sendTXT(clientNum, jsonString);
}


void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t *payload, size_t length) 
{
  if (type == WStype_CONNECTED) 
  {
    Serial.println("WebSocket connected");
  } 
  else if (type == WStype_DISCONNECTED) 
  {
    Serial.println("WebSocket disconnected");
  }
  if (type == WStype_TEXT) 
  {
    String message = String((char *)payload).substring(0, length);
    if (message.equals("getPosition")) 
    {
      Serial.println("Get position!");
      int position = readPosition();
      position++;
      DynamicJsonDocument doc(64);
      doc["type"] = "position";
      doc["position"] = position;
      String jsonString;
      serializeJson(doc, jsonString);
      webSocket.sendTXT(client_num, jsonString);
    } 
    else if (message.equals("loadhistory"))
    {
      Serial.println("Load history!");
      sendHistory(client_num);
    }
    else if (message.equals("overview"))
    {
    }
  }
}


String formatTwoDigits(int value) 
{
  if (value < 10) 
  {
    return "0" + String(value);
  }
  return String(value);
}

void callback_bluetooth(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) 
{
  if (event == ESP_SPP_SRV_OPEN_EVT) 
  {
    Serial.println("New BT client connected!");
    SerialBT.println("Welcome!");
    SerialBT.println("Available commands:");
    SerialBT.println("0 = This help");
    SerialBT.println("1 = Current temp");
    SerialBT.println("2 = Minimal temp");
    SerialBT.println("3 = Maximal temp");
    SerialBT.println("4 = Current pressure");
    SerialBT.println("5 = Current humidity");
    SerialBT.println("6 = Current position + history");
    SerialBT.println("7 = Board time/date");
    SerialBT.println("8 = Altitude");
    SerialBT.println("9 = Restart");
    SerialBT.println("Available settings:");
    SerialBT.println("10 = Set time HHMM");
    SerialBT.println("11 = Set date DDMMYYYY");
    SerialBT.println("12 = Set altitude");
    SerialBT.println("13 = Clear history");
    SerialBT.println("14 = Factory settings");
  }
  else if (event == ESP_SPP_CLOSE_EVT) 
  {
    Serial.println("BT client disconnect!");
  }
  else if (event == ESP_SPP_DATA_IND_EVT) 
  {
    Serial.print("BT client sending command: ");
    size_t len = param->data_ind.len;
    uint8_t *data = param->data_ind.data;
    char message[len + 1];
    memcpy(message, data, len);
    message[len] = '\0';
    Serial.println(message);
    String input = String(message);
    int firstSpaceIndex = input.indexOf(' ');
    int command = -1;
    String parameters = "";
    if (firstSpaceIndex == -1) 
    {
      command = input.toInt();
    }
    else
    {
      command = input.substring(0, firstSpaceIndex).toInt();
      parameters = input.substring(firstSpaceIndex + 1);
    }
    switch (command) 
    {
      case 0:
      {
        SerialBT.println("");
        SerialBT.println("Available commands:");
        SerialBT.println("0 = This help");
        SerialBT.println("1 = Current temp");
        SerialBT.println("2 = Minimal temp");
        SerialBT.println("3 = Maximal temp");
        SerialBT.println("4 = Current pressure");
        SerialBT.println("5 = Current humidity");
        SerialBT.println("6 = Current position + history");
        SerialBT.println("7 = Board time/date");
        SerialBT.println("8 = Altitude");
        SerialBT.println("9 = Restart");
        SerialBT.println("Available settings:");
        SerialBT.println("10 = Set time HHMM");
        SerialBT.println("11 = Set date DDMMYYYY");
        SerialBT.println("12 = Set altitude");
        SerialBT.println("13 = Clear history");
        SerialBT.println("14 = Factory settings");
        SerialBT.println("");
      }
      case 1:
      {
        Serial.println("Sending current temperature");
        SerialBT.println(String(temperature, 1));
        break;
      }
      case 2:
      {
        Serial.println("Sending minimal temperature");
        SerialBT.println(
          String(minTemp.temperature, 1) + " " +
          formatTwoDigits(minTemp.hour) +
          formatTwoDigits(minTemp.minute) + " " +
          formatTwoDigits(minTemp.day) +
          formatTwoDigits(minTemp.month) +
          String(minTemp.year)
        );          
        break;
      }
      case 3:
      {
        Serial.println("Sending maximal temperature");
        SerialBT.println(
          String(maxTemp.temperature, 1) + " " +
          formatTwoDigits(maxTemp.hour) +
          formatTwoDigits(maxTemp.minute) + " " +
          formatTwoDigits(maxTemp.day) +
          formatTwoDigits(maxTemp.month) +
          String(minTemp.year)
        );          
        break;
      }
      case 4:
      {
        Serial.println("Sending current pressure");
        SerialBT.println(String(calculateSeaLevelPressure(pressure,altitude),2));
        break;
      }
      case 5:
      {
        Serial.println("Sending current humidity");
        SerialBT.println(String(humidity, 1));
        break;
      }
      case 6:
      {
        Serial.println("Sending current position and history");
        String sendHistory = "";
        sendHistory += String(readPosition()) + "\n     ";
        for (int i = 0; i < 240; i++) 
        {
          EEData data = {0};
          readFromEprom(i,data);
          sendHistory +=
            formatTwoDigits(data.day) +
            formatTwoDigits(data.month) + 
            String(data.year) + " " +
            formatTwoDigits(data.hour) +
            formatTwoDigits(data.minute) + " " +
            String(data.temperature, 1) + " " +
            String(data.pressure, 2) + " " +
            String(data.humidity, 1) + "\n     "; //5 space
        }
        SerialBT.println(sendHistory);
        break;
      }
      case 7:
      {
        Serial.println("Sending time and date");
        DateTime now = rtc.now();
        SerialBT.println(
          formatTwoDigits(now.hour()) +
          formatTwoDigits(now.minute()) + " " +
          formatTwoDigits(now.day()) + 
          formatTwoDigits(now.month()) + 
          String(now.year())
        );
        break;
      }
      case 8:
      {
        Serial.println("Sending altitude");
        SerialBT.println(String(altitude,0));
        break;
      }
      case 9:
      {
        Serial.println("Restart TempoBW");
        delay(1000);
        LittleFS.end();
        esp_restart();
        break;
      }
      case 10:
      {
        if (isdigit(parameters[0]) && isdigit(parameters[1]) &&
            isdigit(parameters[2]) && isdigit(parameters[3])) {
          int hours = parameters.substring(0, 2).toInt();
          int minutes = parameters.substring(2, 4).toInt();
          if (hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60) {
            Serial.println("Time set to " + String(hours) + ":" + String(minutes));
            DateTime now = rtc.now();
            rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, minutes, 0)); 
            SerialBT.println("OK");
          } else {
            SerialBT.println("KO");
          }
        } else {
          SerialBT.println("KO");
        }
        break;
      }
      case 11:
      {
        if (isdigit(parameters[0]) && isdigit(parameters[1]) &&
            isdigit(parameters[2]) && isdigit(parameters[3]) && isdigit(parameters[4]) &&
            isdigit(parameters[5]) && isdigit(parameters[6]) && isdigit(parameters[7])) {
          int day = parameters.substring(0, 2).toInt();
          int month = parameters.substring(2, 4).toInt();
          int year = parameters.substring(4, 8).toInt();
          if (day >= 1 && day <= 31 && month >= 1 && month <= 12) 
          {
            Serial.println("Date set to " + String(day) + "." + String(month) + "." + String(year));
            DateTime now = rtc.now();
            rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));              
            SerialBT.println("OK");
          }
          else
          {
            SerialBT.println("KO");
          }
        }
        else
        {
          SerialBT.println("KO");
        }
        break;
      }
      case 12:
      {
        if (parameters.length() > 0 && parameters.toInt() != 0) 
        {
          altitude = parameters.toFloat();
          writeAltitude(altitude);
          Serial.println("Altitude set to " + String(altitude) + " m");
          SerialBT.println("OK");
        }
        else
        {
          SerialBT.println("KO");
        }
        break;
      }
      case 13:
      {
        Serial.println("Clear history!");
        clearHistory();
        clearMinTemp();
        clearMaxTemp();
        writePosition(255);
        SerialBT.println("OK");
        break;
      }
      case 14:
      {
        Serial.println("Factory settings!");
        delay(1000);
        measureTemp.cancel();
        measureTemp.disable();
        clearNamespace();
        LittleFS.end();
        esp_restart();
        break;
      }
      default:
      {
        Serial.println("Unknown command: " + String(command));
        SerialBT.println("Unknown command");
        break;
      }
    }
  }
}

void setup() 
{
  Serial.begin(9600);
  delay(600);
  Serial.println("\nTempoBW, welcome!\n");
  delay(200);
  if ( oled.begin() )
  {
    oled.clearBuffer();
    oled.clearDisplay();
    oled.sendBuffer();
    Serial.println("Display init OK!");
  }
  else 
  {
    Serial.println("Display init FAIL!");
  }
  delay(50);
  ShowIntro("TempoBW");
  Serial.print("Starting bluetooth...");
  int connectBluetoothDelta = 0;
  while(!SerialBT.begin("TempoBW", true, false))
  {
    Serial.print(".");
    delay(100);
    connectBluetoothDelta++;
    if (connectBluetoothDelta > 100)
    {
      Serial.println("FAIL!");
      Serial.println("Restart ESP is in proccessing...");
      delay(5000);
      esp_restart();
    }
  }
  SerialBT.register_callback(callback_bluetooth);
  Serial.println("OK!");
  WiFi.disconnect(true);
  Serial.print("Starting webserver...");
  WiFi.softAP(ssid, password);
  int startWebServer = 0;
  while (!WiFi.status() == WL_CONNECTED) 
  {
    Serial.print(".");
    delay(100);
    startWebServer++;
    if (startWebServer > 100)
    {
      Serial.println("FAIL!");
      Serial.println("Restart ESP is in proccessing...");
      delay(5000);
      esp_restart();
    }
  }
  Serial.println("OK!");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  WiFi.onEvent(WiFiEvent);
  server.on("/", handleRoot);
  server.on("/history", handleHistory);
  server.on("/settings", handleSettings);
  server.on("/apply-settings", HTTP_POST, handleApplySettings);
  server.on("/reset-history", HTTP_POST, handleResetHistory);
  server.on("/restart", HTTP_POST, handleRestart);  
  server.on("/factory", HTTP_POST, handleFactory); 
  server.begin();
  Serial.println("WIFI server begin");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);  
  Serial.println("WIFI websocket started");
  Wire.begin(SDA, SCL);
  Serial.println("Wire init OK");
  Serial.println("Startup LittleFS");
  if (!LittleFS.begin(true))
  {
    if (!LittleFS.format())
    {
      Serial.println("FAIL!");
      Serial.println("Restart ESP is in proccessing...");
      delay(5000);
      esp_restart();
    }
    else
    {
      Serial.println("LittleFS formatted successfully!");
    }
  }
  else
  {
    EESet eset = readSettings();
    if (eset.crc != 4444)
    {
      resetSettings();
      clearHistory();
      clearMinTemp();
      clearMaxTemp();
      altitude = 250.0f;
      Serial.println("NEW LittleFS settings init!");
      Serial.print("Altitude set to default: ");
      Serial.println(altitude);
    }
    else
    {
      Serial.println("LittleFS init OK!");
      if (isnan(eset.altitude))
      {
        altitude = 250.0f;
        writeAltitude(altitude);
        Serial.println("Incorrect altitude!");
        Serial.print("Altitude set to default: ");
        Serial.println(altitude);
      }
      else
      {
        altitude = eset.altitude;
        Serial.print("Altitude: ");
        Serial.println(altitude);
      }
    }
    eset = readSettings();
    Serial.print("Current position: ");
    Serial.println(eset.position);

    EEData miTemp = readMinTemp();
    EEData maTemp = readMaxTemp();

    minTemp.temperature = miTemp.temperature;
    minTemp.hour = miTemp.hour;
    minTemp.minute = miTemp.minute;
    minTemp.day = miTemp.day;
    minTemp.month = miTemp.month;
    minTemp.year = miTemp.year;
    Serial.println("Minimal temperature loaded");

    maxTemp.temperature = maTemp.temperature;
    maxTemp.hour = maTemp.hour;
    maxTemp.minute = maTemp.minute;
    maxTemp.day = maTemp.day;
    maxTemp.month = maTemp.month;
    maxTemp.year = maTemp.year;
    Serial.println("Maximal temperature loaded");

    measureTemp.enableDelayed(360000);
    Serial.println("Writter for LittleFS starting [6min] !");
  }
  if (!rtc.begin()) 
  {
    Serial.println("RTC not found!");
  }
  else
  {
    if (rtc.lostPower()) 
    {
      Serial.println("RTC has lost power, time will be reset!");
      rtc.adjust(DateTime(2025, 1, 1, 0, 0, 0));
    }    
  }  
  if (!bme.begin(0x76))
  {
    Serial.println("BME280 not found!");
  }
  else
  {
    Serial.println("BME280 init OK!");
  }
  if (!sht.begin())
  {
    Serial.println("SHT41 not found!");
  }
  else
  {
    sht.setPrecision(SHT4X_HIGH_PRECISION);
    Serial.println("SHT41 init OK!");
  }
  dallas.begin();
  dallas.setResolution(12);
  dallas.requestTemperatures();
  Serial.println("DALLAS sensor init OK!");
  Serial.println("TempoBW run!!");
}


void loop() 
{
  dallas.requestTemperatures();
  while (!dallas.isConversionComplete()) 
  {
    delay(100);
  }
  temperature = dallas.getTempCByIndex(0);
  deviceTemp = rtc.getTemperature();
  sensors_event_t se_humidity, se_temp;
  sht.getEvent(&se_humidity, &se_temp);
  humidity = se_humidity.relative_humidity;
  pressure = bme.readPressure();

  if (minTemp.temperature > temperature)
  {
    DateTime now = rtc.now();
    minTemp.temperature = temperature;
    minTemp.hour = now.hour();
    minTemp.minute = now.minute();
    minTemp.second = now.second();
    minTemp.day = now.day();
    minTemp.month = now.month();
    minTemp.year = now.year();
  }
  if (maxTemp.temperature < temperature)
  {
    DateTime now = rtc.now();
    maxTemp.temperature = temperature;
    maxTemp.hour = now.hour();
    maxTemp.minute = now.minute();
    maxTemp.second = now.second();
    maxTemp.day = now.day();
    maxTemp.month = now.month();
    maxTemp.year = now.year();
  }

  DateTime now = rtc.now();
  char devTime[6];
  char devDate[11];

  snprintf(devTime, sizeof(devTime), "%02d:%02d", now.hour(), now.minute());
  snprintf(devDate, sizeof(devDate), "%02d/%02d/%04d", now.day(), now.month(), now.year());

  DynamicJsonDocument json(512);
  json["temperature"] = String(temperature, 1);
  json["pressure"] = String(calculateSeaLevelPressure(pressure,altitude),2); //String(pressure, 1);
  json["humidity"] = String(humidity, 1);
  json["devTemp"] = String(deviceTemp, 1);
  json["altitude"] = String(altitude,0);
  json["clientCount"] = clientCount;
  json["devTime"] = devTime;
  json["devDate"] = devDate;
  json["minTemperature"] = String(minTemp.temperature,1);
  json["minTemperatureHour"] = minTemp.hour;
  json["minTemperatureMinute"] = minTemp.minute;
  json["minTemperatureDay"] = minTemp.day;
  json["minTemperatureMonth"] = minTemp.month;
  json["minTemperatureYear"] = minTemp.year;
  json["maxTemperature"] = String(maxTemp.temperature,1);
  json["maxTemperatureHour"] = maxTemp.hour;
  json["maxTemperatureMinute"] = maxTemp.minute;
  json["maxTemperatureDay"] = maxTemp.day;
  json["maxTemperatureMonth"] = maxTemp.month;
  json["maxTemperatureYear"] = maxTemp.year;

  String jsonString;
  jsonString.reserve(512);
  serializeJson(json, jsonString);
  webSocket.broadcastTXT(jsonString);

  server.handleClient();
  webSocket.loop();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;
    deltaDraw++;
    if (deltaDraw > 7)
      deltaDraw = 0;
  }

  switch(deltaDraw)
  {
    case 0:
    {
      String strData = String(temperature, 1);
      strData += " ";
      strData += (char)176;
      strData += "C";
      ShowData("*- TEPLOTA -*", strData);
      break;
    }
    case 1:
    {
      String strData = String(humidity, 1);
      strData += " %";
      ShowData("VLHKOST", strData);
      break;
    }
    case 2:
    {
      String strData = String(calculateSeaLevelPressure(pressure,altitude),2);
      strData += " hPa";
      ShowData("ATM. TLAK", strData);
      break;
    }
    case 3:
    {
      String strData = "MIN ";
      strData += (minTemp.day < 10 ? "0" : "") + String(minTemp.day);
      strData += "/";
      strData += (minTemp.month < 10 ? "0" : "") + String(minTemp.month);
      strData += "/";
      strData += String(minTemp.year).substring(2);      
      strData += " ";
      strData += (minTemp.hour < 10 ? "0" : "") + String(minTemp.hour);
      strData += ":";
      strData += (minTemp.minute < 10 ? "0" : "") + String(minTemp.minute);
      String strData2 = String(minTemp.temperature, 1);
      strData2 += " ";
      strData2 += (char)176;
      strData2 += "C";
      ShowData(strData, strData2);
      break;
    }
    case 4:
    {
      String strData = "MAX ";
      strData += (maxTemp.day < 10 ? "0" : "") + String(maxTemp.day);
      strData += "/";
      strData += (maxTemp.month < 10 ? "0" : "") + String(maxTemp.month);
      strData += "/";
      strData += String(maxTemp.year).substring(2);      
      strData += " ";
      strData += (maxTemp.hour < 10 ? "0" : "") + String(maxTemp.hour);
      strData += ":";
      strData += (maxTemp.minute < 10 ? "0" : "") + String(maxTemp.minute);
      String strData2 = String(maxTemp.temperature, 1);
      strData2 += " ";
      strData2 += (char)176;
      strData2 += "C";
      ShowData(strData, strData2);
      break;
    }
    case 5:
    {
      //DateTime now = rtc.now();
      String strData = (now.hour() < 10 ? "0" : "") + String(now.hour());
      strData += ":";
      strData += (now.minute() < 10 ? "0" : "") + String(now.minute());
      ShowData("CAS ZARIZENI", strData);
      break;
    }
    case 6:
    {
      String strData = (now.day() < 10 ? "0" : "") + String(now.day());
      strData += "/";
      strData += (now.month() < 10 ? "0" : "") + String(now.month());
      strData += "/";
      strData += String(now.year()).substring(2);;
      ShowData("DATUM ZARIZENI", strData);
      break;
    }
    case 7:
    {
      String strData = String(deviceTemp, 1);
      strData += " ";
      strData += (char)176;
      strData += "C";
      ShowData("TEPLOTA ZARIZENI", strData);
      break;
    }
  }

  runner.execute();
}




