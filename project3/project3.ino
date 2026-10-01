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
#define DEBUG

#include "TFT_eSPI.h"                               // TFT LCD header file
#include "Free_Fonts.h"                             // Free fonts header file (needs to be in directory with this file)
#include "rpcWiFi.h"                                // WiFi library
#include <ArduinoJson.h>                            // JSON library
#include <WiFiClientSecure.h>                       // HTTPS client
#include <HTTPClient.h>                             // WiFi HTTP client

#include "f26p3config.h"                            // Personal WiFi info
#include "certificate.h"                            // Contains certificate

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

void setup() {
  // Initialize TFT
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  // Initialize Serial
  Serial.begin(115200);

  // Give Serial a moment to start
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);

  // Show splash screen
  displaySplashScreen();
  delay(3000);

  Serial.println("Starting test JSON...");

  // Test JSON instead of calling the weather API
  const char* testJson = R"json(
  {
    "location": {
      "name": "Blacksburg",
      "region": "Virginia",
      "country": "USA",
      "lat": 37.2563018798828,
      "lon": -80.434700012207,
      "tz_id": "America/New_York",
      "localtime_epoch": 1790805422,
      "localtime": "2026-09-30 17:57"
    },
    "current": {
      "last_updated": "2026-09-30 17:45",
      "temp_c": 23.0,
      "temp_f": 73.4,
      "condition": {
        "text": "Overcast"
      },
      "humidity": 32,
      "will_it_rain": 0,
      "chance_of_rain": 9,
      "will_it_snow": 0,
      "chance_of_snow": 0
    }
  }
  )json";

  DeserializationError error = deserializeJson(doc, testJson);

  if (error) {
    Serial.print("Test JSON error: ");
    Serial.println(error.f_str());

    statusMessage("JSON Failed", TFT_RED);
  } else {
    Serial.println("JSON parsed successfully!");
    displayWeather();
  }
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WIO_KEY_A), processButtonA, FALLING);
  statusMessage("TEST", TFT_BLACK);
}

// void setup() {
//   tft.begin();
//   tft.setRotation(3);
//   tft.backlight();

//   pinMode(WIO_KEY_A, INPUT_PULLUP);

//   bool setupFailed = false;

//   // Start serial initilization and wait 2 seconds for it to start
//   Serial.begin(115200);
//   uint32_t start = millis();
//   while (!Serial && (millis() - start) < 2000);

//   displaySplashScreen();
//   statusMessage("Initializing", TFT_BLACK);
//   delay(3000);

//   readParameters();

//   statusMessage("Connecting to WiFi", TFT_BLACK);
//   if(!connectWiFi()) {
//     statusMessage("WiFi Failed", TFT_BLACK);
//     while(1) delay(10000); // loop forever, not possible to connect to server without wifi
//   }

//   statusMessage("Getting Weather Data", TFT_BLACK);
//   if(!getWeather()) {
//     statusMessage("Request Failed", TFT_BLACK);
//     while(1) delay(10000); // loop forever, request failed
//   }

//   statusMessage("Displaying Weather Data", TFT_BLACK);
//   displayWeather();

// }

void loop() {

}

