/**
  Class: ECE 5984 Special Study: IOT system design
  Project 5: BLE Client-Server Application
  Ben Seidel
  benseidel@vt.edu
  
  Required libraries/files:
  TFT library
  Free Fonts library
  Seed rpcWiFi library
  Seed rpcBLE library
  Grove Temperature and Humidity sensor library

  Compiling/Uploading:
  

  Acknowledgments:
  
*/


#include "TFT_eSPI.h"     // TFT LCD header file
#include "Free_Fonts.h"   // Free fonts header file (needs to be in directory with this file)
#include <rpcWiFi.h>      //WiFi
#include <rpcBLEDevice.h> // rpcBLE device header file
#include <BLEServer.h>    // BLE header file
#include <BLE2902.h>      // BLE 2902 Client Characteristic Configuration Descriptor (CCCD) type header file
#include "Grove_Temperature_And_Humidity_Sensor.h" // DHT11 header file

#include "f26p5config.h" //config containing UUIDs, MAC address, and server/client name

#define DEBUG


TFT_eSPI tft;                 // Built-in TFT display

#define DHTTYPE DHT11
#define DHTPIN  0             // This is the pin to use for the digital port (right connector)
DHT dht(DHTPIN, DHTTYPE);

// global variables for sensor values
float lastTemp = 0;
float lastHumidity = 0;

void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000));

  displaySplashScreen();
  statusMessage("Initializing", TFT_BLACK);
  delay(2000);

  // WiFi.mode(WIFI_STA); // Set Wi-Fi to station mode
  // Serial.print("Wio Terminal MAC Address: ");
  // Serial.println(WiFi.macAddress());
  // statusMessage(WiFi.macAddress().c_str(), TFT_BLACK);

  tft.fillScreen(TFT_BLUE);
  drawGridLayout();
  displayServerName();


  // disconnected/connected
  statusMessage("", TFT_BLACK);


}

void loop() {
  // put your main code here, to run repeatedly:

}


// Displays start screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);

  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT/2, TFT_WIDTH/2 - 75);
  tft.drawString("Project 5 BLE Server", TFT_HEIGHT/2, TFT_WIDTH/2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT/2, TFT_WIDTH/2 - 15);
  tft.drawString("benseidel@vt.edu", TFT_HEIGHT/2, TFT_WIDTH/2 + 15);
}

// helper function to write status messages
void statusMessage(const char *message, uint16_t color) {
  uint16_t lightBlue = tft.color565(135, 206, 250);
  tft.fillRect(30, 185, 260, 40, lightBlue); 
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB9);
  tft.setTextColor(color);
  tft.drawString(message, 160, 205);
}

// Draws grid around temperature and humidity data
void drawGridLayout() {
  tft.drawFastHLine(0, 40, 320, TFT_WHITE);
  tft.drawFastHLine(0, 170, 320, TFT_WHITE);
  tft.drawFastVLine(160, 40, 130, TFT_WHITE);
}

// displays server name
void displayServerName() {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);
  tft.drawString(SERVER_DEVICE_NAME, 120, 10);
}

// // displays the temperature
// void displayTemperature() {
//   tft.setTextDatum(MC_DATUM);
//   tft.setTextColor(TFT_WHITE);
//   float temperatureFValue = dht.readTemperature(true);
//   currentTemperature = temperatureFValue;
//   char valueBuffer[16];
//   char stringBuffer[64];
  
//   tft.setFreeFont(FSSB9);
//   tft.drawString("Temperature", 80, 65);
//   tft.drawString("Fahrenheit", 80, 150);
  
//   if (!isnan(temperatureFValue)) {
//     dtostrf(temperatureFValue, 3, 1, valueBuffer);
//     sprintf(stringBuffer, "%s", valueBuffer);
//     tft.fillRect(5, 80, 150, 50, TFT_BLUE);
//     tft.setFreeFont(FSSB24);
//     tft.drawString(stringBuffer, 80, 105);
//   }
// }

// // displays humidity data
// void displayHumidity() {
//   tft.setTextDatum(MC_DATUM);
//   tft.setTextColor(TFT_WHITE);
//   float humidityValue = dht.readHumidity();
//   currentHumidity = humidityValue;
//   char valueBuffer[16];
//   char stringBuffer[64];
  
//   tft.setFreeFont(FSSB9);
//   tft.drawString("Humidity", 240, 65);
//   tft.drawString("Percent", 240, 150);
  
//   if (!isnan(humidityValue)) {
//     dtostrf(humidityValue, 3, 1, valueBuffer);
//     sprintf(stringBuffer, "%s", valueBuffer);
//     tft.fillRect(165, 80, 150, 50, TFT_BLUE);
//     tft.setFreeFont(FSSB24);
//     tft.drawString(stringBuffer, 240, 105);
//   }
// }


