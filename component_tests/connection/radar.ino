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

// ---------------- Ultrasonic Sensor Setup ----------------
#define TRIG_PIN 6
#define ECHO_PIN 7
#define MAX_DISTANCE 40  // max distance in cm

// ---------------- Servo Setup ----------------
#define SERVO_PIN 3
Servo radarServo;

// ---------------- Sweep Settings ----------------
const int STEP = 3;     // 3° increments
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  radarServo.attach(SERVO_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
}

// =====================================================
// ---------------- Helper Functions -------------------
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  if(distance > MAX_DISTANCE) distance = 0; // treat out-of-range as 0
  return distance;
}

// =====================================================
// ---------------- Display Functions ------------------
void drawRadar(int currentAngle, int currentDistance) {
  display.clearDisplay();

  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT - 1;

  // Radar "arcs" using circles
  for(int r = 16; r <= 56; r += 10) {
    display.drawCircle(centerX, centerY, r, WHITE);
  }

  // Angle lines
  display.drawLine(0, centerY, SCREEN_WIDTH, centerY, WHITE);
  for(int a = 30; a <= 150; a += 30) {
    float rad = a * 3.14159 / 180;
    display.drawLine(centerX, centerY,
                     centerX - 56 * cos(rad),
                     centerY - 56 * sin(rad),
                     WHITE);
  }

  // Sweep line
  float rad = currentAngle * 3.14159 / 180;
  display.drawLine(centerX, centerY,
                   centerX - 56 * cos(rad),
                   centerY - 56 * sin(rad),
                   WHITE);

  // Object blip
  if(currentDistance > 0) {
    int blip = map(currentDistance, 0, MAX_DISTANCE, 0, 56);
    display.drawPixel(centerX - blip * cos(rad), centerY - blip * sin(rad), WHITE);
  }

  // Angle & distance text
  display.setCursor(0,0);
  display.print("Angle: "); display.print(currentAngle);
  display.setCursor(0,10);
  display.print("Dist: "); display.print(currentDistance); display.println("cm");

  display.display();
}

// =====================================================
// ---------------- Main Loop --------------------------
void loop() {
  // Sweep 0° -> 180°
  for(int angle = MIN_ANGLE; angle <= MAX_ANGLE; angle += STEP) {
    radarServo.write(angle);
    delay(50);
    int distance = readDistanceCM();
    drawRadar(angle, distance);
  }

  // Sweep 180° -> 0°
  for(int angle = MAX_ANGLE; angle >= MIN_ANGLE; angle -= STEP) {
    radarServo.write(angle);
    delay(50);
    int distance = readDistanceCM();
    drawRadar(angle, distance);
  }
}
