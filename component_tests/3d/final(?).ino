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
#define POT_PIN A2   // potentiometer

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

unsigned long lastBeep = 0;

// ---------- Speed Control ----------
int speedDelay = 50; // default

int readSpeedDelay() {
  static int lastValue = 50;

  int potValue = analogRead(POT_PIN);
  int delayTime = map(potValue, 0, 1023, 7, 70);

  // smoothing
  delayTime = (delayTime + lastValue) / 2;
  lastValue = delayTime;

  return constrain(delayTime, 7, 70);
}

// ---------- Helper Functions ----------
int adjustValue(int value, int minVal, int maxVal) {
  int offset = analogRead(JOY_PIN) - 512;
  if (offset > JOY_THRESHOLD && value < maxVal) value++;
  if (offset < -JOY_THRESHOLD && value > minVal) value--;
  return value;
}

void hapticFeedback() {
  tone(SPEAKER_PIN, 900, 80);
}

// ---------- Calibration ----------
void calibrateStep(const char* label, int &value, int minVal, int maxVal) {
  while (true) {
    value = adjustValue(value, minVal, maxVal);

    display.clearDisplay();
    display.setCursor(0,0);
    display.println(label);
    display.println(F("Move joystick"));
    display.println(F("Press to confirm"));
    display.setCursor(0,40);
    display.print(F("Value: "));
    display.println(value);
    display.display();

    if (digitalRead(TOUCH_PIN) == HIGH) {
      hapticFeedback();
      delay(300);
      break;
    }

    delay(50);
  }
}

// --- Speed Calibration (new) ---
void calibrateSpeed() {
  while (true) {
    int current = readSpeedDelay();

    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("Set Sweep Speed"));
    display.println(F("Turn knob"));
    display.println(F("Press to confirm"));
    display.setCursor(0,40);
    display.print(F("Delay: "));
    display.println(current);
    display.display();

    if (digitalRead(TOUCH_PIN) == HIGH) {
      hapticFeedback();
      speedDelay = current; // store initial value
      delay(300);
      break;
    }

    delay(50);
  }
}

// ---------- Distance ----------
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

// ---------- Drawing ----------
void drawDetection(int hAngle, int vAngle, int distance) {
  int x = map(hAngle, hMin, hMax, 0, SCREEN_WIDTH-1);
  int y = map(vAngle, vMin, vMax, SCREEN_HEIGHT-1, 0);

  int thickness = map(distance, 1, thresholdDistance, 5, 1);
  thickness = max(thickness, 1);

  int half = thickness / 2;

  for(int i = -half; i <= half; i++) {
    display.drawLine(x + i, y - 3, x + i, y + 3, WHITE);
  }
}

// ---------- Sound ----------
void sonarSweepSound(int angle) {
  int center = (hMin + hMax) / 2;
  int distFromCenter = abs(angle - center);
  int freq = map(distFromCenter, 0, (hMax - hMin)/2, 650, 350);
  tone(SPEAKER_PIN, freq);
}

void detectionBeep() {
  tone(SPEAKER_PIN, 1200, 100);
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
    for(;;);
  }

  display.setTextColor(WHITE);
  display.setTextSize(1);

  // -------- Calibration --------
  calibrateStep("H Min", hMin, 0, hMax-1);
  calibrateStep("H Max", hMax, hMin+1, 180);
  calibrateStep("V Min", vMin, 0, vMax-1);
  calibrateStep("V Max", vMax, vMin+1, 180);
  calibrateStep("Max Distance", thresholdDistance, 1, 50);
  calibrateSpeed();  // ⭐ new step

  // initialize positions
  hAngle = hMin;
  vAngle = vMin;

  hServo.write(hAngle);
  vServo.write(vAngle);

  display.clearDisplay();
  display.setCursor(0,20);
  display.println(F("Ready"));
  display.display();
  delay(1000);
  display.clearDisplay();
}

// ---------- Loop ----------
void loop() {
  long distance = readDistanceCM();

  if(distance > 0 && distance <= thresholdDistance) {
    drawDetection(hAngle, vAngle, distance);

    if(millis() - lastBeep > 400) {
      detectionBeep();
      lastBeep = millis();
    }

    display.display();
  } else {
    sonarSweepSound(hAngle);
  }

  // movement
  hAngle += hStep;

  if(hAngle >= hMax || hAngle <= hMin) {
    hStep = -hStep;
    vAngle += vStep;
  }

  if(vAngle > vMax) {
    vAngle = vMin;
    display.clearDisplay();
  }

  hServo.write(hAngle);
  vServo.write(vAngle);

  // 🔥 LIVE SPEED CONTROL
  delay(readSpeedDelay());
}
