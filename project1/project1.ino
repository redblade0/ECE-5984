/**
  Class: ECE 5984 Special Study: IOT system design
  Project 1: Design Kit Testing and Familiarization
  Ben Seidel
  benseidel@vt.edu
  
  Required libraries/files:
  Seeeduino WIO Terminal
  tft_eSPI
  LIS3DHTR

  Compiling/Uploading:
  Use the Seeeduino Wio Terminal board

  Acknowledgments:
  Make sure to use the Free_Fonts.h
  Referenced F26_DeviceKitTest.ino for function calls to drawStrings + accelerometer data
*/



#include "TFT_eSPI.h"                               // TFT LCD header file
#include "Free_Fonts.h"                             // Free fonts header file (needs to be in directory with this file)
#include "LIS3DHTR.h"                               // Accelerometer library


// Instantiate necessary components.
TFT_eSPI tft;                               // Built-in TFT display
LIS3DHTR<TwoWire> accelerometer;            // Built-in accelerometer

// Define value for the top of the display area for tests that display sensor values or other
// information.
#define VALUE_TOP 90

enum Screen {
  SPLASH,
  SOUND,
  LIGHT,
  ACC
};

Screen currentScreen = SPLASH;


void setup() {
  // put your setup code here, to run once:
  tft.begin();
  tft.setRotation(3);
  tft.backlight();

  initVars();
  // Show splash screen.
  displaySplashScreen();
}

void loop() {
  // put your main code here, to run repeatedly:

  if (buttonPress(WIO_KEY_A)) { // button A display sound
    if(currentScreen != SOUND) {
      currentScreen = SOUND;
      displaySound();
    } else {
      updateSound();
    }
  } else if (buttonPress(WIO_KEY_B)) { // button B display light
    if(currentScreen != LIGHT) {
      currentScreen = LIGHT;
      displayLight();
    } else {
      updateLight();
    }
  } else if (buttonPress(WIO_KEY_C)) { // button C display accelerometer
    if(currentScreen != ACC) {
      currentScreen = ACC;
      displayAcc();
    } else {
      updateAcc();
    }
  } else {
    if(currentScreen != SPLASH) { // display splash screen if none pressed
      currentScreen = SPLASH;
      displaySplashScreen();
    }
  }

  delay(1000); // Sensor values update every second (1 Hz refresh rate);
}


bool buttonPress(int buttonPin) {
  return (digitalRead(buttonPin) == LOW);
}


void displaySplashScreen() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(FSSB12);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("ECE 5984", TFT_HEIGHT/2, TFT_WIDTH/2 - 15);
  tft.drawString("Project 1", TFT_HEIGHT/2, TFT_WIDTH/2 + 15);
  tft.drawString("Ben Seidel", TFT_HEIGHT/2, TFT_WIDTH/2 + 45);
}


// displays the sound sensor screen
void displaySound() {
  tft.fillScreen(TFT_PURPLE);
  tft.drawString("Sound Sensor", TFT_HEIGHT/2 - 75, TFT_WIDTH/2);
  updateSound();
}

// updates the sound sensor value
void updateSound() {
  tft.fillRect(TFT_HEIGHT / 2 + 25, TFT_WIDTH / 2 - 20, 80, 40, TFT_PURPLE);
  uint32_t soundValue = analogRead(WIO_MIC);
  tft.drawNumber(soundValue, TFT_HEIGHT/2 + 50, TFT_WIDTH/2);
}

// displays the light sensor screen
void displayLight() {
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Light Sensor", TFT_HEIGHT/2 - 75, TFT_WIDTH/2);
  updateLight();
}

// updates the light sensor value
void updateLight() {
  tft.fillRect(TFT_HEIGHT / 2 + 20, TFT_WIDTH / 2 - 20, 80, 40, TFT_BLACK);
  uint32_t lightValue = analogRead(WIO_LIGHT);
  tft.drawNumber(lightValue, TFT_HEIGHT/2 + 40, TFT_WIDTH/2);
}

// displays the acceleromter screen
void displayAcc() {
  tft.fillScreen(TFT_GREEN);

  tft.drawString("Accelerometer", TFT_HEIGHT/2, TFT_WIDTH/2);
  tft.drawString("X: ", TFT_HEIGHT/2 - 25, TFT_WIDTH/2 + 30);
  tft.drawString("Y: ", TFT_HEIGHT/2 - 25, TFT_WIDTH/2 + 60);
  tft.drawString("Z: ", TFT_HEIGHT/2 - 25, TFT_WIDTH/2 + 90);
}

// updates the accelerometer values
void updateAcc() {
  tft.fillRect(TFT_HEIGHT/2 - 10, TFT_WIDTH/2 + 15, 70, 100, TFT_GREEN);

  float accX = accelerometer.getAccelerationX();
  float accY = accelerometer.getAccelerationY();
  float accZ = accelerometer.getAccelerationZ();

  tft.drawFloat(accX, 3, TFT_HEIGHT/2 + 25, TFT_WIDTH/2 + 30);
  tft.drawFloat(accY, 3, TFT_HEIGHT/2 + 25, TFT_WIDTH/2 + 60);
  tft.drawFloat(accZ, 3, TFT_HEIGHT/2 + 25, TFT_WIDTH/2 + 90);
}


// initalizes buttons + sensors
void initVars() {
  pinMode(WIO_KEY_A, INPUT);
  pinMode(WIO_KEY_B, INPUT);
  pinMode(WIO_KEY_C, INPUT);
  pinMode(WIO_MIC, INPUT);

  accelerometer.begin(Wire1);
  accelerometer.setOutputDataRate(LIS3DHTR_DATARATE_1HZ);  // Set accelerometer data output rate (1Hz to 5kHz).
  accelerometer.setFullScaleRange(LIS3DHTR_RANGE_8G);      // Scale accelerometer range (2G to 16G).
}
