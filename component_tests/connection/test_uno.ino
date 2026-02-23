#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

// ---------------- OLED Setup ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, 8, 9, 10);

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

// ---------------- State Output Pins ----------------
#define STATE_PIN0 A4
#define STATE_PIN1 A5

// =====================================================
// ---------------- Helper Functions -------------------
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
  return analogRead(VRx) - 512;
}

bool isTouchPressed() {
  return digitalRead(TOUCH_PIN) == HIGH;
}

// =====================================================
// ---------------- Display Functions ------------------
void showMessage(const char* line1, const char* line2 = nullptr) {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(line1);

  if(line2 != nullptr){
    display.setCursor(0, 35);
    display.println(line2);
  }

  display.display();
}

// =====================================================
// ---------------- State Management -------------------
void setState(byte state){
  digitalWrite(STATE_PIN0, state & 0x01);          // bit0
  digitalWrite(STATE_PIN1, (state >> 1) & 0x01);   // bit1
}

// =====================================================
// ---------------- Calibration Functions -------------
void handleDistanceCalibration() {
  int xOffset = readJoystickOffset();

  if (xOffset > JOY_THRESHOLD && thresholdDistance < 50)
    thresholdDistance += STEP;
  else if (xOffset < -JOY_THRESHOLD && thresholdDistance > 1)
    thresholdDistance -= STEP;

  char buf[16];
  sprintf(buf, "%ld cm", thresholdDistance);
  showMessage("Set Max Distance", buf);

  setState(0); // Waiting for distance calibration

  if (isTouchPressed()) {
    distanceCalibrated = true;
    delay(300);
  }

  delay(50);
}

void handleRotationCalibration() {
  int xOffset = readJoystickOffset();

  if (xOffset > JOY_THRESHOLD && rotationSetting < MAX_DEGREES)
    rotationSetting += STEP;
  else if (xOffset < -JOY_THRESHOLD && rotationSetting > MIN_DEGREES)
    rotationSetting -= STEP;

  char buf[16];
  sprintf(buf, "%d deg", rotationSetting);
  showMessage("Set Max Rotation", buf);

  setState(1); // Waiting for rotation calibration

  if (isTouchPressed()) {
    rotationCalibrated = true;
    delay(300);
  }

  delay(50);
}

void handleMonitoringMode() {
  long distance = readUltrasonicCM();
  bool objectDetected = (distance > 0 && distance <= thresholdDistance);

  if(objectDetected){
    showMessage("OBJECT DETECTED");
    setState(3); // Object detected
  } else {
    char buf1[16], buf2[16];
    sprintf(buf1, "Dist: %ld cm", thresholdDistance);
    sprintf(buf2, "Rot: %d deg", rotationSetting);
    showMessage("Ready", buf1);
    setState(2); // Monitoring / Ready
  }

  delay(50);
}

// =====================================================
// ---------------- Setup ------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(VRx, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(STATE_PIN0, OUTPUT);
  pinMode(STATE_PIN1, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

// =====================================================
// ---------------- Main Loop --------------------------
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
