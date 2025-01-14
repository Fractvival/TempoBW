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
//#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SHT4x.h>
//#include <Adafruit_GFX.h>
#include "BluetoothSerial.h"

#define ONE_WIRE_BUS 19
#define SDA 21
#define SCL 22
#define WIDTH 128
#define HEIGHT 32
#define ALTITUDE 230

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
int clientCount = 0;

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
  oled.sendBuffer();
  oled.setDrawColor(1);
  oled.setFont(u8g2_font_lucasfont_alternate_tf);
  int strTitleWidth = oled.getStrWidth(title.c_str());
  int strTitleHeight = oled.getMaxCharHeight();
  int titleY = strTitleHeight;
  oled.drawStr((WIDTH - strTitleWidth) / 2, titleY, title.c_str());
  oled.sendBuffer();
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
  <!DOCTYPE html>
  <html lang="cs">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>TempoBW</title>
      <script>
        let socket;
        function initWebSocket() 
        {
          socket = new WebSocket('ws://' + location.host + ':81');
          socket.onmessage = function(event) 
          {
            const data = JSON.parse(event.data);
            document.getElementById('temp').innerText = data.temperature + " °C";
            document.getElementById('press').innerText = data.pressure + " hPa";
            document.getElementById('hum').innerText = data.humidity + " %";
            document.getElementById('devTime').innerText = data.devTime;
            document.getElementById('devDate').innerText = data.devDate;
            document.getElementById('devTemp').innerText = data.devTemp + " °C";
            document.getElementById('clients').innerText = data.clientCount;
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
        font-size: 16px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        transition: all 0.3s ease;
      }
      .menu button#home {
        background-color: #4CAF50;
        color: white;
      }
      .menu button#history {
        background-color: #2196F3;
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
        color: #4CAF50;
        font-size: 36px;
        font-family: 'Lucida Console', serif;
        text-align: center;
        text-transform: uppercase;
      }      
      h2 {
        color: #07efcc;
        font-size: 26px;
        font-family: 'Lucida Console', serif;
        text-align: center;
        text-transform: uppercase;
      }      
      h5 {
        color: #f44336;
        font-size: 12px;
        font-family: 'Lucida Console', serif;
        text-align: center;
        text-transform: uppercase;
      }      
    </style>
    </head>
    <body style="background-color: #212f3c;">
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
      <table id="maintable" align="center" width="70%" cellspacing="15" cellpadding="10"
        border="1">
        <tbody>
          <tr>
            <td style="font-size: 22px; color: #5d02d8; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="6f767c">Aktuální
              teplota</td>
            <td style="font-size: 22px; color: #ffffff; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="000000"> <span
                id="temp">Načítám...</span> </td>
          </tr>
          <tr>
            <td style="font-size: 22px; color: #5d02d8; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="6f767c">Tlak
              vzduchu</td>
            <td style="font-size: 22px; color: #ffffff; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="000000" id="press">Načítám...</td>
          </tr>
          <tr>
            <td style="font-size: 22px; color: #5d02d8; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="6f767c">Vlhkost</td>
            <td style="font-size: 22px; color: #ffffff; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="000000" id="hum">Načítám...</td>
          </tr>
          <tr>
            <td style="font-size: 22px; color: #5d02d8; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="6f767c">Čas
              zařízení</td>
            <td style="font-size: 22px; color: #ffffff; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="000000" id="devTime">Načítám...</td>
          </tr>
          <tr>
            <td style="font-size: 22px; color: #5d02d8; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="6f767c">Datum
              zařízení</td>
            <td style="font-size: 22px; color: #ffffff; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="000000" id="devDate">Načítám...</td>
          </tr>
          <tr>
            <td style="font-size: 22px; color: #5d02d8; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="6f767c">Teplota
              zařízení</td>
            <td style="font-size: 22px; color: #ffffff; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="000000" id="devTemp">Načítám...</td>
          </tr>
          <tr>
            <td style="font-size: 22px; color: #5d02d8; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="6f767c">Počet
              klientů</td>
            <td style="font-size: 22px; color: #ffffff; font-family: Lucida Console;"
              align="center" width="50%" valign="center" bgcolor="000000" id="clients">Načítám...</td>
          </tr>
        </tbody>
      </table>
      <div class="center"> <br>
        <h5>PROGMaxi software 2025</h5>
        <br>
        <br>
      </div>
    </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleHistory() 
{
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="cs">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>TempoBW - Historie</title>
      <script>
        const history = [
          {"apos": 32, "time": "12:00", "date": "25/12/2025", "temperature": 25.5, "pressure": 1013.2, "humidity": 50.0, "devtemperature": 32.3},
          {"apos": 32, "time": "12:00", "date": "25/12/2025", "temperature": 25.5, "pressure": 1013.2, "humidity": 50.0, "devtemperature": 32.3},
          {"apos": 32, "time": "12:00", "date": "25/12/2025", "temperature": 25.5, "pressure": 1013.2, "humidity": 50.0, "devtemperature": 32.3}
        ];      
        function fetchHistory() {
          fetch('/history-data')
            .then(response => response.json())
            .then(data => {
              const table = document.getElementById('history-table');
              const aposLabel = document.querySelector('.apos-label');
              const aposValue = document.querySelector('.apos-value');
              if (data.length > 0) {
                aposValue.innerText = data[0].apos;
              }
              table.innerHTML = '<tr><th>###</th><th>ČAS</th><th>DATUM</th><th>TEPLOTA</th><th>TLAK</th><th>VLHKOST</th><th>TEPLOTA ZAŘÍZENÍ</th></tr>';
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
          const table = document.getElementById('history-table');
          const aposLabel = document.querySelector('.apos-label');
          const aposValue = document.querySelector('.apos-value');
          if (history.length > 0) {
            aposValue.innerText = history[0].apos;
          }
          table.innerHTML = '<tr><th>###</th><th>ČAS</th><th>DATUM</th><th>TEPLOTA [°C]</th><th>TLAK [hPa]</th><th>VLHKOST [%]</th><th>T.ZAŘÍZENÍ [°C]</th></tr>';
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
        font-size: 16px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        transition: all 0.3s ease;
      }
      .menu button#home {
        background-color: #4CAF50;
        color: white;
      }
      .menu button#history {
        background-color: #2196F3;
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
        color: #4CAF50;
        font-size: 36px;
        font-family: 'Lucida Console', serif;
        text-align: center;
        text-transform: uppercase;
      }      
      h2 {
        color: #07efcc;
        font-size: 26px;
        font-family: 'Lucida Console', serif;
        text-align: center;
        text-transform: uppercase;
      }      
      h5 {
        color: #f44336;
        font-size: 12px;
        font-family: 'Lucida Console', serif;
        text-align: center;
        text-transform: uppercase;
      }
      table {
        width: 70%;
        border-collapse: collapse;
        margin: 20px auto;
      }
      th, td {
        border: 1px solid #ddd;
        padding: 8px;
        text-align: center;
        width=16.6%;
      }
      td {
        background-color: #000000;
        color: #FFFFFF;
        font-family: 'Lucida Console', serif;
      }
      th {
        background-color: #4CAF50;
        color: white;
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
    </head>
    <body style="background-color: #212f3c;">
      <div class="menu"> <button id="home" onclick="location.href='/'">Domů</button>
        <button id="history" onclick="location.href='/history'">Historie</button>
        <button id="settings" onclick="location.href='/settings'">Nastavení</button>
      </div>
      <div class="center">
        <br>
        <br>
        <p><span class="apos-label">AKTUÁLNÍ POZICE:</span> <span class="apos-value">0</span></p>
        <br>
      </div>
      <div class="center">
      <table id="history-table">
      </table>
      </div>
      <div class="center"> <br>
        <h5>PROGMaxi software 2025</h5>
        <br>
        <br>
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
  <!DOCTYPE html>
  <html lang="cs">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
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
        margin: 20px auto;
        width: 80%;
        text-align: center;
      }
      .menu {
        display: flex;
        justify-content: center;
        gap: 15px;
        margin-bottom: 20px;
      }
      .menu button {
        padding: 15px 30px;
        font-size: 16px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        transition: all 0.3s ease;
        width: 120px;
        height: 40px;
      }
      .menu button#home { background-color: #4CAF50; color: white; }
      .menu button#history { background-color: #2196F3; color: white; }
      .menu button#settings { background-color: #f44336; color: white; }
      .menu button:hover { opacity: 0.8; }
      h1, h2 {
        text-transform: uppercase;
        color: #07efcc;
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
        background-color: #4CAF50;
        color: white;
        border: none;
        border-radius: 5px;
        padding: 10px 20px;
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
  <body style="background-color: #212f3c;">
    <div class="menu">
      <button id="home" onclick="location.href='/'">Domů</button>
      <button id="history" onclick="location.href='/history'">Historie</button>
      <button id="settings" onclick="location.href='/settings'">Nastavení</button>
    </div>
    <div class="center">
      <h1>Nastavení zařízení</h1>
      <form action="/apply-settings" method="POST">
        <label for="time">Nastavit čas:</label>
        <input type="time" id="time" name="time">
        <label for="date">Nastavit datum:</label>
        <input type="date" id="date" name="date">
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
  if (date.length() != 10 || date.charAt(4) != '-' || date.charAt(7) != '-') {
    return false;
  }
  int year = date.substring(0, 4).toInt();
  int month = date.substring(5, 7).toInt();
  int day = date.substring(8, 10).toInt();
  if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }
  DateTime now = rtc.now();
  rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
  return true;
}


