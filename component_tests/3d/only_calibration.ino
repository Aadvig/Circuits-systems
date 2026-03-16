#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 8
#define OLED_CS 10
#define OLED_RESET 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ---------------- Controls ----------------
#define JOY_PIN A0
#define JOY_THRESHOLD 5
#define TOUCH_PIN 4  // button for confirming

// ---------------- Calibration Parameters ----------------
int thresholdDistance = 20;   // distance to detect
int hMin = 0, hMax = 180;      // horizontal sweep
int vMin = 20, vMax = 100;     // vertical sweep

const unsigned long CALIBRATION_TIMEOUT = 60000; // 1 min per step

void setup() {
  pinMode(JOY_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED init failed!");
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.display();
}

int adjustValue(int value, int minVal, int maxVal) {
  int offset = analogRead(JOY_PIN) - 512;
  if (offset > JOY_THRESHOLD && value < maxVal) value++;
  if (offset < -JOY_THRESHOLD && value > minVal) value--;
  return value;
}

void calibrateStep(const char* label, int &value, int minVal, int maxVal) {
  unsigned long startTime = millis();
  while (millis() - startTime < CALIBRATION_TIMEOUT) {
    value = adjustValue(value, minVal, maxVal);

    display.clearDisplay();
    display.setCursor(0,0);
    display.println(label);
    display.println(F("Press button to confirm"));
    display.setCursor(0,40);
    display.print(F("Value: "));
    display.println(value);
    display.display();

    if (digitalRead(TOUCH_PIN) == HIGH) {
      Serial.print(label); Serial.print(" confirmed: "); Serial.println(value);
      display.clearDisplay();
      display.setCursor(0,20);
      display.println(F("Confirmed!"));
      display.display();
      delay(1000);
      break;
    }
    delay(50);
  }
}

void loop() {
  // Horizontal Min
  calibrateStep("Horizontal min. angle", hMin, 0, hMax-1);

  // Horizontal Max
  calibrateStep("Horizontal max. angle", hMax, hMin+1, 180);

  // Vertical Min
  calibrateStep("Vertical min. angle", vMin, 0, vMax-1);

  // Vertical Max
  calibrateStep("Vertical max. angle", vMax, vMin+1, 180);

  // Distance
  calibrateStep("Max. distance", thresholdDistance, 1, 50);

  // Done
  display.clearDisplay();
  display.setCursor(0,20);
  display.println(F("Calibration Complete!"));
  display.display();
  Serial.println("Calibration done!");
  while(1); // stop
}
