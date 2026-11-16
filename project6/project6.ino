/**
  Class: ECE 5984 Special Study: IOT system design
  Project 6: LoRaWAN End Device and The Things Network
  Ben Seidel
  benseidel@vt.edu

  Required libraries/files:
  TFT_eSPI
  Free_Fonts
  SoftwareSerial
  DHT sensor library
  End_Device_Configuration.h

  Compiling/Uploading:
  Make sure you have your own End_Device_Confirguration.h with
  your DEV_EUI with and without colons. Also, make sur eyou have
  an APP_KEY generated from TTN and your kit number (MY_KIT)

  Acknowledgments:
  The code uses and modifies code from the examples in class.
  It also uses generative AI for portions
*/

#include <SoftwareSerial.h>
#include <DHT.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"
#include "End_Device_Configuration.h"

TFT_eSPI tft;

// left part of WIO terminal for LORA
SoftwareSerial loraE5(BCM3, BCM2, false);

// right part of LORA for temp & humidity
#define DHTPIN 0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define UPDATE_PERIOD_MIN 2
#define AT_TIME_LOCAL 1000
#define AT_TIME_JOIN 60000
#define AT_TIME_SEND 60000
#define BUFFER_LENGTH 512

static bool isJoined = false; // joined network
static char respBuffer[BUFFER_LENGTH]; //holds response
static uint16_t myCount = 0;
unsigned long updateTime;
unsigned long updatePeriod = UPDATE_PERIOD_MIN * 60 * 1000;



void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000));

  dht.begin();

  displaySplashScreen();
  statusMessage("Initializing", TFT_BLACK);
  delay(2000);

  String kitLabel = "Kit " + String(MY_KIT);
  displayTopReading(kitLabel.c_str());

  loraE5.begin(9600); // start UART

  if (!at_send_check_response("+AT: OK", AT_TIME_LOCAL, "AT\r\n")) {
    statusMessage("E5 Not Found", TFT_RED);
    Serial.println("ERROR: No Grove Wio-E5 module found.");
    while (1) delay(1000);
  }

  statusMessage("Configuring", TFT_BLACK);
  configureWioE5();
  statusMessage("LoRaWAN Configured", TFT_BLACK);
  delay(1000);

  updateTime = millis();
}

void loop() {

  // try to join if not already joined
  if (!isJoined) {
    statusMessage("Joining LoRaWAN", TFT_BLACK);
    Serial.println("STATUS: Attempting to join LoRaWAN.");

    if (at_send_check_response("+JOIN: Done", AT_TIME_JOIN, "AT+JOIN\r\n") &&
        strstr(respBuffer, "+JOIN: Network joined") != NULL) {
      isJoined = true;
      statusMessage("Joined LoRaWAN", TFT_BLACK);
      Serial.println("STATUS: LoRaWAN network joined.");
      delay(1000);
    } else {
      statusMessage("Join Failed", TFT_RED);
      Serial.println("ERROR: Join failed. Will retry later.");
      at_send_check_response("+ID:", AT_TIME_LOCAL, "AT+ID\r\n");
    }
  }

  // send values if joined
  if (isJoined) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    uint16_t light = analogRead(WIO_LIGHT);

    if (isnan(temperature) || isnan(humidity)) {
      statusMessage("Sensor Read Failed", TFT_RED);
      Serial.println("ERROR: DHT11 reading failed.");
    } else {
      uint16_t temperaturePacked = (uint16_t)round(temperature * 100.0f);
      uint16_t humidityPacked = (uint16_t)round(humidity * 100.0f);

      displaySensorValues(temperature, humidity, light);
      statusMessage("Data Update", TFT_BLACK);

      char cmd[80];
      sprintf(cmd, "AT+CMSGHEX=\"%02X%04X%04X%04X\"\r\n",
              (uint8_t)MY_KIT, temperaturePacked, humidityPacked, light);

      // make sure temps are correct (DEBUG)
      Serial.print("Temperature: ");
      Serial.print(temperature, 1);
      Serial.println(" C");
      Serial.print("Humidity: ");
      Serial.print(humidity, 1);
      Serial.println(" %");
      Serial.print("Light: ");
      Serial.println(light);
      Serial.print("Payload: ");
      Serial.println(cmd);

      if (at_send_check_response("+CMSGHEX: Done", AT_TIME_SEND, cmd)) {
        int port = 0;
        int data = 0;
        int rssi = 0;
        int snr = 0;
        if (recv_parse(respBuffer, &port, &data, &rssi, &snr)) {
          Serial.print("Downlink received on port ");
          Serial.print(port);
          Serial.print(": 0x");
          Serial.println(data, HEX);
        }
        statusMessage("Message Sent", TFT_BLACK);
        Serial.println("STATUS: Message sent successfully.");
        Serial.print("RSSI: ");
        Serial.println(rssi);
        Serial.print("SNR: ");
        Serial.println(snr);
        myCount++;
      } else {
        statusMessage("Send Failed", TFT_RED);
        Serial.println("ERROR: Message send failed.");
      }
    }
  }

  if (millis() < updateTime + updatePeriod) {
    delay((updateTime + updatePeriod) - millis());
  }
  updateTime += updatePeriod;
}

// displays splash screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB12);
  tft.drawString("ECE 5984 Fall 2026", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 75);
  tft.drawString("Project 6", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 15);
  tft.drawString("benseidel@vt.edu", TFT_HEIGHT / 2, TFT_WIDTH / 2 + 15);
}