// shows display screen + button A label for 12/24 hr mode
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
  String humidityText =
  String(humidity, 0) + "%";

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
  is_celsius = !is_celsius;
  updateTemperatureDisplay();
  while(digitalRead(WIO_KEY_A) == LOW);
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
  #ifdef DEBUG
  Serial.println("STATUS: Getting current weather from API");
  #endif

  // Flag to indicate success or error return.
  bool getSuccess = true;

  // Set the certificate to use HTTPS.
  client.setCACert(test_root_ca);

  if (&client) {
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before
      // WiFiClientSecure *client is destroyed
      HTTPClient https;

      #ifdef DEBUG
      Serial.print("STATUS: [HTTPS] begin...\n");
      #endif

      // Set up header fields in request.
      https.addHeader("Connection", "close");
      https.addHeader("Accept", "text/html, application/json, application/geo-json, application/ld-json");

      // Loop while connection is not successful.
      bool httpsError = true;
      while (httpsError) {

        // Build URL for request.
        // This request gets the current weather conditions.
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

        // Connect to the server and send the request.
        if (https.begin(client, "api.weatherapi.com", 443, url, true)) {

          #ifdef DEBUG
          Serial.print("STATUS: [HTTPS] GET...\n");
          #endif

          // Start connection and send the HTTP request.
          int httpCode = https.GET();

          // httpCode will be negative on error.
          if (httpCode > 0) {

            // HTTP header has been sent and Server response header has been handled.
            #ifdef DEBUG
            Serial.printf("STATUS: [HTTPS] GET... code: %d\n", httpCode);
            #endif

            // File found at server.
            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {

              String payload = https.getString();

              #ifdef DEBUG
              Serial.print("STATUS: ");
              Serial.println(payload);
              #endif

              // Store JSON payload as doc.
              DeserializationError error = deserializeJson(doc, payload);

              if (error) {
                // Deserialization failed.
                #ifdef DEBUG
                Serial.print("ERROR: JSON deserialization error: ");
                Serial.println(error.f_str());
                #endif

                getSuccess = false;
              }

              httpsError = false;
            }
          }

          else {
            #ifdef DEBUG
            Serial.printf(
              "ERROR: [HTTPS] GET... failed, error: %s\n",
              https.errorToString(httpCode).c_str()
            );
            Serial.println("STATUS: Waiting 10 seconds before trying again");
            #endif

            delay(10000);  // slight delay before retry
          }
        }

        else {
          #ifdef DEBUG
          Serial.printf("ERROR: [HTTPS] Unable to connect\n");
          #endif

          getSuccess = false;
        }

        https.end();
      }

      // End extra scoping block
    }
  }

  else {
    #ifdef DEBUG
    Serial.println("ERROR: Unable to create client");
    #endif

    getSuccess = false;
  }

  // Return with status indicated.
  return(getSuccess);
}



// bool getWeather() {

//   Serial.println();
//   Serial.println("----------------------------------------");
//   Serial.println("ENTERING getWeather()");
//   Serial.println("----------------------------------------");

//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("WiFi NOT connected!");
//     return false;
//   }

//   Serial.println("WiFi connected.");

//   Serial.print("Connecting directly to ");
//   Serial.print(weatherServer);
//   Serial.println(":443...");

//   client.setCACert(test_root_ca);

//   if (!client.connect(weatherServer, 443)) {
//     Serial.println("TLS CONNECTION FAILED!");

//     Serial.print("Client connected: ");
//     Serial.println(client.connected());

//     return false;
//   }

//   Serial.println("TLS CONNECTION SUCCESSFUL!");

//   // Build HTTP request manually
//   String request =
//     String("GET /v1/current.json?key=") +
//     api_key +
//     "&q=" +
//     zip_code +
//     " HTTP/1.1\r\n" +
//     "Host: " +
//     weatherServer +
//     "\r\n" +
//     "Connection: close\r\n" +
//     "Accept: application/json\r\n" +
//     "\r\n";

//   Serial.println();
//   Serial.println("========== RAW HTTP REQUEST ==========");
//   Serial.println(request);
//   Serial.println("======================================");

//   Serial.println("Sending request...");

//   client.print(request);

//   Serial.println("Request sent.");

//   // Wait for response
//   uint32_t timeout = millis();

//   while (!client.available()) {

//     if (millis() - timeout > 10000) {
//       Serial.println("TIMEOUT waiting for server response.");
//       client.stop();
//       return false;
//     }

//     delay(100);
//   }

//   Serial.println();
//   Serial.println("========== SERVER RESPONSE ==========");

//   while (client.available()) {
//     String line = client.readStringUntil('\n');
//     Serial.println(line);
//   }

//   Serial.println("=====================================");

//   client.stop();

//   return true;
// }
