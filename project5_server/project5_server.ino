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
  Make sure you have your own f26p5config file with
  the UUIDs your MAC address for the server and the 
  server and client name.

  Acknowledgments:
  The code uses code and modifies code from examples
  in class. It also uses generative AI for some portions.

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
#define INTERVAL_SECS 10 // maybe change

// global variable for last update from sensor
unsigned long updateTime = 0;

TFT_eSPI tft;                 // Built-in TFT display

#define DHTTYPE DHT11
#define DHTPIN  0             // This is the pin to use for the digital port (right connector)
DHT dht(DHTPIN, DHTTYPE);

// Declare connection state indicators.
bool isConnected = false;     // Server is connected
bool wasConnected = false;    // Server was connected on last check

// global variables for sensor values
float lastTemp = 0;
float lastHumidity = 0;


// initialization for BLE follow
BLECharacteristic *pTempCharacteristic = NULL;
BLEDescriptor *pTempCCCDDescriptor = NULL;
BLECharacteristic *pHumidityCharacteristic = NULL;
BLEDescriptor *pHumidityCCCDDescriptor = NULL;

// callback routine for temp characteristic. Only defined in deubgging.
class MyTempCallbacks: public BLECharacteristicCallbacks {
  #ifdef DEBUG
  void onRead(BLECharacteristic *pCharacteristic) {
    std::string charValue = pCharacteristic->getValue();
    Serial.println("STATUS: onRead callback - temperature read");
  }

  void onNotify(BLECharacteristic *pCharacteristic) {
      Serial.println("STATUS: onNotify callback - temperature notification");
  }
  #endif
};

// callback routine for humidity characteristic. Only defined in deubgging.
class MyHumidityCallbacks: public BLECharacteristicCallbacks {
  #ifdef DEBUG
  void onRead(BLECharacteristic *pCharacteristic) {
    std::string charValue = pCharacteristic->getValue();
    Serial.println("STATUS: onRead callback - humidity read");
  }

  void onNotify(BLECharacteristic *pCharacteristic) {
      Serial.println("STATUS: onNotify callback - humidity notification");
  }
  #endif
};

class MyServerCallbacks: public BLEServerCallbacks {

  void onConnect(BLEServer *pServer) {
    isConnected = true;
    #ifdef DEBUG
    Serial.println("STATUS: onConnect callback - BLE server connected");
    #endif
  }

  void onDisconnect(BLEServer *pServer) {
    isConnected = false;
    #ifdef DEBUG
    Serial.println("STATUS: onConnect callback - BLE server disconnected");
    #endif
  }
};


void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000));

  displaySplashScreen();
  displayTopReading(SERVER_DEVICE_NAME);
  statusMessage("Initializing", TFT_BLACK);
  delay(2000);

  tft.fillScreen(TFT_BLUE);
  drawGridLayout();
  displayTopReading("Local Data");

  BLEDevice::init(SERVER_DEVICE_NAME);

  // create BLE service, associated with callbacks from server.
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(ENVIRONMENTAL_SENSING_SERVICE_UUID);

  // temp char supports and permits reads and notificaiton requests from client.
  pTempCharacteristic = pService->createCharacteristic(TEMPERATURE_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_NOTIFY);
  pTempCharacteristic->setAccessPermissions(GATT_PERM_READ | GATT_PERM_NOTIF_IND);
  pTempCharacteristic->addDescriptor(new BLE2902()); // allow notifications
  
  pTempCCCDDescriptor = pTempCharacteristic->getDescriptorByUUID("2902");
  pTempCharacteristic->setCallbacks(new MyTempCallbacks()); // set callback routine for temp operations


  // humidity setup
  pHumidityCharacteristic = pService->createCharacteristic(HUMIDITY_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_NOTIFY);

  pHumidityCharacteristic->setAccessPermissions(GATT_PERM_READ | GATT_PERM_NOTIF_IND);
  pHumidityCharacteristic->addDescriptor(new BLE2902()); // allow notifications
  
  pHumidityCCCDDescriptor = pHumidityCharacteristic->getDescriptorByUUID("2902");
  pHumidityCharacteristic->setCallbacks(new MyHumidityCallbacks()); // set callback routine for temp operations

  dht.begin();
  readSensor();
  displayTemperature();
  displayHumidity();

  updateTime = millis();

  uint16_t tempValue = (uint16_t) lastTemp;
  pTempCharacteristic->setValue(tempValue);

  uint16_t humidityValue = (uint16_t) lastHumidity;
  pHumidityCharacteristic->setValue(humidityValue);

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(ENVIRONMENTAL_SENSING_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  #ifdef DEBUG
  Serial.println("STATUS: Server advertising started");

  Serial.print("STATUS: Configured server name: ");
  Serial.println(SERVER_DEVICE_NAME);

  Serial.print("STATUS: ESS UUID: ");
  Serial.println(ENVIRONMENTAL_SENSING_SERVICE_UUID);
  #endif

  statusMessage("Disconnected/Advertising", TFT_BLACK);
}

