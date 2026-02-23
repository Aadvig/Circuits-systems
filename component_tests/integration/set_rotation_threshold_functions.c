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
const int VRx = A0;
const int JOY_THRESHOLD = 100;

// ---------------- Touch Sensor Setup ----------------
const int TOUCH_PIN = 12;

// ---------------- Ultrasonic Sensor Setup ----------------
#define TRIG_PIN 6
#define ECHO_PIN 7
long thresholdDistance = 20;

// ---------------- Rotation Variables ----------------
int rotationSetting = 90;
const int MAX_DEGREES = 180;
const int MIN_DEGREES = 10;
const int STEP = 1;

// Calibration flags
bool distanceCalibrated = false;
bool rotationCalibrated = false;


// =====================================================
// ---------------- Helper Functions -------------------
// =====================================================

long readUltrasonicCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance;
}

int readJoystickOffset() {
  int xVal = analogRead(VRx);
  return xVal - 512;
}

bool isTouchPressed() {
  return digitalRead(TOUCH_PIN) == HIGH;
}


// =====================================================
// ---------------- Display Functions ------------------
// =====================================================

void updateDisplayDistance() {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Set Max Distance");
  display.setCursor(0, 35);
  display.print(thresholdDistance);
  display.println(" cm");
  display.display();
}

void updateDisplayRotation() {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Set Max Rotation");
  display.setCursor(0, 35);
  display.print(rotationSetting);
  display.print(" deg");
  display.display();
}

void updateDisplayMonitoring(bool objectDetected) {
  display.clearDisplay();
  display.setCursor(0, 10);

  if (objectDetected) {
    display.println("OBJECT DETECTED");
  } else {
    display.println("Ready");

    display.setCursor(0, 35);
    display.print("Dist: ");
    display.print(thresholdDistance);
    display.print("cm");

    display.setCursor(0, 50);
    display.print("Rot: ");
    display.print(rotationSetting);
    display.println(" deg");
  }

  display.display();
}


// =====================================================
// ---------------- Calibration Modes ------------------
// =====================================================

void handleDistanceCalibration() {
  int xOffset = readJoystickOffset();

  if (xOffset > JOY_THRESHOLD && thresholdDistance < 50)
    thresholdDistance += STEP;
  else if (xOffset < -JOY_THRESHOLD && thresholdDistance > 1)
    thresholdDistance -= STEP;

  updateDisplayDistance();

  if (isTouchPressed()) {
    distanceCalibrated = true;
    delay(300); // debounce
  }

  delay(50);
}

void handleRotationCalibration() {
  int xOffset = readJoystickOffset();

  if (xOffset > JOY_THRESHOLD && rotationSetting < MAX_DEGREES)
    rotationSetting += STEP;
  else if (xOffset < -JOY_THRESHOLD && rotationSetting > MIN_DEGREES)
    rotationSetting -= STEP;

  updateDisplayRotation();

  if (isTouchPressed()) {
    rotationCalibrated = true;
    delay(300);
  }

  delay(50);
}

void handleMonitoringMode() {
  long distance = readUltrasonicCM();
  bool objectDetected = (distance > 0 && distance <= thresholdDistance);

  updateDisplayMonitoring(objectDetected);
  delay(50);
}


// =====================================================
// ---------------- Setup ------------------------------
// =====================================================

void initHardware() {
  Serial.begin(9600);

  pinMode(VRx, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void setup() {
  initHardware();
}


// =====================================================
// ---------------- Main Loop --------------------------
// =====================================================

void loop() {

  if (!distanceCalibrated) {
    handleDistanceCalibration();
    return;
  }

  if (!rotationCalibrated) {
    handleRotationCalibration();
    return;
  }

  handleMonitoringMode();
}
