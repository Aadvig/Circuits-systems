#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// your existing pin defs here...
#define SERVO_PIN 3
Servo radarServo;

// OLED setup if you want visual feedback
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 8
#define OLED_RESET 9
#define OLED_CS 10
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// helper to draw radar at a single angle
long readDistanceCM() { return 0; } // dummy for display
void drawRadar(int angle, int distance) { /* optional visual code */ }

void runCalibrationTest() {
  int testAngles[] = {0, 30, 60, 90, 120, 150, 180};
  for(int i=0;i<7;i++){
    radarServo.write(testAngles[i]);
    drawRadar(testAngles[i], readDistanceCM());
    delay(2000);
  }
  radarServo.write(0);
  drawRadar(0, readDistanceCM());
}

void setup() {
  radarServo.attach(SERVO_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC)) for(;;);
  display.clearDisplay();
  display.setTextColor(WHITE);

  runCalibrationTest();
}

void loop() {
  // your normal logic later
}
