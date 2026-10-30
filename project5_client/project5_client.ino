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

#include <rpcBLEDevice.h>         // rpcBLE device header file
#include <BLEScan.h>              // rpcBLE scanner header file
#include <BLEAdvertisedDevice.h>  // rpcBLE advertised device header file
#include "TFT_eSPI.h"             // TFT LCD header file
#include "Free_Fonts.h"           // Free fonts header 

//config containing UUIDs, MAC address, and server/client name
#include "f26p5config.h"

#define DEBUG

#define DELAY_FOR_STATE  2


#define BLE_SCAN_DURATION 10

#define STATE_INIT      0     // Initializing
#define STATE_SCAN      1     // Scanning
#define STATE_SCANFAIL  2     // Scan failed
#define STATE_SERVFND   3     // Server found
#define STATE_CONN      4     // Connected
#define STATE_ACTIVE    5     // Connection active
#define STATE_CONNFAIL  6     // Connection failed
#define STATE_DISCONN   7     // Disconnected
static int clientState = STATE_INIT;

// instantiate tft display
TFT_eSPI tft;

unsigned long scanFailedTime = 0;

// UUID values used at server in config file. Converted to BLEUUID values
static BLEUUID serviceUUID(ENVIRONMENTAL_SENSING_SERVICE_UUID);
static BLEUUID temperatureUUID(TEMPERATURE_CHARACTERISTIC_UUID);
static BLEUUID humidityUUID(HUMIDITY_CHARACTERISTIC_UUID);

// global variables for pointers to temp and humidity charactersits and server device
static BLERemoteCharacteristic* pRemoteTempCharacteristic = NULL;
static BLERemoteCharacteristic* pRemoteHumidityCharacteristic = NULL;
static BLEAdvertisedDevice* myDevice = NULL;

// global variables for clients scanner and scanning for a target server
static BLEScan* pBLEScan;

// BLE device is identified by 48 bit MAC address fo servers BLE interface.
BLEAddress THServer(bd_addr);

// callback routine for new temperature data from server from notiifcaiton.
static void notifyTempCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    char buffer[4];
    sprintf(buffer, "%d", (uint16_t) (*pData));  
    displayTemperatureValue((uint16_t)(*pData));
  }

// callback routine for new humidity data from server from notiifcaiton.
static void notifyHumidityCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    char buffer[4];
    sprintf(buffer, "%d", (uint16_t) (*pData));  
    displayHumidityValue((uint16_t)(*pData));
  }


// callback routine for client device. 
class MyClientCallback : public BLEClientCallbacks {
  
  void onConnect(BLEClient* pclient) {
    #ifdef DEBUG
    Serial.println("STATUS: onConnect called - client is connected");
    #endif
  }

  void onDisconnect(BLEClient* pclient) {
    clientState = STATE_DISCONN;
    statusMessage("Disconnected", TFT_BLACK);

    #ifdef DEBUG
    Serial.println("STATUS: onDisconnect called - client is disconnected");
    #endif
  }
};


//callback routine for scanning. Checks if address of advertised device matches
// target of client
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    #ifdef DEBUG
    Serial.print("STATUS: onResult callback - advertised BLE device: ");
    Serial.println(advertisedDevice.toString().c_str());
    #endif

    Serial.print("Configured server address: ");
    Serial.println(THServer.toString().c_str());

    Serial.print("Detected address: ");
    Serial.println(advertisedDevice.getAddress().toString().c_str());

    Serial.print("Detected name: ");
    Serial.println(advertisedDevice.getName().c_str());

    if(advertisedDevice.getAddress().equals(THServer)) {
      #ifdef DEBUG
      Serial.print("STATUS: Target BLE server found - ");
      Serial.println(advertisedDevice.toString().c_str());
      #endif

      BLEDevice::getScan()->stop();

      myDevice = new BLEAdvertisedDevice(advertisedDevice);

      clientState = STATE_SERVFND;

      statusMessage("Server Found", TFT_BLACK);
    }
  }
};


void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);
  #endif

  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  displaySplashScreen();
  delay(2000);
  drawGridLayout();
  displayTopReading(CLIENT_DEVICE_NAME);

  clientState = STATE_INIT;
  statusMessage("Initializing", TFT_BLACK);
  delay(2000);

  BLEDevice::init(CLIENT_DEVICE_NAME);

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  clientState = STATE_SCAN;
  statusMessage("Scanning", TFT_BLACK);

  scanFailedTime = millis() + (BLE_SCAN_DURATION * 1000) + 1000;

  pBLEScan->start(BLE_SCAN_DURATION, false);
}

void loop() {

  if(clientState == STATE_SERVFND) {
    if (connectToServer()) {
      #ifdef DEBUG
      Serial.println("STATUS: Client connected to remote BLE server");
      #endif

      // Update state and display.
      clientState = STATE_ACTIVE;
      statusMessage("Connection Active", TFT_BLACK);
    }
    else {
      #ifdef DEBUG
      Serial.println("ERROR: Client failed to connect to the server, so stopping");
      #endif

      // Update state and display;
      clientState = STATE_CONNFAIL;
      statusMessage("Connection Failed", TFT_BLACK);
    }
  } else if(clientState == STATE_DISCONN) {
    #ifdef DEBUG
    Serial.println("STATUS: Client disconnected, pausing and will rescan for target server");
    #endif

    delay(20000);

    clientState = STATE_SCAN;
    statusMessage("Scanning", TFT_BLACK);

    pBLEScan->start(BLE_SCAN_DURATION, false);
    scanFailedTime = millis() + (BLE_SCAN_DURATION * 1000) + 1000;
  } else if(clientState == STATE_SCAN && scanFailedTime < millis()) {
    #ifdef DEBUG
    Serial.println("ERROR: Scanning ended without finding a target device, so stopping");
    #endif

    clientState = STATE_SCANFAIL;
    statusMessage("Scan Failed", TFT_BLACK);
  }

  delay(1000);
}


