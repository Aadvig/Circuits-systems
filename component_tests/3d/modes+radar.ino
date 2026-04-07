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
#define JOY_X A0
#define JOY_Y A1
#define JOY_THRESHOLD 10
#define THRESHOLD 50

#define TOUCH_PIN 4
#define POT_PIN A2

// ---------------- Servos ----------------
#define H_SERVO_PIN 3
#define V_SERVO_PIN 5

Servo hServo;
Servo vServo;

// ---------------- Ultrasonic ----------------
#define TRIG_PIN 7
#define ECHO_PIN 6
#define SPEAKER_PIN 2

#define MAX_DISTANCE 40

// ---------------- Calibration / Sweep ----------------
int hMin = 0, hMax = 180;
int vMin = 0, vMax = 130;
int thresholdDistance = 20;

int hAngle, vAngle;
int hStep = 1;
int vStep = 10;

unsigned long lastBeep = 0;

// ---------------- Mode ----------------
bool manualMode = false;
unsigned long lastDisplayUpdate = 0;

// ---------- Mode 1 Radar Trail ----------
#define SWEEP_RADIUS 56
#define TRAIL_LENGTH 10

int angleTrail[TRAIL_LENGTH];
int distTrail[TRAIL_LENGTH];

// ---------- Speed Control ----------
int speedDelay = 20;

int readSpeedDelay() {

  static int lastValue = 20;

  int potValue = analogRead(POT_PIN);

  // Reverse mapping so clockwise = faster sweep
  int delayTime = map(potValue, 0, 1023, 100, 10);

  // simple smoothing
  delayTime = (delayTime + lastValue) / 2;
  lastValue = delayTime;

  return constrain(delayTime, 10, 100);
}

int getSweepDelay() {
  return 110 - readSpeedDelay();
}

// ---------- Helper Functions ----------
int adjustValue(int value, int minVal, int maxVal) {

  int joyY = analogRead(JOY_Y);     // read vertical axis
  int offset = joyY - 512;          // center around 0

  // dead zone
  if (abs(offset) < JOY_THRESHOLD) return value;

  // move value
  if (offset > 0) value--;
  else value++;

  // clamp within limits
  value = constrain(value, minVal, maxVal);

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
void calibrateSpeed() {

  int lastShown = -1;

  while (true) {

    int current = readSpeedDelay();

    if (current != lastShown) {

      display.clearDisplay();
      display.setCursor(0,0);
      display.println(F("Set Sweep Speed"));
      display.println(F("Turn knob"));
      display.println(F("Press to confirm"));
      display.setCursor(0,40);
      display.print(F("Speed: "));
      display.println(current);
      display.display();

      lastShown = current;
    }

    if (digitalRead(TOUCH_PIN) == HIGH) {
      hapticFeedback();
      speedDelay = current;
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
void drawMode1Radar(int currentAngle, int currentDistance) {

  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT - 1;

  // shift trail history
  for(int i = TRAIL_LENGTH - 1; i > 0; i--) {
    angleTrail[i] = angleTrail[i - 1];
    distTrail[i] = distTrail[i - 1];
  }

  angleTrail[0] = currentAngle;
  distTrail[0] = currentDistance;

  display.clearDisplay();

  // radar circles
  for(int r = 16; r <= SWEEP_RADIUS; r += 10) {
    display.drawCircle(centerX, centerY, r, WHITE);
  }

  // baseline
  display.drawLine(0, centerY, SCREEN_WIDTH, centerY, WHITE);

  // angle markers
  for(int a = 30; a <= 150; a += 30) {
    float rad = a * 3.14159 / 180.0;

    display.drawLine(centerX, centerY,
                     centerX - SWEEP_RADIUS * cos(rad),
                     centerY - SWEEP_RADIUS * sin(rad),
                     WHITE);
  }

  // draw sweep trail
  for(int i = TRAIL_LENGTH - 1; i >= 0; i--) {
    float rad = angleTrail[i] * 3.14159 / 180.0;

    int lineLength = SWEEP_RADIUS;

    if(distTrail[i] > 0) {
      lineLength = map(distTrail[i], 0, thresholdDistance, 0, SWEEP_RADIUS);
      if(lineLength > SWEEP_RADIUS) lineLength = SWEEP_RADIUS;
    }

    int pixelStep = 1 + i;

    for(int l = 0; l < lineLength; l += pixelStep) {
      int x = centerX - l * cos(rad);
      int y = centerY - l * sin(rad);
      display.drawPixel(x, y, WHITE);
    }

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

// ---------- Manual HUD ----------
void drawManualHUD(long distance) {

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("MANUAL MODE");

  display.setCursor(0,16);
  display.print("H: ");
  display.print(hAngle);

  display.setCursor(0,26);
  display.print("V: ");
  display.print(vAngle);

  display.setCursor(0,40);
  display.print("Dist: ");
  display.print(distance);
  display.print("cm");

  if(distance > 0 && distance < thresholdDistance) {
    display.setCursor(0,54);
    display.print("OBJECT DETECTED");
  }

  display.display();
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

  calibrateStep("H Min", hMin, 0, hMax-1);
  calibrateStep("H Max", hMax, hMin+1, 180);
  calibrateStep("V Min", vMin, 0, vMax-1);
  calibrateStep("V Max", vMax, vMin+1, 180);
  calibrateStep("Max Distance", thresholdDistance, 1, 50);
  calibrateSpeed();

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

// ----- Toggle Modes (edge-detect) -----
static bool lastTouchState = LOW;
bool currentTouchState = digitalRead(TOUCH_PIN);

if(currentTouchState == HIGH && lastTouchState == LOW) {
    manualMode = !manualMode;    // toggle mode
    hapticFeedback();
    
    display.clearDisplay();       // <<< clear OLED immediately
    display.display();            // ensure cleared
    
    delay(10);                    // tiny debounce
}

lastTouchState = currentTouchState;

  // ================= MANUAL MODE =================
  if(manualMode) {

  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);

  // Horizontal movement
  if (x > 512 + THRESHOLD && hAngle < hMax) hAngle++;
  else if (x < 512 - THRESHOLD && hAngle > hMin) hAngle--;

  // Vertical movement (inverted joystick)
  if (y > 512 + THRESHOLD && vAngle > vMin) vAngle--;
  else if (y < 512 - THRESHOLD && vAngle < vMax) vAngle++;

  // Hard safety clamp
  hAngle = constrain(hAngle, hMin, hMax);
  vAngle = constrain(vAngle, vMin, vMax);

  hServo.write(hAngle);
  vServo.write(vAngle);

  if(millis() - lastDisplayUpdate > 120) {

    long distance = readDistanceCM();
    drawManualHUD(distance);

    lastDisplayUpdate = millis();
  }

  delay(getSweepDelay());
  return;
}

  // ================= SWEEP MODE =================
  long distance = readDistanceCM();

drawMode1Radar(hAngle, distance);

if(distance > 0 && distance <= thresholdDistance) {
  if(millis() - lastBeep > 400) {
    detectionBeep();
    lastBeep = millis();
  }
}
else {
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

  delay(getSweepDelay());
}