// displays status messages
void statusMessage(const char *message, uint16_t color) {
  uint16_t lightBlue = tft.color565(135, 206, 250);
  tft.fillRect(30, 185, 260, 40, lightBlue);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB9);
  tft.setTextColor(color);
  tft.drawString(message, 160, 205);
}

// displays topreading (kit number)+ sensor labels
void displayTopReading(const char *message) {
  tft.fillRect(0, 0, 320, 185, TFT_BLUE);
  tft.drawFastHLine(0, 35, 320, TFT_WHITE);
  tft.drawFastHLine(0, 85, 320, TFT_WHITE);
  tft.drawFastHLine(0, 135, 320, TFT_WHITE);
  tft.drawFastHLine(0, 180, 320, TFT_WHITE);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB12);
  tft.drawString(message, 160, 18);
  displaySensorLabels();
}

// displays sensor labels (temp, humidity, light)
void displaySensorLabels() {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);
  tft.drawString("Temperature:", 15, 48);
  tft.drawString("Humidity:", 15, 98);
  tft.drawString("Light:", 15, 148);
}

// displays sensor values (temp, humidity, light)
void displaySensorValues(float temperature, float humidity, uint16_t light) {
  char value[24];
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSSB9);

  tft.fillRect(145, 36, 175, 49, TFT_BLUE);
  tft.fillRect(145, 86, 175, 49, TFT_BLUE);
  tft.fillRect(145, 136, 175, 44, TFT_BLUE);

  sprintf(value, "%.1f C", temperature);
  tft.drawString(value, 305, 48);
  sprintf(value, "%.1f %%", humidity);
  tft.drawString(value, 305, 98);
  sprintf(value, "%u", light);
  tft.drawString(value, 305, 148);
}

// Used example code for this
void configureWioE5() {
  at_send_check_response("+UART: TIMEOUT, 0", AT_TIME_LOCAL, "AT+UART=TIMEOUT,0\r\n");
  at_send_check_response("+MODE: LWOTAA", AT_TIME_LOCAL, "AT+MODE=LWOTAA\r\n");
  at_send_check_response("+DR: US915", AT_TIME_LOCAL, "AT+DR=US915\r\n");
  at_send_check_response("+CH: NUM", AT_TIME_LOCAL, "AT+CH=NUM,8-15\r\n");
  at_send_check_response("+ADR: ON", AT_TIME_LOCAL, "AT+ADR=ON\r\n");
  at_send_check_response("+CLASS: A", AT_TIME_LOCAL, "AT+CLASS=A\r\n");

  char cmd[100];
  char ack[100];

  sprintf(cmd, "AT+ID=DevEUI,\"%s\"\r\n", MY_DEV_EUI);
  sprintf(ack, "+ID: DevEui, %s", MY_DEV_EUI_COLONS);
  at_send_check_response(ack, AT_TIME_LOCAL, cmd);

  at_send_check_response("+ID: AppEui", AT_TIME_LOCAL,
                         "AT+ID=AppEUI,\"0000000000000000\"\r\n");

  sprintf(cmd, "AT+KEY=APPKEY,\"%s\"\r\n", MY_APP_KEY);
  at_send_check_response("+KEY: APPKEY", AT_TIME_LOCAL, cmd);
  at_send_check_response("+PORT: 8", AT_TIME_LOCAL, "AT+PORT=8\r\n");
}


// handles sending an AT command to the WIO-E5 module. Checks for response and saves
// response for additional processing.
static bool at_send_check_response(const char *ptrAck, int respTimeoutMs, const char *ptrCmd) {
  int index = 0;
  unsigned long startMillis;
  memset(respBuffer, 0, BUFFER_LENGTH);

  loraE5.print(ptrCmd);
  Serial.print(ptrCmd);
  delay(200);
  startMillis = millis();

  while (millis() - startMillis < (unsigned long)respTimeoutMs) {
    while (index < BUFFER_LENGTH - 1 && loraE5.available() > 0) {
      char ch = (char)loraE5.read();
      respBuffer[index++] = ch;
      respBuffer[index] = '\0';
      Serial.print(ch);
    }
    if (strstr(respBuffer, ptrAck) != NULL) return true;
    if (strstr(respBuffer, "+JOIN: Join failed") != NULL) return false;
    if (strstr(respBuffer, "+CMSGHEX: Length error") != NULL ||
        strstr(respBuffer, "+CMSGHEX: ERROR") != NULL ||
        strstr(respBuffer, "RERROR") != NULL) return false;
    delay(2);
  }
  return false;
}

// Used example from class
// parses received response from sending uplink message. Extracts any downlink data.
static bool recv_parse(char *ptrMsg, int *pPort, int *pData, int *pRssi, int *pSnr) {
  if (ptrMsg == NULL || pPort == NULL || pData == NULL || pRssi == NULL || pSnr == NULL) {
    return false;
  }

  char *pStart = strstr(ptrMsg, "PORT:");
  bool dataFound = false;
  *pPort = 0;
  *pData = 0;
  *pRssi = 0;
  *pSnr = 0;

  if (pStart != NULL) {
    sscanf(pStart, "PORT: %d", pPort);
    pStart = strstr(ptrMsg, "RX:");
    unsigned int receivedHex;
    if (pStart != NULL && sscanf(pStart, "RX: \"%x\"", &receivedHex) == 1) {
      *pData = (int)receivedHex;
      dataFound = true;
    }
  }

  pStart = strstr(ptrMsg, "RSSI");
  if (pStart != NULL) sscanf(pStart, "RSSI %d", pRssi);

  pStart = strstr(ptrMsg, "SNR");
  if (pStart != NULL) sscanf(pStart, "SNR %d", pSnr);

  return dataFound;
}
