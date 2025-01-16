// TempoBW
// Outdoor bluetooth and wifi thermometer
// PROGMaxi software 2025

#include <U8g2lib.h>
#include <RTClib.h>
#include <DallasTemperature.h>
#include <TaskScheduler.h>
#include <OneWire.h>
#include <EEPROM.h>
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
};

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
};


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
  //oled.sendBuffer();
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
      Serial.print("Client connected: ");
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
               info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
               info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
      Serial.println(macStr);
      clientCount++;
      Serial.print("Clients connected: ");
      Serial.println(clientCount);
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    {
      Serial.println("Client disconnected");
      clientCount--;
      Serial.print("Clients connected: ");
      Serial.println(clientCount);
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

void clearHistory()
{
  EEData edata = {0};
  for(int i = 0; i < 240; i++)
  {
    EEPROM.put(sizeof(EEData)*i, edata);
    EEPROM.commit();    
  }
}

void writeToEprom(int zerobaseIndex, EEData edata)
{
  EEPROM.put(sizeof(EEData)*zerobaseIndex, edata);
  EEPROM.commit();
}

EEData readFromEprom(uint zerobaseIndex)
{
  EEData edata = {0};
  EEPROM.get(sizeof(EEData)*zerobaseIndex, edata);
  return edata;
}

void resetSettings()
{
  EESet eset = {0};
  eset.altitude = 250.0f;
  eset.position = 255;
  eset.crc = 4444;
  EEPROM.put(sizeof(EESet)*245, eset);
  EEPROM.commit();
}

void writeSettings(EESet eset)
{
  EEPROM.put(sizeof(EESet)*245, eset);
  EEPROM.commit();
}

EESet readSettings()
{
  EESet eset = {0};
  EEPROM.get(sizeof(EESet)*245, eset);
  return eset;
}

void writePosition(int zerobaseIndex)
{
  if (zerobaseIndex > 239)
  {
    return;
  }
  EESet eset = readSettings();
  eset.position = zerobaseIndex;
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
  if (eset.position == 255)
  {
    return 0;
  }
  return eset.position;
}

void clearMinTemp()
{
  EEData edata = {0};
  edata.temperature = 55.5f;
  edata.processorTemperature = 0.0f;
  edata.humidity = 0.0f;
  edata.pressure = 0.0f;
  edata.hour = 0;
  edata.minute = 0;
  edata.day = 1;
  edata.month = 1;
  edata.year = 2025;
  EEPROM.put(sizeof(EEData)*246, edata);
  EEPROM.commit();
}

void clearMaxTemp()
{
  EEData edata = {0};
  edata.temperature = -55.5f;
  edata.processorTemperature = 0.0f;
  edata.humidity = 0.0f;
  edata.pressure = 0.0f;
  edata.hour = 0;
  edata.minute = 0;
  edata.day = 1;
  edata.month = 1;
  edata.year = 2025;
  EEPROM.put(sizeof(EEData)*247, edata);
  EEPROM.commit();
}

void writeMinTemp(EEData edata)
{
  EEPROM.put(sizeof(EEData)*246, edata);
  EEPROM.commit();
}

void writeMaxTemp(EEData edata)
{
  EEPROM.put(sizeof(EEData)*247, edata);
  EEPROM.commit();
}

EEData readMinTemp()
{
  EEData edata = {0};
  EEPROM.get(sizeof(EEData)*246, edata);
  return edata;
}

EEData readMaxTemp()
{
  EEData edata = {0};
  EEPROM.get(sizeof(EEData)*247, edata);
  return edata;
}

void writeHistory()
{
  DateTime now = rtc.now();
  int pos = readPosition();
  EEData edata = {0};
  edata.temperature = temperature;
  edata.processorTemperature = deviceTemp;
  edata.humidity = humidity;
  edata.pressure = pressure;
  edata.hour = now.hour();
  edata.minute = now.minute();
  edata.day = now.day();
  edata.month = now.month();
  edata.year = now.year();
  writeToEprom(pos, edata);
  increasePosition();
  EEData eminTemp = {0};
  eminTemp.temperature = minTemp.temperature;
  eminTemp.processorTemperature = deviceTemp;
  eminTemp.humidity = humidity;
  eminTemp.pressure = pressure;
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
  emaxTemp.pressure = pressure;
  emaxTemp.hour = maxTemp.hour;
  emaxTemp.minute = maxTemp.minute;
  emaxTemp.day = maxTemp.day;
  emaxTemp.month = maxTemp.month;
  emaxTemp.year = maxTemp.year;
  writeMaxTemp(emaxTemp);
}

Task measureTemp(360000, TASK_FOREVER, &writeHistory, &runner, false);



String history = R"rawjson(
[
  {"apos": 32, "time": "12:00", "date": "25/12/2025", "temperature": 25.5, "pressure": 1013.2, "humidity": 50.0, "devtemperature": 32.3},
  {"apos": 32, "time": "12:00", "date": "25/12/2025", "temperature": 25.5, "pressure": 1013.2, "humidity": 50.0, "devtemperature": 32.3},
  {"apos": 32, "time": "12:00", "date": "25/12/2025", "temperature": 25.5, "pressure": 1013.2, "humidity": 50.0, "devtemperature": 32.3}
]
)rawjson";


