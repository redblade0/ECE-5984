/**
  Class: ECE 5984 Special Study: IOT system design
  Project 4: CoAP Client-Server Architecture
  Ben Seidel
  benseidel@vt.edu
  
  Required libraries/files:
  TFT library
  Free Fonts library
  Seed rpcWiFi library
  WiFi UDP library
  Grove Temperature and Humidity sensor library
  ProsecCoAP library

  Compiling/Uploading:
  When compiling, make you have set your own CoAPServerConfig that contains
  your access point SSID and access point password. You should also connect
  the temperature and sensor connector on the right port of your WIO terminal.
  Start the server code (this file) before the client.

  Acknowledgments:
  This code uses and modifies code from the examples from class. I also used
  generative AI for creating the grid function as well as helping me fix my
  callback funcition for temperature.
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
#define DHTPIN 0               // pin for digital port (right connector)

uint32_t lastTemperatureRead = 0;
uint32_t lastHumidityRead = 3000;

// set values to null until values start coming in
float currentTemperature = NAN;
float currentHumidity = NAN;
float lastNotifiedTemperature = NAN;
float lastNotifiedHumidity = NAN;

const uint32_t SAMPLE_INTERVAL = 10000;   //interval in ms

Coap::ObserverRegistry<3> temperatureObservers;
Coap::ObserverRegistry<3> humidityObservers;

// Most recently received client name
char clientName[32] = "";

// Callback prototypes
void callbackTemperature(Coap::Message &message, IPAddress ip, uint16_t port);
void callbackHumidity(Coap::Message &message, IPAddress ip, uint16_t port);
void callbackClientName(Coap::Message &message, IPAddress ip, uint16_t port);


// Instantitate udp and coap objects (port is 5683)
WiFiUDP udp;
Coap::Node coapNode(udp, COAP_DEFAULT_PORT);

DHT dht(DHTPIN, DHTTYPE);
TFT_eSPI tft;                 // Built-in TFT display


char * mySSID = AP_SSID;
char * myPW = AP_PASSWORD;

void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000));

  displaySplashScreen();
  delay(2000);

  dht.begin();
  
  statusMessage("Reading Sensors", TFT_BLACK);
  delay(2000);

  tft.fillScreen(TFT_BLUE);
  drawGridLayout();
  displayServerName();
  displayTemperature();
  displayHumidity();
  
  // Configure access point
  statusMessage("Establishing WiFi AP", TFT_BLACK);
  delay(2000);

  // set device to be an access point
  if(!WiFi.mode(WIFI_AP)) {
    statusMessage("WiFi Failed", TFT_BLACK);
    while(1) delay(10000);
  }

  if(!WiFi.softAP(mySSID, myPW)){
    statusMessage("WiFi Failed", TFT_BLACK);
    while(1) delay(10000);
  }

  char message[64];
  snprintf(message, sizeof(message), "WiFi Active, %s", mySSID);
  statusMessage(message, TFT_BLACK);
  delay(2000);

  Coap::ErrorCode coapErrorCode;
  coapErrorCode = coapNode.serve("temperature", callbackTemperature);
  if ((int8_t)coapErrorCode != 0) {
    statusMessage("CoAP Failed", TFT_BLACK);
    while (1) delay(10000);
  }

  coapErrorCode = coapNode.serve("humidity", callbackHumidity);
  if ((int8_t)coapErrorCode != 0) {
    statusMessage("CoAP Failed", TFT_BLACK);
    while (1) delay(10000);
  }

  coapErrorCode = coapNode.serve("clientname", callbackClientName);
  if ((int8_t)coapErrorCode != 0) {
    statusMessage("CoAP Failed", TFT_BLACK);
    while (1) delay(10000);
  }

  coapErrorCode = coapNode.start();
  if((int8_t)coapErrorCode != 0) {
    statusMessage("CoAP Failed", TFT_BLACK);
    while(1) delay(10000);
  }

  IPAddress serverIP = WiFi.softAPIP();
  snprintf(message, sizeof(message), "CoAP: %d.%d.%d.%d:%d", serverIP[0], 
    serverIP[1], serverIP[2], serverIP[3], coapNode.getPort());
  statusMessage(message, TFT_BLACK);
  delay(2000);

}

void loop() {

  uint32_t now = millis();
  coapNode.loop();

  if(now - lastTemperatureRead >= SAMPLE_INTERVAL) {
    lastTemperatureRead = now;
    displayTemperature();

    if(!isnan(currentTemperature)) {
      if(isnan(lastNotifiedTemperature)) {
        lastNotifiedTemperature = currentTemperature;
      }  // checks for 1 degree diff in temp
      else if(currentTemperature - lastNotifiedTemperature >= 1.0 ||
               lastNotifiedTemperature - currentTemperature >= 1.0) {
        notifyTemperature();
      }
    }
  }

  if(now - lastHumidityRead >= SAMPLE_INTERVAL) {
    lastHumidityRead = now;
    displayHumidity();

    if(!isnan(currentHumidity)) {
      if(isnan(lastNotifiedHumidity)) {
        lastNotifiedHumidity = currentHumidity;
      } // checks for 1% diff in humidity
      else if(currentHumidity - lastNotifiedHumidity >= 1.0 ||
               lastNotifiedHumidity - currentHumidity >= 1.0) {
        notifyHumidity();
      }
    }
  }
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

// displays server name
void displayServerName() {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);
  String serverString = String("Server ") + mySSID;
  tft.drawString(serverString, 15, 10);
}

// displays client name
void displayClientName() {
  tft.fillRect(160, 0, 160, 40, TFT_BLUE);

  if (strlen(clientName) > 0) {
    tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(FSSB9);
    tft.setTextColor(TFT_WHITE);
    char text[48];
    snprintf(text, sizeof(text), "Client: %s", clientName);
    tft.drawString(text, 165, 10);
  }
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
  currentTemperature = temperatureFValue;
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

// notifies clients if temp has changed
void notifyTemperature() {
  if (temperatureObservers.countActive() == 0) {
    return;
  }

  for (size_t i = 0; i < temperatureObservers.length(); i++) {
    if (temperatureObservers[i].isActive()) {
      Coap::Message notification(Coap::MessageType::CON,Coap::MessageCode::CONTENT);
      addSensorPayload(notification, currentTemperature);

      Coap::ErrorCode err = notification.intoNotification(temperatureObservers[i]);
      if (err == Coap::ErrorCode::OK) {
        err = coapNode.sendMessage(notification, temperatureObservers[i].getIp(), temperatureObservers[i].getPort());
      }

      if (err == Coap::ErrorCode::OK) {
        statusMessage("Temperature Notification", TFT_BLACK);
      }

      #ifdef DEBUG
      if (err == Coap::ErrorCode::OK) {
        Serial.print("STATUS: Temperature notification sent: ");
        Serial.println(currentTemperature);
      } else {
        Serial.print("ERROR: Temperature notification failed: ");
        Serial.println((int8_t)err);
      }
      #endif
    }
  }
  lastNotifiedTemperature = currentTemperature;
}

// displays humidity data
void displayHumidity() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  float humidityValue = dht.readHumidity();
  currentHumidity = humidityValue;
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

// notifies clients if humidity has changed
void notifyHumidity() {

  // Only send notifications if there are active observers.
  if (humidityObservers.countActive() == 0) {
    return;
  }

  for (size_t i = 0; i < humidityObservers.length(); i++) {
    if (humidityObservers[i].isActive()) {
      Coap::Message notification(Coap::MessageType::CON, Coap::MessageCode::CONTENT);
      addSensorPayload(notification, currentHumidity);

      Coap::ErrorCode err = notification.intoNotification(humidityObservers[i]);
      if (err == Coap::ErrorCode::OK) {
        err = coapNode.sendMessage(notification, humidityObservers[i].getIp(), humidityObservers[i].getPort());
      }

      if (err == Coap::ErrorCode::OK) {
        statusMessage("Humidity Notification", TFT_BLACK);
      }

      #ifdef DEBUG
      if (err == Coap::ErrorCode::OK) {
        Serial.print("STATUS: Humidity notification sent: ");
        Serial.println(currentHumidity);
      } else {
        Serial.print("ERROR: Humidity notification failed: ");
        Serial.println((int8_t)err);
      }
      #endif
    }
  }
  lastNotifiedHumidity = currentHumidity;
}

// helper function to not write out network byte order.
// I used AI to help with this function for converting the floating point.
void addSensorPayload(Coap::Message &response, float value) {
  uint32_t scaledValue = (uint32_t)(value * 10.0f);
  uint8_t networkBuffer[sizeof(scaledValue)];
  Coap::Utils::toNetworkByteOrder(scaledValue, networkBuffer);
  response.addPayload(networkBuffer, sizeof(networkBuffer), Coap::ContentFormat::APPLICATION_OCTET_STREAM);
}

// callback function for temperature, responds to GET and PUT requests from client
void callbackTemperature(Coap::Message &message, IPAddress ip, uint16_t port) {

  if (message.getCode() == Coap::MessageCode::GET) {
    if (message.isObserveRegister()) {
      statusMessage("Temperature Registration", TFT_BLACK);
    }

    // service unavailable, returns noting
    if (isnan(currentTemperature)) {
      Coap::Message response;
      response.intoResponse(message, Coap::MessageCode::SERVICE_UNAVAILABLE);
      if (coapNode.sendMessage(response, ip, port) == Coap::ErrorCode::OK) {
        statusMessage("Temperature Response Sent", TFT_BLACK);
      }
      return;
    }

    Coap::Message response;
    response.intoResponse(message, Coap::MessageCode::CONTENT);
    addSensorPayload(response, currentTemperature);

    if (message.isObserveRegister()) {
      Coap::Observer new_observer(ip, port, message.getToken(), message.getTokenLength());
      Coap::ErrorCode err = temperatureObservers.add(new_observer);
      if (err == Coap::ErrorCode::OK) {
        response.intoNotification(new_observer);
        response.setType(Coap::MessageType::ACK);
      }
      else {
        response.intoResponse(message, Coap::MessageCode::SERVICE_UNAVAILABLE);
      }

    } else if (message.isObserveDeregister()) {
      Coap::ErrorCode err = temperatureObservers.remove(ip, port, message.getToken(), message.getTokenLength());

      if (err == Coap::ErrorCode::OK) {
        response.intoResponse(message, Coap::MessageCode::VALID);
      }
      else {
        response.intoResponse(message, Coap::MessageCode::SERVICE_UNAVAILABLE);
      }
    }

    if (coapNode.sendMessage(response, ip, port) != Coap::ErrorCode::OK) {
      #ifdef DEBUG
      Serial.println("ERROR: Failed to send temperature response");
      #endif
    } else {
      statusMessage("Temperature Response Sent", TFT_BLACK);
    }

  } else {
    #ifdef DEBUG
    Serial.println("ERROR: Temperature resource only supports GET");
    #endif
  }
}

//call back function for humidity, similar to temperatures
void callbackHumidity(Coap::Message &message, IPAddress ip, uint16_t port) {

  if (message.getCode() == Coap::MessageCode::GET) {
    if (message.isObserveRegister()) {
      statusMessage("Humidity Registration", TFT_BLACK);
    }

    // service unavailable, returns nothing
    if (isnan(currentHumidity)) {
      Coap::Message response;
      response.intoResponse(message, Coap::MessageCode::SERVICE_UNAVAILABLE);
      if (coapNode.sendMessage(response, ip, port) == Coap::ErrorCode::OK) {
        statusMessage("Humidity Response Sent", TFT_BLACK);
      }
      return;
    }

    Coap::Message response;
    response.intoResponse(message, Coap::MessageCode::CONTENT);
    addSensorPayload(response, currentHumidity);

    if (message.isObserveRegister()) {
      Coap::Observer new_observer(ip, port, message.getToken(), message.getTokenLength());
      Coap::ErrorCode err = humidityObservers.add(new_observer);
      if (err == Coap::ErrorCode::OK) {
        response.intoNotification(new_observer);
        response.setType(Coap::MessageType::ACK);
      }
      else {
        response.intoResponse(
          message, Coap::MessageCode::SERVICE_UNAVAILABLE
        );
      }

    } else if (message.isObserveDeregister()) {
      Coap::ErrorCode err = humidityObservers.remove(ip, port, message.getToken(), message.getTokenLength());

      if (err == Coap::ErrorCode::OK) {
        response.intoResponse(message, Coap::MessageCode::VALID);
      }
      else {
        response.intoResponse(message, Coap::MessageCode::SERVICE_UNAVAILABLE);
      }
    }

    if (coapNode.sendMessage(response, ip, port) != Coap::ErrorCode::OK) {
      #ifdef DEBUG
      Serial.println("ERROR: Failed to send humidity response");
      #endif
    } else {
      statusMessage("Humidity Response Sent", TFT_BLACK);
    }

  } else {
    #ifdef DEBUG
    Serial.println("ERROR: Humidity resource only supports GET");
    #endif
  }
}

void callbackClientName(Coap::Message &message, IPAddress ip, uint16_t port) {

  if (message.getCode() == Coap::MessageCode::PUT) {
    statusMessage("Client Name Received", TFT_BLACK);

    const uint8_t *putPayload;
    size_t putPayloadLength;
    message.getPayload(putPayload, putPayloadLength);

    // Leave room for the null terminator.
    size_t nameLength = putPayloadLength;

    if (nameLength >= sizeof(clientName)) {
      nameLength = sizeof(clientName) - 1;
    }

    memcpy(clientName, putPayload, nameLength);
    clientName[nameLength] = '\0';

    displayClientName();

    Coap::Message response;
    response.intoResponse(message, Coap::MessageCode::CHANGED);
    response.addPayload((const uint8_t *)clientName, strlen(clientName), Coap::ContentFormat::TEXT_PLAIN);

    if (coapNode.sendMessage(response, ip, port) != Coap::ErrorCode::OK) {
      #ifdef DEBUG
      Serial.println("ERROR: Failed to send client name response");
      #endif
    }

    #ifdef DEBUG
    Serial.print("STATUS: Received client name: ");
    Serial.println(clientName);
    #endif

  } else if (message.getCode() == Coap::MessageCode::GET) {

    Coap::Message response;
    response.intoResponse(message, Coap::MessageCode::CONTENT);
    response.addPayload((const uint8_t *)clientName, strlen(clientName), Coap::ContentFormat::TEXT_PLAIN);
    coapNode.sendMessage(response, ip, port);

  } else {
    #ifdef DEBUG
    Serial.println("ERROR: Client name resource only supports GET and PUT");
    #endif
  }
}