/**
  Class: ECE 5984 Special Study: IOT system design
  Project 4: CoAP Client-Server Architecture
  Ben Seidel
  benseidel@vt.edu
  
  Required libraries/files:
  

  Compiling/Uploading:
  

  Acknowledgments:

*/

#include "TFT_eSPI.h"     // TFT LCD header file
#include "Free_Fonts.h"   // Free fonts header file (needs to be in directory with this file)
#include <rpcWiFi.h>      //WiFi
#include <WiFiUDP.h>      //UDP
#include <ProsecCoAP.h>   //CoAP
#include "Grove_Temperature_And_Humidity_Sensor.h"

// WiFi AP config for CoAP server
#include "CoAPServerConfig.h"

// Used for debug mode
#define DEBUG
#define DHTTYPE DHT11
#define DHTPIN 0 // pin for digital port (right connector)

DHT dht(DHTPIN, DHTTYPE);

TFT_eSPI tft;             // Built-in TFT display


char * mySSID = AP_SSID;
char * myPW = AP_PASSWORD;

void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000));

  dht.begin();
  
  displaySplashScreen();
  delay(2000);

}

void loop() {
  // put your main code here, to run repeatedly:

}

// helper function to write status messages
void statusMessage(const char *message, uint16_t color) {
  uint16_t lightBlue = tft.color565(135, 206, 250);
  tft.fillRect(TFT_HEIGHT/2 - 120, TFT_WIDTH/2 + 40, 240, 40, lightBlue);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB9);
  tft.setTextColor(color);
  tft.drawString(message, TFT_HEIGHT/2, TFT_WIDTH/2 + 60);
}

// Displays start screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);

  tft.setFreeFont(FSSB9);
  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT/2, TFT_WIDTH/2 - 75);
  tft.drawString("Project 4 CoAP Server", TFT_HEIGHT/2, TFT_WIDTH/2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT/2, TFT_WIDTH/2 - 15);
  tft.drawString("benseidel@vt.edu", TFT_HEIGHT/2, TFT_WIDTH/2 + 15);
}

void displayTemperature() {


}

void displayHumidity() {


}