void handleRoot() 
{
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
                  font-size: 18px;
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
          <div class="center">
              <h1>TempoBW</h1>
          </div>
          <div class="center">
              <h2>Venkovní BT+WIFI teploměr</h2>
          </div>
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
  String html = R"rawliteral(
  <!doctype html>
  <html lang="cs">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      <title>TempoBW - Historie</title>
      <script>
        const history = [
          {
            apos: 32,
            time: "12:00",
            date: "25/12/2025",
            temperature: 25.5,
            pressure: 1013.2,
            humidity: 50.0,
            devtemperature: 32.3
          },
          {
            apos: 32,
            time: "12:00",
            date: "25/12/2025",
            temperature: 25.5,
            pressure: 1013.2,
            humidity: 50.0,
            devtemperature: 32.3
          },
          {
            apos: 32,
            time: "12:00",
            date: "25/12/2025",
            temperature: 25.5,
            pressure: 1013.2,
            humidity: 50.0,
            devtemperature: 32.3
          }
        ];
        function fetchHistory() {
          fetch("/history-data")
            .then((response) => response.json())
            .then((data) => {
              const table = document.getElementById("history-table");
              const aposLabel = document.querySelector(".apos-label");
              const aposValue = document.querySelector(".apos-value");
              if (data.length > 0) {
                aposValue.innerText = data[0].apos;
              }
              table.innerHTML =
                "<tr><th>###</th><th>ČAS</th><th>DATUM</th><th>TEPLOTA</th><th>TLAK</th><th>VLHKOST</th><th>TEPLOTA ZAŘÍZENÍ</th></tr>";
              data.forEach((row, index) => {
                table.innerHTML += `<tr>
                    <td>${index + 1}</td>
                    <td>${row.time}</td>
                    <td>${row.date}</td>
                    <td>${row.temperature} °C</td>
                    <td>${row.pressure} hPa</td>
                    <td>${row.humidity} %</td>
                    <td>${row.devtemperature} °C</td>
                  </tr>`;
              });
            });
        }
        function renderHistory() {
          const table = document.getElementById("history-table");
          const aposLabel = document.querySelector(".apos-label");
          const aposValue = document.querySelector(".apos-value");
          if (history.length > 0) {
            aposValue.innerText = history[0].apos;
          }
          table.innerHTML =
            "<tr><th>###</th><th>ČAS</th><th>DATUM</th><th>TEPLOTA [°C]</th><th>TLAK [hPa]</th><th>VLHKOST [%]</th><th>T.ZAŘÍZENÍ [°C]</th></tr>";
          history.forEach((row, index) => {
            table.innerHTML += `<tr>
                <td>${index + 1}</td>
                <td>${row.time}</td>
                <td>${row.date}</td>
                <td>${row.temperature} °C</td>
                <td>${row.pressure} hPa</td>
                <td>${row.humidity} %</td>
                <td>${row.devtemperature} °C</td>
              </tr>`;
          });
        }
        window.onload = fetchHistory;
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
        table {
          width: 90%;
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
      </style>
    </head>
    <body style="background-color: #212f3c">
      <div class="menu">
        <button id="home" onclick="location.href='/'">Domů</button>
        <button id="history" onclick="location.href='/history'">Historie</button>
        <button id="settings" onclick="location.href='/settings'">Nastavení</button>
      </div>
      <div class="center">
        <br />
        <br />
        <p><span class="apos-label">AKTUÁLNÍ POZICE:</span> <span class="apos-value">0</span></p>
        <br />
      </div>
      <table id="history-table"></table>
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

void handleHistoryData() 
{
  server.send(200, "application/json", history);
}

void handleSettings() 
{
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
          margin: 15px auto;
          display: flex;
          flex-direction: column;
          align-items: center;
        }
        label {
          font-size: 18px;
          margin: 10px 0;
          color: #ddd;
        }
        input {
          width: 60%;
          padding: 10px;
          margin: 10px 0;
          border: 1px solid #ccc;
          border-radius: 5px;
          text-align: center;
          font-size: 16px;
        }
        button {
          background-color: #4caf50;
          color: white;
          border: none;
          border-radius: 5px;
          padding: 10px 30px;
          font-size: 16px;
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
      </script>
    </head>
    <body style="background-color: #212f3c">
      <div class="menu">
        <button id="home" onclick="location.href='/'">Domů</button>
        <button id="history" onclick="location.href='/history'">Historie</button>
        <button id="settings" onclick="location.href='/settings'">Nastavení</button>
      </div>
      <div class="center">
        <h1>Nastavení zařízení</h1>
        <form action="/apply-settings" method="POST">
          <label for="time">Nastavit čas:</label>
          <input type="time" id="time" name="time" />
          <label for="date">Nastavit datum:</label>
          <input type="date" id="date" name="date" />
          <label for="altitude">Výška nad mořem (m):</label>
          <input type="number" id="altitude" name="altitude" min="0" max="8848" step="1" placeholder="Zadej výšku"/>
          <button type="submit">Uložit nastavení</button>
        </form>
        <h2>Možnosti:</h2>
        <form id="reset-history-form" action="/reset-history" method="POST">
          <button type="button" onclick="confirmReset()">Smazat historii</button>
        </form>
        <form action="/restart" method="POST">
          <button type="submit">Restartovat zařízení</button>
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

float parseAltitude(String altitude) {
    altitude.trim();
    if (altitude.length() == 0) {
        return NAN;
    }
    for (int i = 0; i < altitude.length(); i++) {
        char c = altitude[i];
        if (!(isDigit(c) || c == '.' || (c == '-' && i == 0))) {
            return NAN;
        }
    }
    float result = altitude.toFloat();
    if (result == 0.0 && altitude != "0" && altitude != "0.0") {
        return NAN;
    }
    return result;
}

void setDeviceAltitude(float fAlt)
{
  if (fAlt != NAN)
  {
    EESet eset = {0};
    eset = readSettings();
    eset.altitude = fAlt;
    writeSettings(eset);
  }
}

float getDeviceAltitude()
{
  EESet eset = {0};
  eset = readSettings();
  return eset.altitude;
}

void handleApplySettings() 
{
  String time = server.arg("time");
  String date = server.arg("date");
  String salt = server.arg("altitude");
  Serial.print("Nastavit čas: ");
  Serial.println(time);
  Serial.print("Nastavit datum: ");
  Serial.println(date);
  Serial.print("Nastavit vysku v metrech: ");
  Serial.println(salt);
  setDeviceTime(time);
  setDeviceDate(date);
  setDeviceAltitude(parseAltitude(salt));
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

void handleRestart() {
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
    </style>
    <script>
      function startCountdown() {
        let value = 4;
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
      <p><span class="apos-value">5</span></p>
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
  ESP.restart();
}

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t *payload, size_t length) 
{
  if (type == WStype_CONNECTED) 
  {
    //clientCount++;
    //Serial.printf("Client %u connected\n", client_num);
  } 
  else if (type == WStype_DISCONNECTED) 
  {
    //clientCount--;
    //Serial.printf("Client %u disconnected\n", client_num);
  }
}

// 6000 size eeprom / 24 size EEData = 250
// Size history = 240 * 24 = 5760
// 1xSettings (245)
// 1xMinTemp (246)
// 1xMaxTemp (247)
// 1440 / 240 = 6 minutes for one cyclus write to eeprom


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
      ESP.restart();
    }
  }
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
      ESP.restart();
    }
  }
  Serial.println("OK!");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  WiFi.onEvent(WiFiEvent);
  server.on("/", handleRoot);
  server.on("/history", handleHistory);
  server.on("/history-data", handleHistoryData);  
  server.on("/settings", handleSettings);
  server.on("/apply-settings", HTTP_POST, handleApplySettings);
  server.on("/reset-history", HTTP_POST, handleResetHistory);
  server.on("/restart", HTTP_POST, handleRestart);  
  server.begin();
  Serial.println("WIFI server begin");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);  
  Serial.println("WIFI websocket started");
  Wire.begin(SDA, SCL);
  Serial.println("Wire begin");
  if (!EEPROM.begin(6000))
  {
    Serial.println("EEPROM cannot be initialized!");
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
      Serial.println("NEW EEPROM settings init!");
    }
    else
    {
      altitude = eset.altitude;
    }
    Serial.println("EEPROM init OK!");
    Serial.print("Size EEPROM: ");
    Serial.println(EEPROM.length());
    Serial.print("Current position: ");
    Serial.println(eset.position);
    Serial.print("Current altitude: ");
    Serial.println(altitude);
    measureTemp.enableDelayed(360000);
    Serial.println("Writter eeprom starting [6min] !");
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

  /*
  if (SerialBT.available()) {
    char incomingChar = SerialBT.read();
    Serial.print("Received: ");
    Serial.println(incomingChar);
  }

  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  */

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

}




/*
  ShowData("TEPLOTA","-12.7 C");
  delay(5000);
  ShowData("VLHKOST","78.4 %");
  delay(5000);
  ShowData("ATM. TLAK","1018.3 hPa");
  delay(5000);
  ShowData("MIN 24/12/25 04:54","-10.5 C");
  delay(5000);
  ShowData("MAX 23/12/25 22:11","+32.5 C");
  delay(5000);
  ShowData("CAS ZARIZENI","05:18");
  delay(5000);
  ShowData("DATUM ZARIZENI","25/12/25");
  delay(5000);
  ShowData("TEPLOTA ZARIZENI","+5.8 C");
  delay(5000);
*/

