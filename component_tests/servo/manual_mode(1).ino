#include <Arduino.h>
#include <Servo.h>

#define JOY_X A0
#define JOY_Y A1

#define H_SERVO_PIN 3
#define V_SERVO_PIN 5

Servo hServo;
Servo vServo;

int hAngle = 90;
int vAngle = 90;

// threshold to ignore small joystick noise
const int THRESHOLD = 50;

void setup() {
  hServo.attach(H_SERVO_PIN);
  vServo.attach(V_SERVO_PIN);

  hServo.write(hAngle);
  vServo.write(vAngle);
}

void loop() {

  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);

  // --- Horizontal control ---
  if (x > 512 + THRESHOLD && hAngle < 180) {
    hAngle++;
  }
  else if (x < 512 - THRESHOLD && hAngle > 0) {
    hAngle--;
  }

  // --- Vertical control ---
  if (y > 512 + THRESHOLD && vAngle < 180) {
    vAngle++;
  }
  else if (y < 512 - THRESHOLD && vAngle > 0) {
    vAngle--;
  }

  hServo.write(hAngle);
  vServo.write(vAngle);

  delay(10); // speed of movement
}