void handleApplySettings() 
{
  String time = server.arg("time");
  String date = server.arg("date");
  Serial.print("Nastavit čas: ");
  Serial.println(time);
  Serial.print("Nastavit datum: ");
  Serial.println(date);
  setDeviceTime(time);
  setDeviceDate(date);
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
  //resetDeviceHistory();
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


void setup() 
{
  Serial.begin(9600);
  delay(500);
  Serial.println("\nTempoBW, welcome!\n");
  if ( oled.begin() )
  {
    oled.clearBuffer();
    oled.clearDisplay();
    oled.sendBuffer();
    Serial.println("Display init OK!\n");
  }
  else 
  {
    Serial.println("Display init FAIL!\n");
  }
  delay(50);
  //ShowIntro("TempoBW");
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
    Serial.println("EEPROM init OK!");
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

  DateTime now = rtc.now();
  char devTime[6];
  char devDate[11];

  snprintf(devTime, sizeof(devTime), "%02d:%02d", now.hour(), now.minute());
  snprintf(devDate, sizeof(devDate), "%02d-%02d-%04d", now.day(), now.month(), now.year());

  DynamicJsonDocument json(256);
  json["temperature"] = String(temperature, 1);
  json["pressure"] = String(calculateSeaLevelPressure(pressure,ALTITUDE),2); //String(pressure, 1);
  json["humidity"] = String(humidity, 1);
  json["devTemp"] = String(deviceTemp, 1);
  json["clientCount"] = clientCount;
  json["devTime"] = devTime;
  json["devDate"] = devDate;

  String jsonString;
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

