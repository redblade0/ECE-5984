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

#define WIFI_TIMEOUT_SEC 30
#define WIFI_ATTEMPTS 2

TFT_eSPI tft;             // Built-in TFT display
WiFiClientSecure client;  // HTTPS client
JsonDocument doc;         // JSON document


bool is_celsius = true;
// These are strings becuase Serial.readString takes a string
// and it was easier to convert from string to a c string in wifi
String ssid = WIFI_SSID;
String password = WIFI_PASSWORD;
String zip_code = DEFAULT_ZIP_CODE;
const char* api_key = API_KEY;
const char* weatherServer = "api.weatherapi.com"; // weather service

const char *test_root_ca = R"(-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)";

void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  bool setupFailed = false;

  // Start serial initilization and wait 2 seconds for it to start
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);

  displaySplashScreen();
  statusMessage("Initializing", TFT_BLACK);
  delay(3000);

  readParameters();

  statusMessage("Connecting to WiFi", TFT_BLACK);
  if(!connectWiFi()) {
    statusMessage("WiFi Failed", TFT_BLACK);
    while(1) delay(10000); // loop forever, not possible to connect to server without wifi
  }


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
  // tft.drawString("C", TFT_HEIGHT/2 + 10, TFT_WIDTH/2 - 105);
  // tft.drawString("Refresh", TFT_HEIGHT/2 - 70, TFT_WIDTH/2 - 105);
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

void readParameters() {
  if(Serial) {
    statusMessage("Reading Parameters", TFT_BLACK);
    Serial.println();

    Serial.print("Wi-Fi SSID?: ");
    while(Serial.available() == 0) {}
    ssid = Serial.readString();
    ssid.trim();
    Serial.println(ssid);

    Serial.print("Wi-Fi Password?: ");
    while(Serial.available() == 0) {}
    password = Serial.readString();
    password.trim();
    Serial.println(password);

    Serial.print("Zip code for weather?: ");
    while(Serial.available() == 0) {}
    zip_code = Serial.readString();
    zip_code.trim();
    Serial.println(zip_code);
  } else {
    statusMessage("Using Default Parameters", TFT_BLACK);
  }
}


// Connects to wifi using SSID and password. Repeats WIFI_ATTEMPTS times.
// Taken from my project 2 code and slightly modified.
bool connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int attempts = 0;
  do {
    if(strlen(password.c_str()) > 0) {
      WiFi.begin(ssid.c_str(), password.c_str());
    } else {
      WiFi.begin(ssid.c_str());
    }
    attempts++;
    statusMessage("Connecting to Wi-Fi", TFT_BLACK);
    delay(1000);

    uint32_t start = millis();
    while((WiFi.status() != WL_CONNECTED) && ((millis() - start) < 4000));
  } while((WiFi.status() != WL_CONNECTED) && (attempts < WIFI_ATTEMPTS));

  // DEBUG for how many times WiFi is tried
  // Serial.print("attempts: ");
  // Serial.println(attempts);
  if(WiFi.status() != WL_CONNECTED) {
    statusMessage("Connection to Wi-Fi Failed", TFT_BLACK);
    delay(1000);
    return false;
  }
  
  statusMessage("Connected to Wi-Fi", TFT_BLACK);
  delay(1000);
  return true;
}


bool getWeather() {

  bool getSuccess = true;

  client.setCACert(test_root_ca);

  if(&client) {
    HTTPClient https;

    https.addHeader("Connection", "close");
    https.addHeader("Accept", "text/html, application/json, application/geo-json, application/ld-json");


    bool httpsError = true;
    while (httpsError) {

      // builds the url for a weather api request for the current weather of a specific zip code
      char url[200];
      sprintf(url, "%s%s%s%s%s%s%s",
        "https://",
        weatherServer,
        "/v1/current.json?key=",
        apiKey,
        "&q=",
        zipCode,
        "%20HTTP/1.1"
      );

      statusMessage("Getting Weather Data", TFT_BLACK);

      if(https.begin(client, weatherServer, 443, url, true)){
        
        int httpCode = https.GET();
        if(httpCode > 0) {
          if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMENTELY) {

            String payload = https.getString();
            Deserialization error = deserializeJson(doc, payload);
            
            if(error) {
              getSuccess = false;
            }
            httpsError = false;
          }
        } else {
          delay(10000) // delay before trying again
        }

      } 

    }

  }


}
