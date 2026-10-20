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
  Grove Temperature and Humidity sensor Library
  ProsecCoAP library

  Compiling/Uploading:
  Make sure you have your own CoAPClientConfig file with
  your cilents name, the servers IP address, and the servers
  access point for the SSID and passowrd. Run this code on a
  separate WIO termirnal after you have started the server.

  Acknowledgments:
  The code uses and modifies code from the examples in class.
  It also uses generative AI for portions of the getResource.
*/

#include "TFT_eSPI.h"       // TFT LCD header file
#include "Free_Fonts.h"     // Free fonts header file (needs to be in directory with this file)
#include <rpcWiFi.h>        //WiFi
#include <WiFiUDP.h>        //UDP
#include <ProsecCoAP.h>     //CoAP
#include "CoAPClientConfig.h"

#define DEBUG

#define WIFI_WAIT_SEC 30
#define LOCAL_PORT 59840

// tokens for resources
#define TOKEN_CLIENTNAME 1
#define TOKEN_TEMPERATURE 2
#define TOKEN_HUMIDITY 3

// spacing of temp and humidity
const uint32_t REQUEST_INTERVAL = 2500;

char ssid[] = WIFI_SSID;
char password[] = WIFI_PASSWORD;
char theClientName[] = CLIENT_NAME;

// server ip, set in config
IPAddress destinationIp = SERVER_IP_ADDRESS;

// received values
float currentTemperature = NAN;
float currentHumidity = NAN;

// request state
bool sentPutRequest = false;
bool sentTemperatureRequest = false;
bool sentHumidityRequest = false;

uint32_t lastRequestTime = 0;

// instantiate udp, coap, and tft objects
WiFiUDP Udp;
Coap::Node coapNode(Udp, LOCAL_PORT);
TFT_eSPI tft;


void setup() {
  Serial.begin(115200);
  uint32_t serialStart = millis();
  while (!Serial && millis() - serialStart < 2000);

  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  displaySplashScreen();
  delay(3000);

  tft.fillScreen(TFT_BLUE);
  drawGridLayout();
  displayClientName();
  displayTemperature();
  displayHumidity();

  // connect to servers wifi AP
  statusMessage("Connecting to WiFi", TFT_BLACK);
  delay(2000);

  if (!connectWiFi()) {
    statusMessage("WiFi Failed", TFT_BLACK);
    while (1) {delay(10000);}
  }

  statusMessage("Connected to WiFi", TFT_BLACK);
  delay(2000);

  IPAddress localIP = WiFi.localIP();
  char message[64];
  snprintf(message, sizeof(message), "Local IP: %d.%d.%d.%d", 
    localIP[0], localIP[1], localIP[2], localIP[3]);

  statusMessage(message, TFT_BLACK);
  delay(2000);

  // register callback for coap responses
  coapNode.setResponseHandler(callbackResponse);

  Coap::ErrorCode coapErrorCode = coapNode.start();

  if ((int8_t)coapErrorCode != 0) {
    statusMessage("CoAP Failed", TFT_BLACK);
    while (1) {delay(10000);}
  }
}

void loop() {
  uint32_t now = millis();

  // sends clients name
  if (!sentPutRequest) {
    putClientName();
    sentPutRequest = true;
    lastRequestTime = millis();
  } else if (!sentTemperatureRequest &&
           now - lastRequestTime >= REQUEST_INTERVAL) {

    getResource("temperature", TOKEN_TEMPERATURE);
    sentTemperatureRequest = true;
    lastRequestTime = millis();
  } else if (!sentHumidityRequest &&
           now - lastRequestTime >= REQUEST_INTERVAL) {

    getResource("humidity", TOKEN_HUMIDITY);
    sentHumidityRequest = true;
    lastRequestTime = millis();
  }

  coapNode.loop();
}

// helper function to write status messages
void statusMessage(const char *message, uint16_t color) {
  uint16_t lightBlue = tft.color565(135, 206, 250);
  tft.fillRect(30, 185, 260, 40, lightBlue);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB9);
  tft.setTextColor(color, lightBlue);
  tft.drawString(message, 160, 205);
}

// Displays start screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);

  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 75);
  tft.drawString("Project 4 CoAP Client", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 15);
  tft.drawString("benseidel@vt.edu", TFT_HEIGHT / 2, TFT_WIDTH / 2 + 15);
}

// Draws grid around temperature and humidity data
void drawGridLayout() {
  tft.drawFastHLine(0, 40, 320, TFT_WHITE);
  tft.drawFastHLine(0, 145, 320, TFT_WHITE);
  tft.drawFastVLine(160, 40, 105, TFT_WHITE);
  tft.drawFastHLine(0, 175, 320, TFT_WHITE);
}

// displays client name
void displayClientName() {
  tft.fillRect(0, 0, 320, 40, TFT_BLUE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setFreeFont(FSSB9);
  char text[48];
  snprintf(text, sizeof(text), "Client %s", theClientName);
  tft.drawString(text, 10, 10);
}

// displays the temperature
void displayTemperature() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setFreeFont(FSSB9);
  tft.drawString("Temperature", 80, 55);
  tft.fillRect(5, 65, 150, 45, TFT_BLUE);

  if (!isnan(currentTemperature)) {
    char value[16];
    dtostrf(currentTemperature, 3, 1, value);

    tft.setFreeFont(FSSB24);
    tft.drawString(value, 80, 90);
  }

  tft.setFreeFont(FSSB9);
  tft.drawString("Fahrenheit", 80, 125);
}

