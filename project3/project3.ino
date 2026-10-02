/**
  Class: ECE 5984 Special Study: IOT system design
  Project 3: Weather Web Service
  Ben Seidel
  benseidel@vt.edu
  
  Required libraries/files:
  TFT library
  Free Fonts library
  Seed rpcWiFi library
  JSON library
  HTTPS client library
  WiFi HTTP client library
  certificate.h with server certificate (if running post 2027 you will likely need a new one)
  f26p3config.h (This will contain your api_key and default wifi + zip information)

  Compiling/Uploading:
  When compiling this code, make sure you have your own f26p3config.h
  with the WIFI_SSID, WIFI_PASSWORD, and default ZIP code set. This is
  to be used if the serial monitor does not work. You will also need a
  certificate.h that will be used to access Weather API with HTTPS

  Acknowledgments:
  This code uses and modifies examples from SEED and the examples provided
  to us in class.
*/

#define DEBUG
#define REFRESH_TIME_MIN 1 // change to 15 later

#include "TFT_eSPI.h"                               // TFT LCD header file
#include "Free_Fonts.h"                             // Free fonts header file (needs to be in directory with this file)
#include "rpcWiFi.h"                                // WiFi library
#include <ArduinoJson.h>                            // JSON library
#include <WiFiClientSecure.h>                       // HTTPS client
#include <HTTPClient.h>                             // WiFi HTTP client

#include "f26p3config.h"                            // Personal WiFi info
#include "certificate.h"                            // Contains certificate

#define WIFI_ATTEMPTS 2
#define UPDATE_PERIOD_MIN 15                        // How often the weather API is called

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

unsigned long updateTime;                         // Last time refreshed in ms
int updatePeriod = UPDATE_PERIOD_MIN * 60 * 1000; // Length of update in ms

// interrupt flags
volatile bool buttonARequested = false;
volatile bool buttonBRequested = false;

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
  delay(2000);

  readParameters();

  statusMessage("Connecting to WiFi", TFT_BLACK);
  if(!connectWiFi()) {
    statusMessage("WiFi Failed", TFT_BLACK);
    while(1) delay(10000); // loop forever, not possible to connect to server without wifi
  }

  statusMessage("Getting Weather Data", TFT_BLACK);
  if(!getWeather()) {
    statusMessage("Request Failed", TFT_BLACK);
    while(1) delay(10000); // loop forever, request failed
  }

  statusMessage("Displaying Weather Data", TFT_BLACK);
  displayWeather();
  updateTime = millis();

  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(WIO_KEY_A), processButtonA, FALLING);
  attachInterrupt(digitalPinToInterrupt(WIO_KEY_B), processButtonB, FALLING);

}

void loop() {

  // toggle F and C
  if (buttonARequested) {
    noInterrupts();
    buttonARequested = false;
    interrupts();
    is_celsius = !is_celsius;
    updateTemperatureDisplay();
    while (digitalRead(WIO_KEY_A) == LOW);
  }

  // Refresh weather data
  if (buttonBRequested) {
    noInterrupts();
    buttonBRequested = false;
    interrupts();
    statusMessage("Getting Weather Data", TFT_BLACK);

    if (getWeather()) {
      statusMessage("Displaying Weather Data", TFT_BLACK);
      displayWeather();
      updateTime = millis(); // reset timer
    } else {
      statusMessage("Request Failed", TFT_BLACK);
    }

    while (digitalRead(WIO_KEY_B) == LOW);
  }

  // Automatic weather refresh
  if (millis() - updateTime >= updatePeriod) {
    statusMessage("Getting Weather Data", TFT_BLACK);

    if (getWeather()) {
      statusMessage("Displaying Weather Data", TFT_BLACK);
      displayWeather();
      updateTime = millis(); // reset timer
    } else {
      statusMessage("Request Failed", TFT_BLACK);
      updateTime = millis(); // reset timer
    }
  }
}


// Displays start screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);

  tft.setFreeFont(FSSB9);
  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT/2, TFT_WIDTH/2 - 75);
  tft.drawString("Project 3", TFT_HEIGHT/2, TFT_WIDTH/2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT/2, TFT_WIDTH/2 - 15);
  tft.drawString("benseidel@vt.edu", TFT_HEIGHT/2, TFT_WIDTH/2 + 15);
}