void loop() {

  if(isConnected && !wasConnected) {
    
    wasConnected = true;
    BLEDevice::stopAdvertising();
    statusMessage("Connected", TFT_BLACK);

  } else if(!isConnected && wasConnected) {

    wasConnected = false;

    uint8_t *cValue;
    cValue = pTempCCCDDescriptor->getValue();
    *cValue = *cValue & 0xFE;
    pTempCCCDDescriptor->setValue(cValue, sizeof(uint8_t));

    uint8_t *humValue;
    humValue = pHumidityCCCDDescriptor->getValue();
    *humValue = *humValue & 0xFE;
    pHumidityCCCDDescriptor->setValue(humValue, sizeof(uint8_t));

    BLEDevice::startAdvertising();

    statusMessage("Disconnected/Advertising", TFT_BLACK);
  }

  if(millis() > updateTime + (1000 * INTERVAL_SECS)) {
    updateTime = updateTime + (1000 * INTERVAL_SECS);

    readSensor();

    uint16_t tempValue = (uint16_t) round(lastTemp * 100.0);
    pTempCharacteristic->setValue(tempValue);
    pTempCharacteristic->notify(true);

    uint16_t humValue = (uint16_t) round(lastHumidity * 100.0);
    pHumidityCharacteristic->setValue(humValue);
    pHumidityCharacteristic->notify(true);

    displayTemperature();
    displayHumidity();
  }

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
void displayTopReading(const char* message) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);
  tft.drawString(message, 120, 10);
}

// reads temp & humidity sensor
void readSensor() {
  float bothTempHumid[2] = {0.0};

  if (!dht.readTempAndHumidity(bothTempHumid)) {
    lastHumidity = bothTempHumid[0];
    lastTemp = bothTempHumid[1];

    #ifdef DEBUG
    Serial.print("Temperature: ");
    Serial.println(lastTemp);

    Serial.print("Humidity: ");
    Serial.println(lastHumidity);
    #endif
  }
  else {
    lastTemp = 0.0;
    lastHumidity = 0.0;

    #ifdef DEBUG
    Serial.println("ERROR: Failed to read DHT11 sensor");
    #endif
  }
}

// displays the temperature
void displayTemperature() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);
  
  tft.drawString("Temperature", 80, 65);
  tft.drawString("Celsius", 80, 150);
  
  char valueBuffer[16];
  dtostrf(lastTemp, 3, 1, valueBuffer);
  tft.fillRect(5, 80, 150, 50, TFT_BLUE);
  tft.setFreeFont(FSSB24);
  tft.drawString(valueBuffer, 80, 105);
}

// displays humidity data
void displayHumidity() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);
  
  tft.drawString("Humidity", 240, 65);
  tft.drawString("Percent", 240, 150);
  
  char valueBuffer[16];
  dtostrf(lastHumidity, 3, 1, valueBuffer);
  tft.fillRect(165, 80, 150, 50, TFT_BLUE);

  tft.setFreeFont(FSSB24);
  tft.drawString(valueBuffer, 240, 105);
}