#include <Arduino.h>
#include <Servo.h>
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
#define TOUCH_PIN 4  

// ---------------- Servos ----------------
#define H_SERVO_PIN 3
#define V_SERVO_PIN 5

Servo hServo;
Servo vServo;

// ---------------- Ultrasonic ----------------
#define TRIG_PIN 6
#define ECHO_PIN 7
#define SPEAKER_PIN 2

#define MAX_DISTANCE 40

// ---------------- Calibration / Sweep ----------------
int hMin = 0, hMax = 180;
int vMin = 20, vMax = 100;
int thresholdDistance = 30;

int hAngle, vAngle;
int hStep = 1;
int vStep = 10;

const unsigned long CALIBRATION_TIMEOUT = 60000;
unsigned long lastBeep = 0;

// ---------- Helper Functions ----------
int adjustValue(int value, int minVal, int maxVal) {
  int offset = analogRead(JOY_PIN) - 512;
  if (offset > JOY_THRESHOLD && value < maxVal) value++;
  if (offset < -JOY_THRESHOLD && value > minVal) value--;
  return value;
}

void hapticFeedback() {
  tone(SPEAKER_PIN, 900, 80); // 80ms haptic buzz
}

void calibrateStep(const char* label, int &value, int minVal, int maxVal) {
  unsigned long startTime = millis();
  while (millis() - startTime < CALIBRATION_TIMEOUT) {
    value = adjustValue(value, minVal, maxVal);

    display.clearDisplay();
    display.setCursor(0,0);
    display.println(label);
    display.println(F("Move joystick to adjust"));
    display.println(F("Press button to confirm"));
    display.setCursor(0,40);
    display.print(F("Value: "));
    display.println(value);
    display.display();

    if (digitalRead(TOUCH_PIN) == HIGH) {
      hapticFeedback();
      display.clearDisplay();
      display.setCursor(0,20);
      display.println(F("Confirmed!"));
      display.display();
      Serial.print(label); Serial.print(" confirmed: "); Serial.println(value);
      delay(500);
      break;
    }
    delay(50);
  }
}

// ---------- Distance Reading ----------
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

// ---------- Draw Detection ----------
void drawDetection(int hAngle, int vAngle, int distance) {
  int x = map(hAngle, hMin, hMax, 0, SCREEN_WIDTH-1);
  int y = map(vAngle, vMin, vMax, SCREEN_HEIGHT-1, 0);

  int thickness = map(distance, 1, thresholdDistance, 5, 1);
  if(thickness < 1) thickness = 1;
  int half = thickness / 2;

  for(int i = -half; i <= half; i++) {
    display.drawLine(x + i, y - 3, x + i, y + 3, WHITE);
  }
}

// ---------- Sonar Sweep Sound ----------
void sonarSweepSound(int angle) {
  int center = (hMin + hMax) / 2;
  int distFromCenter = abs(angle - center);
  int freq = map(distFromCenter, 0, (hMax - hMin)/2, 650, 350);
  tone(SPEAKER_PIN, freq);
}

// ---------- Detection Beep ----------
void detectionBeep() {
  tone(SPEAKER_PIN, 1200, 100); // 100ms beep
}

// ---------- Setup ----------
void setup() {
  pinMode(JOY_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  hServo.attach(H_SERVO_PIN);
  vServo.attach(V_SERVO_PIN);

  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED init failed!");
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.display();

  // -------- Calibration --------
  calibrateStep("Horizontal min. angle", hMin, 0, hMax-1);
  calibrateStep("Horizontal max. angle", hMax, hMin+1, 180);
  calibrateStep("Vertical min. angle", vMin, 0, vMax-1);
  calibrateStep("Vertical max. angle", vMax, vMin+1, 180);
  calibrateStep("Max. distance", thresholdDistance, 1, 50);

  // initialize angles after calibration
  hAngle = hMin;
  vAngle = vMin;
  hStep = 1;
  hServo.write(hAngle);
  vServo.write(vAngle);

  display.clearDisplay();
  display.setCursor(0,20);
  display.println(F("Calibration Complete!"));
  display.display();
  delay(1000);
  display.clearDisplay();
}

// ---------- Loop ----------
void loop() {
  long distance = readDistanceCM();

  if(distance > 0 && distance <= thresholdDistance) {
    drawDetection(hAngle, vAngle, distance);

    // Detection beep with cooldown
    if(millis() - lastBeep > 400) {
      detectionBeep();
      lastBeep = millis();
    }

    display.display();
  } else {
    // play sonar sweep when nothing detected
    sonarSweepSound(hAngle);
  }

  // move horizontally
  hAngle += hStep;

  if(hAngle >= hMax) {
    hAngle = hMax;
    hStep = -hStep;
    vAngle += vStep;
  } else if(hAngle <= hMin) {
    hAngle = hMin;
    hStep = -hStep;
    vAngle += vStep;
  }

  // check vertical bounds
  if(vAngle > vMax) {
    vAngle = vMin;
    display.clearDisplay(); // clear after full sweep
  }

  hServo.write(hAngle);
  vServo.write(vAngle);

  delay(50);
}