// Display weather data. Location, temperature, humidity, condition, last updated
void displayWeather() {

  uint16_t lightBlue = tft.color565(135, 206, 250);
  tft.fillRect(0, 0,TFT_HEIGHT,TFT_WIDTH/2 + 40 ,TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);

  String name = doc["location"]["name"].as<String>();
  String region = doc["location"]["region"].as<String>();
  float tempC = doc["current"]["temp_c"];
  float tempF = doc["current"]["temp_f"];
  float humidity = doc["current"]["humidity"];
  String condition = doc["current"]["condition"]["text"].as<String>();
  String localTime = doc["location"]["localtime"].as<String>();

  String location = String(name) + ", " + String(region);
  tft.setFreeFont(FSSB12);
  tft.drawString(location, TFT_HEIGHT / 2, TFT_WIDTH / 2 - 85);

  tft.setFreeFont(FSSB12);

  String temperature;
  if (is_celsius) {
    temperature = String(tempC, 1) + " C";
  } else {
    temperature = String(tempF, 1) + " F";
  }

  tft.setFreeFont(FSSB9);
  String humidityText = String(humidity, 0) + "%";

  tft.drawString("Temperature", TFT_HEIGHT/4 , TFT_WIDTH / 2 - 55);
  tft.drawString(temperature, TFT_HEIGHT / 4, TFT_WIDTH / 2 - 35);

  tft.drawString("Humidity", (3 * TFT_HEIGHT / 4), TFT_WIDTH / 2 - 55);
  tft.drawString(humidityText, 3 * TFT_HEIGHT / 4, TFT_WIDTH / 2 - 35);

  tft.drawString(condition, TFT_HEIGHT / 2, TFT_WIDTH / 2 + 5);

  tft.drawString("Updated: " + localTime, TFT_HEIGHT/2, TFT_WIDTH/2 + 25);
  tft.setFreeFont(FSSB9);

  if (is_celsius) {
    tft.drawString("F", TFT_HEIGHT / 2 + 10, TFT_WIDTH / 2 - 105);
  } else {
    tft.drawString("C", TFT_HEIGHT / 2 + 10, TFT_WIDTH / 2 - 105);
  }

  tft.drawString("Refresh", TFT_HEIGHT / 2 - 70, TFT_WIDTH / 2 - 105 );
}

// Updates the temperature display (used in interrupt)
void updateTemperatureDisplay() {
  float tempC = doc["current"]["temp_c"];
  float tempF = doc["current"]["temp_f"];

  String temperature;

  if (is_celsius) {
    temperature = String(tempC, 1) + " C";
  } else {
    temperature = String(tempF, 1) + " F";
  }

  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB9);
  tft.setTextColor(TFT_WHITE);

  tft.fillRect(TFT_HEIGHT / 4 - 50, TFT_WIDTH / 2 - 45, 110, 28, TFT_BLUE);
  tft.drawString(temperature, TFT_HEIGHT / 4, TFT_WIDTH / 2 - 35);
  tft.fillRect(TFT_HEIGHT / 2 - 5, TFT_WIDTH / 2 - 118, 30, 28, TFT_BLUE);

  if (is_celsius) {
    tft.drawString("F", TFT_HEIGHT / 2 + 10, TFT_WIDTH / 2 - 105);
  } else {
    tft.drawString("C", TFT_HEIGHT / 2 + 10, TFT_WIDTH / 2 - 105);
  }
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

// button A interrupt (switches from F to C and vice versa)
void processButtonA() {
  buttonARequested = true;
}

// button B interrupt (refreshes weather data)
void processButtonB() {
  buttonBRequested = true;
}

// Reads wifi SSID, password, and zip code for where weather is coming from
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

// Sends a GET request to weather API
bool getWeather() {
  #ifdef DEBUG
  Serial.println("STATUS: Getting current weather from API");
  #endif

  bool getSuccess = true;

  client.setCACert(test_root_ca);

  if (&client) {
    {
      HTTPClient https;

      #ifdef DEBUG
      Serial.print("STATUS: [HTTPS] begin...\n");
      #endif

      https.addHeader("Connection", "close");
      https.addHeader("Accept", "text/html, application/json, application/geo-json, application/ld-json");

      bool httpsError = true;
      while (httpsError) {

        char url[200];
        sprintf(url, "%s%s%s%s%s%s",
            "https://",
            weatherServer,
            "/v1/current.json?key=",
            api_key,
            "&q=",
            zip_code.c_str()
        );

        #ifdef DEBUG
        Serial.print("STATUS: Requesting URL: ");
        Serial.println(url);
        #endif

        if (https.begin(client, "api.weatherapi.com", 443, url, true)) {

          #ifdef DEBUG
          Serial.print("STATUS: [HTTPS] GET...\n");
          #endif

          int httpCode = https.GET();
          if (httpCode > 0) {
            #ifdef DEBUG
            Serial.printf("STATUS: [HTTPS] GET... code: %d\n", httpCode);
            #endif

            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {

              String payload = https.getString();

              #ifdef DEBUG
              Serial.print("STATUS: ");
              Serial.println(payload);
              #endif

              DeserializationError error = deserializeJson(doc, payload);

              if (error) {
                #ifdef DEBUG
                Serial.print("ERROR: JSON deserialization error: ");
                Serial.println(error.f_str());
                #endif

                getSuccess = false;
              }

              httpsError = false;
            }
          } else {
            #ifdef DEBUG
            Serial.printf(
              "ERROR: [HTTPS] GET... failed, error: %s\n",
              https.errorToString(httpCode).c_str()
            );
            Serial.println("STATUS: Waiting 10 seconds before trying again");
            #endif

            delay(10000);  // slight delay before retry
          }
        } else {
          #ifdef DEBUG
          Serial.printf("ERROR: [HTTPS] Unable to connect\n");
          #endif
          getSuccess = false;
        }

        https.end();
      }

    }
  } else {
    #ifdef DEBUG
    Serial.println("ERROR: Unable to create client");
    #endif

    getSuccess = false;
  }

  return(getSuccess);
}
