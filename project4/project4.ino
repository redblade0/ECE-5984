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
  

  // displaySplashScreen();
  // delay(2000);
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  drawGridLayout();
  displayTemperature();
  displayHumidity();
  statusMessage("TEST TEST TEST TEST", TFT_BLACK);


}

void loop() {
  // put your main code here, to run repeatedly:

}

// helper function to write status messages
void statusMessage(const char *message, uint16_t color) {
  uint16_t lightBlue = tft.color565(135, 206, 250);
  tft.fillRect(40, 185, 240, 40, lightBlue);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB9);
  tft.setTextColor(color);
  tft.drawString(message, 160, 205);
}

// Displays start screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);

  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT/2, TFT_WIDTH/2 - 75);
  tft.drawString("Project 4 CoAP Server", TFT_HEIGHT/2, TFT_WIDTH/2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT/2, TFT_WIDTH/2 - 15);
  tft.drawString("benseidel@vt.edu", TFT_HEIGHT/2, TFT_WIDTH/2 + 15);
}

// Draws grid around temperature and humidity data
void drawGridLayout() {
  tft.drawFastHLine(0, 40, 320, TFT_WHITE);
  tft.drawFastHLine(0, 170, 320, TFT_WHITE);
  tft.drawFastVLine(160, 40, 130, TFT_WHITE);
}

// displays the temperature
void displayTemperature() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  float temperatureFValue = dht.readTemperature(true);
  char valueBuffer[16];
  char stringBuffer[64];
  
  tft.setFreeFont(FSSB9);
  tft.drawString("Temperature", 80, 65);
  tft.drawString("Fahrenheit", 80, 150);
  
  if (!isnan(temperatureFValue)) {
    dtostrf(temperatureFValue, 3, 1, valueBuffer);
    sprintf(stringBuffer, "%s", valueBuffer);
    tft.fillRect(5, 80, 150, 50, TFT_BLUE);
    tft.setFreeFont(FSSB24);
    tft.drawString(stringBuffer, 80, 105);
  }
}

// displays humidity data
void displayHumidity() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  float humidityValue = dht.readHumidity();
  char valueBuffer[16];
  char stringBuffer[64];
  
  tft.setFreeFont(FSSB9);
  tft.drawString("Humidity", 240, 65);
  tft.drawString("Percent", 240, 150);
  
  if (!isnan(humidityValue)) {
    dtostrf(humidityValue, 3, 1, valueBuffer);
    sprintf(stringBuffer, "%s", valueBuffer);
    tft.fillRect(165, 80, 150, 50, TFT_BLUE);
    tft.setFreeFont(FSSB24);
    tft.drawString(stringBuffer, 240, 105);
  }
}
