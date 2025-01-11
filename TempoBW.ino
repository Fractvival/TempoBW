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


void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       Serial.println("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    Serial.println("Disconnected from WiFi access point"); break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        Serial.println("Lost IP address and IP address is reset to 0"); break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           Serial.println("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            Serial.println("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    Serial.println("Client connected"); break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: Serial.println("Client disconnected"); break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   Serial.println("Assigned IP address to client"); break;
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
    Serial.println("\nDisplay init OK!\n");
  }
  else 
  {
    Serial.println("\nDisplay init FAIL!\n");
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
  //WiFi.onEvent(onConnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  //WiFi.onEvent(onDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  WiFi.onEvent(WiFiEvent);

  server.begin();
}


void loop() 
{
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
  server.handleClient();
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

