/**
  Class: ECE 5984 Special Study: IOT system design
  Project 2: Network Time Display
  Ben Seidel
  benseidel@vt.edu
  
  Required libraries/files:
  TODO

  Compiling/Uploading:
  TODO

  Acknowledgments:
  TODO
*/


#include "TFT_eSPI.h"                               // TFT LCD header file
#include "Free_Fonts.h"                             // Free fonts header file (needs to be in directory with this file)
#include "rpcWiFi.h"                                // WiFi library
#include "RTC_SAMD51.h"                             // Real Time Clock
#include "DateTime.h"                               // Data + Time

#include "f26p2config.h"                            // Personal WiFi info

char ssid[] = WIFI_SSID;
char password[] = WIFI_PASSWORD;
int offsetFromUTC = TIME_OFFSET;

// one hour offset (in seconds) for adjusting time
#define ONE_HOUR 3600UL

// number of WiFi attempts before message appears
#define WIFI_ATTEMPTS 2

// number of NTP server attempts before message appears
#define NTP_SERVER_ATTEMPTS 2

// top of display
#define VALUE_TOP 90

TFT_eSPI tft;       // Built-in TFT display
RTC_SAMD51 rtc;     // Real-time clock
WiFiClient client;  // Wi-Fi client
WiFiUDP udp;        // UDP endpoint
DateTime now;       // Current date and time object

// Initialize global variables. Uncomment entry for just one time server. Other NTP servers
// can be added.
// const char timeServer[] = "ntp-1.vt.edu";        // A Virginia Tech NTP server
const char timeServer[] = "ntp-2.vt.edu";        // A Virginia Tech NTP server
// const char timeServer[] = "ntp-3.vt.edu";        // A Virginia Tech NTP server
// const char timeServer[] = "ntp-4.vt.edu";        // A Virginia Tech NTP server
// const char timeServer[] = "time.nist.gov";       // Public time.nist.gov NTP server
// const char timeServer[] = "time.google.com";     // Public time.google.com NTP server

const unsigned int localPort = 59840;   // Local port to listen for UDP packets (use ephemeral port 49152–65535)
const unsigned int ntpPort = 123;       // Well-known port for NTP service using UDP

const int NTP_PACKET_SIZE = 48;     // NTP timestamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // Buffer to hold incoming and outgoing NTP packets

unsigned long devicetime;           // Device time variable as seconds offset from 1/1/1970

const char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char monthsOfTheYear[12][12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

unsigned long previousMillis = 0;   // Previous time used to take action in loop() about every minute
// (In the full solution, previousMillis is not needed since display updates are interrupt-driven
// rather than based on checking time in the loop() routine.)

void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  // setup failure flag
  bool setupFailed = false;

  // Start serial initilization and wait 2 seconds for it to start
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);

  displaySplashScreen();
  statusMessage("Initializing", TFT_BLACK);
  delay(1000);

  if(!rtc.begin()) {
    setupFailed = true;
    statusMessage("RTC Initialization Failed", TFT_RED);
  } else {
    statusMessage("RTC Initialized", TFT_BLACK);
  }

  if(!setupFailed) {
    if(!setTime()) {
      setupFailed = true;
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}

// shows display screen
void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB12);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("ECE 5984", TFT_HEIGHT/2, TFT_WIDTH/2 - 75);
  tft.drawString("Project 2", TFT_HEIGHT/2, TFT_WIDTH/2 - 45);
  tft.drawString("Ben Seidel", TFT_HEIGHT/2, TFT_WIDTH/2 - 15);
}

// helper function to write status messages
void statusMessage(char *message, uint16_t color) {
  uint16_t lightBlue = tft.color565(135, 206, 250);
  tft.fillRect(TFT_HEIGHT/2 - 120, TFT_WIDTH/2 + 40, 240, 40, lightBlue);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB9);
  tft.setTextColor(color);
  tft.drawString(message, TFT_HEIGHT/2, TFT_WIDTH/2 + 60);
}

