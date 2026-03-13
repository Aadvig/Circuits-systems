#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- Servo ----------------
#define SERVO_PIN 3
Servo radarServo;

// ---------------- Touch ----------------
#define TOUCH_PIN 5

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 8
#define OLED_RESET 9
#define OLED_CS 10
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ---------------- Helpers ----------------
long readDistanceCM() { return 0; } // dummy for display

void drawRadar(int angle, int distance) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Angle: "); display.println(angle);
  display.display();
}

// ---------------- Calibration Test ----------------
int currentAngle = 0;

void setup() {
  pinMode(TOUCH_PIN, INPUT);
  radarServo.attach(SERVO_PIN);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) for(;;);
  display.clearDisplay();
  display.setTextColor(WHITE);

  drawRadar(currentAngle, readDistanceCM());
}

void loop() {
  static bool lastTouch = false;

  bool touchPressed = digitalRead(TOUCH_PIN);

  // Detect rising edge (button press)
  if(touchPressed && !lastTouch) {
    currentAngle += 30;
    if(currentAngle > 200) currentAngle = 0; // wrap around
    radarServo.write(currentAngle);
    drawRadar(currentAngle, readDistanceCM());
  }

  lastTouch = touchPressed;
  delay(50); // debounce / loop speed
}
