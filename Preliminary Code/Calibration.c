#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

// ---------------- OLED Setup ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI   11
#define OLED_CLK    13
#define OLED_DC     8
#define OLED_CS     10
#define OLED_RESET  9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ---------------- Joystick Setup ----------------
const int VRx = A0;  // X-axis
const int JOY_THRESHOLD = 100;  // minimal deflection to count as move

// ---------------- Touch Sensor Setup ----------------
const int TOUCH_PIN = 12;

// ---------------- Rotation Variables ----------------
int rotationSetting = 90;  // start at 90
const int MAX_DEGREES = 180;
const int MIN_DEGREES = 10;
const int STEP = 2;        // degrees per joystick deflection

bool buttonPressed = false;

void setup() {
  Serial.begin(9600);

  pinMode(VRx, INPUT);
  pinMode(TOUCH_PIN, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void loop() {
  int xVal = analogRead(VRx);
  int xOffset = xVal - 512; // center ~512

  // Linear increase/decrease
  if (xOffset > JOY_THRESHOLD) {
    rotationSetting += STEP;
  } else if (xOffset < -JOY_THRESHOLD) {
    rotationSetting -= STEP;
  }

  // Clamp to min/max
  if (rotationSetting > MAX_DEGREES) rotationSetting = MAX_DEGREES;
  if (rotationSetting < MIN_DEGREES) rotationSetting = MIN_DEGREES;

  // Touch sensor confirm
  if (digitalRead(TOUCH_PIN) == HIGH) {
    buttonPressed = true;
  }

  // -------- OLED Display -----------
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Set Max Rotation");
  display.setCursor(0, 35);
  display.print(rotationSetting);
  display.print(" deg");
  if (buttonPressed) {
    display.setCursor(0, 50);
    display.println("Confirmed!");
  }
  display.display();

  // Freeze when confirmed
  if (buttonPressed) {
    while (1) {
      display.clearDisplay();
      display.setCursor(0, 10);
      display.println("Max Rotation Set");
      display.setCursor(0, 35);
      display.print(rotationSetting);
      display.println(" deg");
      display.display();
      delay(500);
    }
  }

  delay(50);  // smooth but responsive
}
