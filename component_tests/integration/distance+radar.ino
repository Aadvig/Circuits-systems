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
#define JOY_THRESHOLD 100
#define TOUCH_PIN 4

// ---------------- Servo ----------------
#define SERVO_PIN 3
Servo radarServo;

// ---------------- Sweep Settings ----------------
const int STEP = 1;     // 1 increment for joystick calibration
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;
const int SWEEP_RADIUS = 56;

// ---------------- Calibration Defaults ----------------
long thresholdDistance = 20;
int rotationSetting = 180;
const unsigned long CALIBRATION_TIMEOUT = 30000; // 30 seconds

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

// ---------------- Display -------------------
void showMessage(String line1, String line2, long value, String unit) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(line1);
  display.println(line2);
  display.setTextSize(1);
  display.setCursor(0,40);
  display.print(value);
  display.print(" ");
  display.println(unit);
  display.display();
}

// ---------------- Calibration -------------------
void runCalibration() {
  unsigned long startTime = millis();

  bool distanceSet = false;
  bool rotationSet = false;

  // Temporary variables for live adjustment
  long tempDistance = thresholdDistance;
  int tempRotation = rotationSetting;

  while(millis() - startTime < CALIBRATION_TIMEOUT && (!distanceSet || !rotationSet)) {
    int offset = readJoystickOffset();

    // Adjust distance
    if(!distanceSet) {
      if(offset > JOY_THRESHOLD && tempDistance < 50) tempDistance += STEP;
      if(offset < -JOY_THRESHOLD && tempDistance > 1) tempDistance -= STEP;
      showMessage("Set MAX DISTANCE", "Press touch to confirm", tempDistance, "cm");

      if(isTouchPressed()) {
        distanceSet = true;
        thresholdDistance = tempDistance;
        delay(300); // debounce
      }
    }

    // Adjust rotation
    if(distanceSet && !rotationSet) {
      if(offset > JOY_THRESHOLD && tempRotation < MAX_ANGLE) tempRotation += STEP;
      if(offset < -JOY_THRESHOLD && tempRotation > MIN_ANGLE) tempRotation -= STEP;
      showMessage("Set MAX ROTATION", "Press touch to confirm", tempRotation, "deg");

      if(isTouchPressed()) {
        rotationSet = true;
        rotationSetting = tempRotation;
        delay(300); // debounce
      }
    }

    delay(50);
  }

  // Apply defaults if not set in time
  if(!distanceSet) thresholdDistance = 20;
  if(!rotationSet) rotationSetting = 180;
}

// ---------------- Radar Display -------------------
void drawRadar(int currentAngle, int currentDistance) {
  display.clearDisplay();
  int centerX = SCREEN_WIDTH/2;
  int centerY = SCREEN_HEIGHT-1;

  // Radar circles
  for(int r=16; r<=SWEEP_RADIUS; r+=10) display.drawCircle(centerX, centerY, r, WHITE);

  // Reference lines
  display.drawLine(0, centerY, SCREEN_WIDTH, centerY, WHITE);
  for(int a=30;a<=150;a+=30){
    float rad = a*3.14159/180;
    display.drawLine(centerX, centerY,
                     centerX - SWEEP_RADIUS*cos(rad),
                     centerY - SWEEP_RADIUS*sin(rad),
                     WHITE);
  }

  // Sweep line and object
  float rad = currentAngle*3.14159/180;
  int lineLength = SWEEP_RADIUS;
  if(currentDistance>0){
    lineLength = map(currentDistance,0,thresholdDistance,0,SWEEP_RADIUS);
    if(lineLength>SWEEP_RADIUS) lineLength = SWEEP_RADIUS;
  }
  display.drawLine(centerX, centerY,
                   centerX - lineLength*cos(rad),
                   centerY - lineLength*sin(rad),
                   WHITE);
  if(currentDistance>0 && currentDistance<=thresholdDistance){
    display.fillCircle(centerX - lineLength*cos(rad),
                       centerY - lineLength*sin(rad),
                       2, WHITE);
  }

  // Text
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Angle: "); display.print(currentAngle);
  display.setCursor(0,10);
  display.print("Dist: "); display.print(currentDistance); display.println("cm");
  display.display();
}

// ---------------- Setup -------------------
void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(JOY_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) for(;;);
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Run the 30-second two-step calibration BEFORE servo init
  runCalibration();

  radarServo.attach(SERVO_PIN);
}

// ---------------- Main Loop -------------------
void loop() {
  // Sweep 0 -> rotationSetting
  for(int angle=0; angle<=rotationSetting; angle+=3){
    radarServo.write(angle);
    delay(50);
    int distance = readDistanceCM();
    drawRadar(angle,distance);
  }

  // Sweep rotationSetting -> 0
  for(int angle=rotationSetting; angle>=0; angle-=3){
    radarServo.write(angle);
    delay(50);
    int distance = readDistanceCM();
    drawRadar(angle,distance);
  }
}
