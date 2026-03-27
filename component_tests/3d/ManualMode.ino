#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------- Joystick --------
#define JOY_X A0
#define JOY_Y A1

// -------- Servos --------
#define H_SERVO_PIN 3
#define V_SERVO_PIN 5

Servo hServo;
Servo vServo;

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 8
#define OLED_RESET 9
#define OLED_CS 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// -------- Ultrasonic --------
#define TRIG_PIN 6
#define ECHO_PIN 7

// -------- Detection --------
#define DETECT_DISTANCE 20

// -------- State --------
int hAngle = 90;
int vAngle = 90;

const int THRESHOLD = 50;

// OLED refresh timer
unsigned long lastDisplayUpdate = 0;

// -------- Distance --------
long readDistanceCM() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 20000);
  long distance = duration * 0.034 / 2;

  return distance;
}

// -------- OLED --------
void drawHUD(long distance) {

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

  if(distance > 0 && distance < DETECT_DISTANCE) {
    display.setCursor(0,54);
    display.print("OBJECT DETECTED");
  }

  display.display();
}

// -------- Setup --------
void setup() {

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  hServo.attach(H_SERVO_PIN);
  vServo.attach(V_SERVO_PIN);

  hServo.write(hAngle);
  vServo.write(vAngle);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
}

// -------- Loop --------
void loop() {

  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);

  // SAME movement logic as your working version
  if (x > 512 + THRESHOLD && hAngle < 180) hAngle++;
  else if (x < 512 - THRESHOLD && hAngle > 0) hAngle--;

  if (y > 512 + THRESHOLD && vAngle < 180) vAngle++;
  else if (y < 512 - THRESHOLD && vAngle > 0) vAngle--;

  hServo.write(hAngle);
  vServo.write(vAngle);

  // Only update display occasionally
  if(millis() - lastDisplayUpdate > 120) {

    long distance = readDistanceCM();
    drawHUD(distance);

    lastDisplayUpdate = millis();
  }

  delay(20);
}
