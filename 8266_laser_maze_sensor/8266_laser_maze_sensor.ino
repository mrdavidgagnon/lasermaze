#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Wire.h>
#include "SSD1306Wire.h"

#define ONE_HOUR 3600000UL

#ifndef STASSID
#define STASSID "zion"
#define STAPSK  "johnconnor"
#endif


//D0   = 16;
//D1   = 5;
//D2   = 4;
//D3   = 0;
//D4   = 2;
//D5   = 14;
//D6   = 12;
//D7   = 13;
//D8   = 15;
//D9   = 3;
//D10  = 1;

#define BEAM_BROKEN 1
#define BEAM_UNBROKEN 2

const char* ssid = STASSID;
const char* password = STAPSK;

SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h

ESP8266WiFiMulti wifiMulti;    // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

const char *OTAName = "ESP8266";         // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

WiFiUDP UDP;                   // Create an instance of the WiFiUDP class to send and receive UDP messages

const int NTP_PACKET_SIZE = 48;          // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE];      // A buffer to hold incoming and outgoing packets

// digital pin 2 has a pushbutton attached to it. Give it a name:
int sensor = 15; //D4 on board
int buzzer = 14; //D5 on board


/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  pinMode(sensor, INPUT);
  pinMode(buzzer, OUTPUT);
  
  Serial.println("\r\n");
  display.init();
  display.flipScreenVertically();

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

  startOTA();                  // Start the OTA service

  delay(500);
}

/*__________________________________________________________LOOP__________________________________________________________*/

void loop() {
  int state;
  int buttonState = digitalRead(sensor); //1 for no light, 0 for light
  if (buttonState == 1) state = BEAM_BROKEN;
  else state = BEAM_UNBROKEN;

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);

  if (state == BEAM_UNBROKEN) {
    display.drawString(0, 0, "RUNNING");
    display.display();
  }
  if (state == BEAM_BROKEN) {
    display.drawString(0, 0, "ALERT");
    display.display();
    tone(buzzer, 4000);
    delay(1000);
    noTone(buzzer);
  }
  
  ArduinoOTA.handle();                        // listen for OTA events
}

/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Try to connect to some given access points. Then wait for a connection
  WiFi.setSleep(false); //Fixes mDNS issues? See https://github.com/espressif/arduino-esp32/issues/5020
  
  wifiMulti.addAP(ssid, password);   // add Wi-Fi networks you want to connect to
  Serial.println("Connecting to " + String(ssid));
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Connecting to " + String(ssid) + "..."); 
  display.display();
  
  while (wifiMulti.run() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }


  
  Serial.println("\r\n");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());             // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  Serial.println("\r\n");

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  String strIPAddress = WiFi.localIP().toString();
  display.drawString(0, 0, "Connected."); 
  display.display();
  
}

void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages to port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}