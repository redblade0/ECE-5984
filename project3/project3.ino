/**
  Class: ECE 5984 Special Study: IOT system design
  Project 3: Weather Web Service
  Ben Seidel
  benseidel@vt.edu
  
  Required libraries/files:
  TFT library
  Free Fonts library
  Seed rpcWiFi library

  Compiling/Uploading:
  When compiling this code, make sure you have your own f26p2config.h
  with the WIFI_SSID and WIFI_PASSWORD set.

  Acknowledgments:
  
*/

#include "TFT_eSPI.h"                               // TFT LCD header file
#include "Free_Fonts.h"                             // Free fonts header file (needs to be in directory with this file)
#include "rpcWiFi.h"                                // WiFi library
#include <ArduinoJson.h>                            // JSON library
#include <WiFiClientSecure.h>                       // HTTPS client
#include <HTTPClient.h>                             // WiFi HTTP client

#include "f26p3config.h"                            // Personal WiFi info
#include "certificate.h"

bool is_celsius = true;                           


TFT_eSPI tft;       // Built-in TFT display

void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  // Start serial initilization and wait 2 seconds for it to start
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);

  displaySplashScreen();
  statusMessage("Initializing", TFT_BLACK);
  delay(2000);


}

void loop() {
  // put your main code here, to run repeatedly:

}

// shows display screen + button A label for 12/24 hr mode
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);

  tft.setFreeFont(FSSB9);
  tft.drawString("C", TFT_HEIGHT/2 + 10, TFT_WIDTH/2 - 105);
  tft.drawString("Refresh", TFT_HEIGHT/2 - 70, TFT_WIDTH/2 - 105);
  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT/2, TFT_WIDTH/2 - 75);
  tft.drawString("Project 3", TFT_HEIGHT/2, TFT_WIDTH/2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT/2, TFT_WIDTH/2 - 15);
  tft.drawString("benseidel@vt.edu", TFT_HEIGHT/2, TFT_WIDTH/2 + 15);
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