// displays the humidity
void displayHumidity() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setFreeFont(FSSB9);
  tft.drawString("Humidity", 240, 55);
  tft.fillRect(165, 65, 150, 45, TFT_BLUE);

  if (!isnan(currentHumidity)) {
    char value[16];
    dtostrf(currentHumidity, 3, 1, value);
    tft.setFreeFont(FSSB24);
    tft.drawString(value, 240, 90);
  }
  tft.setFreeFont(FSSB9);
  tft.drawString("Percent", 240, 125);
}


// puts the clients name to the servers clientname resource
void putClientName() {

  Coap::Message msg(Coap::MessageType::CON, Coap::MessageCode::PUT);
  msg.addPath("clientname"); // path
  msg.addPayload((const uint8_t *)theClientName, strlen(theClientName), Coap::ContentFormat::TEXT_PLAIN);

  uint8_t token = TOKEN_CLIENTNAME;
  msg.setToken(&token, sizeof(token));

  Coap::ErrorCode err = coapNode.sendMessage(msg, destinationIp, COAP_DEFAULT_PORT);
  statusMessage("PUT Client Name", TFT_BLACK);

  #ifdef DEBUG // debug used to make sure put goes through
  if (err == Coap::ErrorCode::OK) {
    Serial.println("STATUS: PUT clientname sent");
    Serial.print(" * Name: ");
    Serial.println(theClientName);
  } else {
    Serial.print("ERROR: PUT failed: ");
    Serial.println((int8_t)err);
  }
  #endif
}

// sends GET request with OBSERVER for resouce
void getResource(const char *resource, uint8_t token) {

  Coap::Message msg(Coap::MessageType::CON, Coap::MessageCode::GET);
  msg.addPath(resource);

  uint8_t observeOptionValue = (uint8_t)Coap::ObserveValue::REGISTER;
  msg.addOption(Coap::OptionNumber::OBSERVE, &observeOptionValue, sizeof(observeOptionValue));
  msg.setToken(&token, sizeof(token));

  Coap::ErrorCode err = coapNode.sendMessage(msg, destinationIp, COAP_DEFAULT_PORT);

  if (strcmp(resource, "temperature") == 0) { // compares resource and temperature
    statusMessage("GET Temperature", TFT_BLACK);
  } else {
    statusMessage("GET Humidity", TFT_BLACK);
  }

  // debug to make sure GET/Observe sent
  #ifdef DEBUG
  if (err == Coap::ErrorCode::OK) {
    Serial.print("STATUS: GET with OBSERVE sent for ");
    Serial.println(resource);
    Serial.print(" * Token: ");
    Serial.println(token);
  } else {
    Serial.print("ERROR: GET failed for ");
    Serial.print(resource);
    Serial.print(", error: ");
    Serial.println((int8_t)err);
  }
  #endif
}

void callbackResponse(Coap::Message &message, IPAddress ip, uint16_t port) {
  processResponse(message, ip, port);
}

void processResponse(Coap::Message &message, IPAddress ip, uint16_t port) {

  const uint8_t *payload;
  size_t payloadLength;
  message.getPayload(payload, payloadLength);

  Coap::Message response;
  const uint8_t *tokenPtr = message.getToken();
  size_t tokenLength = message.getTokenLength();

  // ensure reqest 1 byte
  if (tokenLength != 1) {
    #ifdef DEBUG
    Serial.println("ERROR: Unexpected CoAP token length");
    #endif
    return;
  }

  uint8_t token = *tokenPtr;
  if (message.getType() == Coap::MessageType::CON) {
    response.intoResponse(message, Coap::MessageCode::CONTENT, Coap::MessageType::ACK);
  }

  if (token == TOKEN_TEMPERATURE || token == TOKEN_HUMIDITY) {

    // make sure data is nbo liek sent
    if (payloadLength != sizeof(uint32_t)) {
      #ifdef DEBUG
      Serial.println("ERROR: Invalid sensor payload length");
      #endif
      return;
    }

    uint32_t scaledValue = Coap::Utils::read_uint32(payload);
    float value = scaledValue / 10.0f;

    if (token == TOKEN_TEMPERATURE) {
      currentTemperature = value;
      displayTemperature();
      statusMessage("Temperature Received", TFT_BLACK);
    } else {
      currentHumidity = value;
      displayHumidity();
      statusMessage("Humidity Received", TFT_BLACK);
    }

    if (message.getType() == Coap::MessageType::CON) {
      response.addPayload(payload, payloadLength, Coap::ContentFormat::APPLICATION_OCTET_STREAM);
    }

  } else if (token == TOKEN_CLIENTNAME) {
    statusMessage("Client Name Received", TFT_BLACK);
  } else {

    #ifdef DEBUG
    Serial.print("ERROR: Invalid token value received: ");
    Serial.println(token);
    #endif

    if (message.getType() == Coap::MessageType::CON) {
      response.intoResponse(message, Coap::MessageCode::EMPTY, Coap::MessageType::RST);
    }
  }

  if (message.getType() == Coap::MessageType::CON) {
    coapNode.sendMessage(response, ip, port);
  }
}

// connect wifi using SSID and password specified.
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (strlen(password) > 0) {
    WiFi.begin(ssid, password);
  } else {
    WiFi.begin(ssid);
  }

  uint32_t timeout = millis() + 1000 * WIFI_WAIT_SEC;

  while (WiFi.status() != WL_CONNECTED && (int32_t)(timeout - millis()) > 0) {
          delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  return true;
}