// Displays start screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);

  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT/2, TFT_WIDTH/2 - 75);
  tft.drawString("Project 5 BLE Client", TFT_HEIGHT/2, TFT_WIDTH/2 - 45);
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
  tft.fillScreen(TFT_BLUE);

  tft.drawFastHLine(0, 40, 320, TFT_WHITE);
  tft.drawFastHLine(0, 170, 320, TFT_WHITE);
  tft.drawFastVLine(160, 40, 130, TFT_WHITE);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);

  tft.drawString("Temperature", 80, 65);
  tft.drawString("Humidity", 240, 65);

  tft.drawString("Celsius (C)", 80, 150);
  tft.drawString("Percent (%)", 240, 150);
}

// displays server name
void displayTopReading(const char* message) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);
  tft.drawString(message, 120, 10);
}

// displays temperature value
void displayTemperatureValue(uint16_t value) {
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "%u", (unsigned int)value);
  tft.fillRect(5, 80, 150, 50, TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB24);
  tft.drawString(buffer, 80, 105);
}

// displays humidity value
void displayHumidityValue(uint16_t value) {
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "%u", (unsigned int)value);
  tft.fillRect(165, 80, 150, 50, TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB24);
  tft.drawString(buffer, 240, 105);
}


// connects to server once advertised device is found that matches target 
// MAC address. returns false if it fails if any of verifications.
bool connectToServer() {
  #ifdef DEBUG
  Serial.print("STATUS: Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  #endif


  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  if(pClient->connect(myDevice)) {
    #ifdef DEBUG
    Serial.println("STATUS: Connected to server, but have not verified service, etc.");
    #endif

    clientState = STATE_CONN;
    statusMessage("Connected", TFT_BLACK);
  } else {
    #ifdef DEBUG
    Serial.println("ERROR: Connect to server failed");
    #endif

    return(false);
  }

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if(pRemoteService == nullptr){
    #ifdef DEBUG
    Serial.print("ERROR: Failed to find service with UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    #endif

    // Disconnect and return false.
    pClient->disconnect();
    return false;
  }

  displayTopReading(myDevice->getName().c_str());

  #ifdef DEBUG  
  Serial.print("STATUS: Connected to server ");
  Serial.print(myDevice->getName().c_str());
  Serial.print(" with service UUID ");
  Serial.println(serviceUUID.toString().c_str());
  #endif

  pRemoteTempCharacteristic = pRemoteService->getCharacteristic(temperatureUUID);
  pRemoteHumidityCharacteristic = pRemoteService->getCharacteristic(humidityUUID);

  if(pRemoteTempCharacteristic == nullptr) {
    #ifdef DEBUG
    Serial.print("ERROR: Failed to find temperature characteristic UUID ");
    Serial.println(temperatureUUID.toString().c_str());
    #endif

    pClient->disconnect();
    return false;
  } else {
    #ifdef DEBUG
    Serial.print("STATUS: Found temperature characteristic UUID ");
    Serial.println(temperatureUUID.toString().c_str());
    #endif
  }

  if(pRemoteHumidityCharacteristic == nullptr) {
    #ifdef DEBUG
    Serial.println("ERROR: Humidity characteristic not found");
    #endif

    pClient->disconnect();
    return false;
  }

  // read vlaue of remp characteristic/display. fails if can't be read
  if(pRemoteTempCharacteristic->canRead()) {
    #ifdef DEBUG
    Serial.println("STATUS: Can read temperature characteristic");
    #endif

    uint16_t tempValue = pRemoteTempCharacteristic->readUInt16();
    char tempBuffer[8];
    sprintf(tempBuffer, "%d", tempValue);
    displayTemperatureValue(tempValue);

    #ifdef DEBUG
    Serial.print("STATUS: Temperature is ");
    Serial.print(tempBuffer);
    Serial.println(" C");
    #endif
  }
  else {
    #ifdef DEBUG
    Serial.println("ERROR: Cannot read temperature characteristic");
    #endif

    // Disconnect and return false.
    pClient->disconnect();
    return false;
  }

    // Read the initial humidity value.
  if (pRemoteHumidityCharacteristic->canRead()) {
    uint16_t humidityValue =
      pRemoteHumidityCharacteristic->readUInt16();

    displayHumidityValue(humidityValue);

    #ifdef DEBUG
    Serial.print("STATUS: Humidity is ");
    Serial.print(humidityValue);
    Serial.println(" %");
    #endif
  }
  else {
    #ifdef DEBUG
    Serial.println("ERROR: Cannot read humidity characteristic");
    #endif

    pClient->disconnect();
    return false;
  }
    
  // register for notifications for temp characteristic
  if(pRemoteTempCharacteristic->canNotify()) {
    #ifdef DEBUG
    Serial.println("STATUS: Can use notifications for temperature");
    #endif

    pRemoteTempCharacteristic->registerForNotify(notifyTempCallback);
  }
  else {
    #ifdef DEBUG
    Serial.println("ERROR: Notifications not supported for temperature");
    #endif

    pClient->disconnect();
    return false;
  }  

  // register for humidity notifications.
  if (pRemoteHumidityCharacteristic->canNotify()) {
    pRemoteHumidityCharacteristic->registerForNotify(
      notifyHumidityCallback
    );
  }
  else {
    #ifdef DEBUG
    Serial.println("ERROR: Humidity notifications not supported");
    #endif

    pClient->disconnect();
    return false;
  }

  return true;
}

