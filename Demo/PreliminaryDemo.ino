#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <SPI.h>

// ---------------- OLED Setup ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI   11
#define OLED_CLK    13
#define OLED_DC     8
#define OLED_CS     10
#define OLED_RESET  9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ---------------- Sensors ----------------
#define TRIG_PIN 6
#define ECHO_PIN 7
#define MAX_DISTANCE 40

#define JOY_PIN A0
#define JOY_THRESHOLD 5
#define TOUCH_PIN 5

// ---------------- Servo ----------------
#define SERVO_PIN 3
Servo radarServo;

// ---------------- Speaker ----------------
#define SPEAKER_PIN 2

// ---------------- Sweep Settings ----------------
const int STEP = 2;
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;
const int SWEEP_RADIUS = 56;


// ---------------- Radar Settings ----------------
#define TRAIL_LENGTH 15

int angleTrail[TRAIL_LENGTH];
int distTrail[TRAIL_LENGTH];

// ---------------- Calibration Defaults ----------------
long thresholdDistance = 20;
int rotationSetting = 180;
const unsigned long CALIBRATION_TIMEOUT = 30000;

// ---------------- Helper Functions -------------------
long readDistanceCM() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  long distance = duration * 0.034 / 2;

  if(distance > MAX_DISTANCE) distance = 0;

  return distance;
}

int readJoystickOffset() {
  return analogRead(JOY_PIN) - 512;
}

bool isTouchPressed() {
  return digitalRead(TOUCH_PIN) == HIGH;
}

// ---------------- Sound -------------------

void sonarSweepSound(int angle) {

  int center = rotationSetting / 2;
  int distFromCenter = abs(angle - center);

  int freq = map(distFromCenter, 0, center, 650, 350);

  tone(SPEAKER_PIN, freq);
}

void detectionBeep() {

  tone(SPEAKER_PIN, 1200);
}

void HapticSound() {
  tone(SPEAKER_PIN, 900, 80);   // 900 Hz for 40 ms
}

// ---------------- Display -------------------
void showMessage(String line1, String line2, long value, String unit) {

  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0,0);
  display.println(line1);
  display.println(line2);

  display.setCursor(0,40);
  display.print(value);
  display.print(" ");
  display.println(unit);

  display.display();
}

// ---------------- Calibration -------------------
void runCalibration() {

  long tempDistance = thresholdDistance;
  int tempRotation = rotationSetting;

  unsigned long startTime = millis();

  while(millis() - startTime < CALIBRATION_TIMEOUT) {

    int offset = readJoystickOffset();

    if(offset > JOY_THRESHOLD && tempDistance < 50) tempDistance += STEP;
    if(offset < -JOY_THRESHOLD && tempDistance > 1) tempDistance -= STEP;

    showMessage("Set MAX DISTANCE", "Press touch to confirm", tempDistance, "cm");

    if(isTouchPressed()) {
      HapticSound();
      thresholdDistance = tempDistance;
      delay(300);
      break;
    }

    delay(50);
  }

  startTime = millis();

  while(millis() - startTime < CALIBRATION_TIMEOUT) {

    int offset = readJoystickOffset();

    if(offset > JOY_THRESHOLD && tempRotation < MAX_ANGLE) tempRotation += STEP;
    if(offset < -JOY_THRESHOLD && tempRotation > MIN_ANGLE) tempRotation -= STEP;

    showMessage("Set MAX ROTATION", "Press touch to confirm", tempRotation, "deg");

    if(isTouchPressed()) {
      HapticSound();
      rotationSetting = tempRotation;
      delay(300);
      break;
    }

    delay(50);
  }
}

// ---------------- Radar Display -------------------
void drawRadar(int currentAngle, int currentDistance) {

  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT - 1;

  // shift trail history
  for(int i = TRAIL_LENGTH-1; i > 0; i--) {
    angleTrail[i] = angleTrail[i-1];
    distTrail[i] = distTrail[i-1];
  }

  angleTrail[0] = currentAngle;
  distTrail[0] = currentDistance;

  display.clearDisplay();

  // radar circles
  for(int r = 16; r <= SWEEP_RADIUS; r += 10)
    display.drawCircle(centerX, centerY, r, WHITE);

  // baseline
  display.drawLine(0, centerY, SCREEN_WIDTH, centerY, WHITE);

  // angle markers
  for(int a = 30; a <= 150; a += 30) {

    float rad = a * 3.14159 / 180;

    display.drawLine(centerX, centerY,
                     centerX - SWEEP_RADIUS * cos(rad),
                     centerY - SWEEP_RADIUS * sin(rad),
                     WHITE);
  }

  // draw sweep trail
  for(int i = TRAIL_LENGTH-1; i >= 0; i--) {

    float rad = angleTrail[i] * 3.14159 / 180;

    int lineLength = SWEEP_RADIUS;

    if(distTrail[i] > 0) {
      lineLength = map(distTrail[i], 0, thresholdDistance, 0, SWEEP_RADIUS);
      if(lineLength > SWEEP_RADIUS) lineLength = SWEEP_RADIUS;
    }

    // older lines draw fewer pixels (fade effect)
    int pixelStep = 1 + i;

    for(int l = 0; l < lineLength; l += pixelStep) {

      int x = centerX - l * cos(rad);
      int y = centerY - l * sin(rad);

      display.drawPixel(x, y, WHITE);
    }

    // object dot only for newest sweep
    if(i == 0 && distTrail[i] > 0 && distTrail[i] <= thresholdDistance) {

      int x = centerX - lineLength * cos(rad);
      int y = centerY - lineLength * sin(rad);

      display.fillCircle(x, y, 2, WHITE);
    }
  }

  // text
  display.setTextSize(1);

  display.setCursor(0,0);
  display.print("Angle: ");
  display.print(currentAngle);

  display.setCursor(0,10);
  display.print("Dist: ");
  display.print(currentDistance);
  display.print("cm");

  display.display();
}

// ---------------- Setup -------------------
void setup() {

  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(JOY_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) for(;;);

  display.clearDisplay();
  display.setTextColor(WHITE);

  runCalibration();

  radarServo.attach(SERVO_PIN);
}

// ---------------- Main Loop -------------------
void loop() {

  static int angle = 0;
  static int step = 2;
  static unsigned long lastBeep = 0;

  int distance = readDistanceCM();

  int offset = readJoystickOffset();

  if (abs(offset) > JOY_THRESHOLD) {

    if (offset > 0 && angle < rotationSetting) angle++;
    if (offset < 0 && angle > 0) angle--;
  }
  else {

    angle += step;

    if (angle >= rotationSetting || angle <= 0)
      step = -step;
  }

  radarServo.write(angle);

  // --- Detection logic ---
  if(distance > 0 && distance <= thresholdDistance) {

    if(millis() - lastBeep > 400) {

      detectionBeep();     // alarm sound only
      lastBeep = millis();
    }

  }
  else {

    sonarSweepSound(angle);   // only play sweep when nothing detected
  }

  drawRadar(angle, distance);

  delay(20);
}
