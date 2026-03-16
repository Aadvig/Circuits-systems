#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_DC 8
#define OLED_CS 10
#define OLED_RESET 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// -------- Servos --------
#define H_SERVO_PIN 3
#define V_SERVO_PIN 5

Servo hServo;
Servo vServo;

// -------- Ultrasonic --------
#define TRIG_PIN 6
#define ECHO_PIN 7

#define MAX_DISTANCE 40
#define DETECT_DISTANCE 30

// horizontal sweep limits
int hMin = 0;
int hMax = 180;

// vertical sweep limits
int vMin = 20;
int vMax = 100;

int hAngle = 0;
int vAngle = 40;

int hStep = 1;
int vStep = 10;

// ---------- Distance ----------
long readDistanceCM()
{
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

// ---------- Draw detection ----------
void drawDetection(int hAngle, int vAngle, int distance)
{
  int x = map(hAngle, hMin, hMax, 0, SCREEN_WIDTH-1);
  int y = map(vAngle, vMin, vMax, SCREEN_HEIGHT-1, 0);

  // closer object = thicker line
  int thickness = map(distance, 1, DETECT_DISTANCE, 5, 1);

  if(thickness < 1) thickness = 1;

  int half = thickness / 2;

  for(int i = -half; i <= half; i++)
  {
    display.drawLine(x + i, y - 3, x + i, y + 3, WHITE);
  }
}

// ---------- Setup ----------
void setup()
{
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  hServo.attach(H_SERVO_PIN);
  vServo.attach(V_SERVO_PIN);

  hServo.write(hAngle);
  vServo.write(vAngle);

  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.display();

  delay(1000);
}

// ---------- Loop ----------
void loop()
{

  int distance = readDistanceCM();

  // if object detected draw it
  if(distance > 0 && distance <= DETECT_DISTANCE)
  {
    drawDetection(hAngle, vAngle, distance);
    display.display();
  }

  // move horizontally
  hAngle += hStep;

  if (hAngle >= hMax || hAngle <= hMin)
  {
    hStep = -hStep;

    vAngle += vStep;

    if (vAngle > vMax)
    {
      vAngle = vMin;

      // clear map after full scan
      display.clearDisplay();
    }

    vServo.write(vAngle);

    delay(500);
  }

  hServo.write(hAngle);

  delay(50);
}