// Set time by connecting to WiFi, retrieving time from NTP, and writing to the RTC. Return true
// if it all works, otherwise return false.bool setTime() 
bool setTime() {

  if(!connectWiFi()) {
    statusMessage("Connection to Wi-Fi Failed", TFT_BLACK);
    return false;
  }

  devicetime = getNTPtime();

  if(devicetime == 0) {
      statusMessage("NTP Request Failed", TFT_BLACK);
      return false;
  }

  rtc.adjust(DateTime(devicetime));

  now = rtc.now();
  statusMessage("RTC Updated", TFT_BLACK);

  WiFi.disconnect();
  statusMessage("Disconnected from Wi-Fi", TFT_BLACK);

  return true;
}

// Connects to wifi using SSID and password. Repeats WIFI_ATTEMPTS times.
bool connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int attempts = 0;
  do {
    if(strlen(password) > 0) {
      WiFi.begin(ssid, password);
    } else {
      WiFi.begin(ssid);
    }
    attempts++;
    statusMessage("Connecting to Wi-Fi", TFT_BLACK);
    delay(1000);

    uint16_t start = millis();
    while((WiFi.status() != WL_CONNECTED) && ((millis() - start) < 4000));
  } while((WiFi.status() != WL_CONNECTED) && (attempts < WIFI_ATTEMPTS));

  // DEBUG for how many times WiFi is tried
  // Serial.print("attempts: ");
  // Serial.println(attempts);
  if(WiFi.status() != WL_CONNECTED) {
    statusMessage("Connection to Wi-Fi Failed", TFT_BLACK);
    return false;
  }
  
  statusMessage("Connected to Wi-Fi", TFT_BLACK);
  return true;
}

//
unsigned long getNTPtime() {

  int udpBytes = 0;
  if(WiFi.status() == WL_CONNECTED) {

    udp.begin(WiFi.localIP(), localPort);

    sendNTPpacket(timeServer);
    statusMessage("Getting Time", TFT_BLACK);
    delay(1000);
  
    uint32_t start = millis();
    do{
      udpBytes = udp.parsePacket();
    } while((udpBytes == 0) && (start - millis() < 10000));

    if(udpBytes > 0) {
      statusMessage("Time Retrieved", TFT_BLACK);

      // Read the data from the packet into the packet buffer up to maximum size.
            udp.read(packetBuffer, NTP_PACKET_SIZE);

            // The timestamp starts at byte 40 of the received packet and is four bytes (two words)
            // long. First, extract the two words.
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

            // Then combine the four bytes (two words) into a long integer. The result is NTP time,
            // which is the number of seconds since January 1, 1900.
            unsigned long secsSince1900 = highWord << 16 | lowWord;

            // Unix time starts 70 years later on January 1, 1970. 70 years is 2208988800 seconds.
            const unsigned long seventyYears = 2208988800UL;

            // Subtract seventy years to get Unix time, which is for UTC.
            unsigned long epoch = secsSince1900 - seventyYears;

            // Adjust time for timezone offset in secs +/- from UTC. Offset was set from
            // configuration input.
            long tzOffset = offsetFromUTC * ONE_HOUR;
            unsigned long adjustedTime;
            return adjustedTime = epoch + tzOffset;
    } else {
      statusMessage("NTP Request Failed", TFT_BLACK);
      udp.stop(); // clear udp connection
      return 0;
    }
    udp.stop();
  } else {
    // not connected to WiFi
    return 0;
  }
}

// Send NTP request to an NTP server
// function from SEED website for RTC overview, also code from example overview
void sendNTPpacket(const char* address) {

  // init NTP packet to 0s
  for(int i = 0; i < NTP_PACKET_SIZE; i++) {
    packetBuffer[i] = 0;
  }

    // Initialize values needed to form NTP request
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, ntpPort); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

