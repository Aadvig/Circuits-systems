//#define SERIAL_TX_BUFFER_SIZE 8
//#define SERIAL_RX_BUFFER_SIZE 8

#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PCF8574.h>
#include <Wire.h>
#include <SPI.h>
#include <string.h>

// ============================================================
// OLED CONFIGURATION
// ============================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 8
#define OLED_CS 10
#define OLED_RESET 9

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &SPI,
  OLED_DC,
  OLED_RESET,
  OLED_CS
);

// ============================================================
// INPUT / CONTROL PINS
// ============================================================
#define JOY_PIN A0
#define JOY_X A0
#define JOY_Y A1
#define JOY_THRESHOLD 10
#define THRESHOLD 50

#define TOUCH_PIN 4
#define POT_PIN A2

// ============================================================
// SERVO PINS
// ============================================================
#define H_SERVO_PIN 3
#define V_SERVO_PIN 5

Servo hServo;
Servo vServo;

// ============================================================
// ULTRASONIC / AUDIO
// ============================================================
#define TRIG_PIN 7
#define ECHO_PIN 6
#define SPEAKER_PIN 2

#define MAX_DISTANCE 40

// ============================================================
// MODE CONSTANTS / DISPLAY BUFFERS
// ============================================================

// ---------- Radar trail ----------
#define SWEEP_RADIUS 56
#define TRAIL_LENGTH 5
uint8_t angleTrail[TRAIL_LENGTH];

// ---------- Graph / B-scan ----------
#define GRAPH_SAMPLES 128   // width of OLED
uint8_t distanceHistory[GRAPH_SAMPLES];


int freeRAM() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

//----------------- Alarm -----------------

bool alarm_on = false;         // Alarm stateSystemState current_alarm_state = ENTER_CODE;
int alarm_armed = 0;
int entry_delay = 0;
int alarm_range;
//unsigned int alarm_time;

// ---------------- Keypad -------------------

Adafruit_PCF8574 pcf1;
Adafruit_PCF8574 pcf2;

uint16_t keyStates = 0;
const char keys[16] PROGMEM = {'4','9','*','-','7','-','1','0','8','5','2','-','#','-','6','3'};

char code[5]  = "1234";
char input[5] = "";
uint8_t counter = 0;


enum alarmState { 
  ENTER_CODE,
  VERIFY_FOR_RESET,
  SET_NEW_CODE
};

alarmState current_alarm_state = ENTER_CODE;


// ============================================================
// CALIBRATION / SWEEP STATE
// ============================================================
int hMin = 0, hMax = 180;
int vMin = 0, vMax = 90;
int thresholdDistance = 25;

uint8_t hAngle, vAngle;
int hStep = 1;
uint8_t vStep = 10;

int speedDelay = 20;

unsigned long lastBeep;
unsigned long alarm_start_time;
unsigned long lastDisplayUpdate = 0;

// ============================================================
// DISPLAY MODES
// ============================================================
enum DisplayMode {
  MODE_RADAR = 0,
  MODE_GRAPH = 1,
  MODE_PLANE = 2,
  MODE_MANUAL = 3
};

DisplayMode currentMode = MODE_RADAR;

// ============================================================
// DATA BUFFER HELPERS
// ============================================================

// Shift graph samples left and append newest sample at the end
void pushDistanceSample(int d) {
  for (int i = 0; i < GRAPH_SAMPLES - 1; i++) {
    distanceHistory[i] = distanceHistory[i + 1];
  }
  distanceHistory[GRAPH_SAMPLES - 1] = d;
}

// ============================================================
// SPEED CONTROL
// ============================================================

// Read potentiometer and convert to a sweep delay value
int readSpeedDelay() {
  static int lastValue = 20;

  int potValue = analogRead(POT_PIN);

  // Reverse mapping so clockwise = faster sweep
  int delayTime = map(potValue, 0, 1023, 100, 1);

  // Simple smoothing
  delayTime = (delayTime + lastValue) / 2;
  lastValue = delayTime;

  return constrain(delayTime, 1, 100);
}

int getSweepDelay() {
  return 110 - readSpeedDelay();
}

// ============================================================
// GENERAL HELPERS
// ============================================================
//---------- Keypad Functions -----------



char check_keys(){

  Wire.requestFrom(0x23, 1);
  uint8_t pcf2_in = Wire.read();

  Wire.requestFrom(0x27, 1);
  uint8_t pcf1_in = Wire.read();

  for(int i = 0; i < 8; i++){

    if(i == 3 || i == 5) continue;

    bool keyPressed = !((pcf1_in >> i) & 1); // Shifts important bit to end and sets all other bits to 0
    //Value keyPressed is 8'b00000000 if key is pressed and 8'b00000001 if not.

  if(keyPressed && !((keyStates >> i) & 1)) {
      keyStates |= (1 << i);   
      return pgm_read_byte(&keys[i]);
  }
  if(!keyPressed) keyStates &= ~(1 << i);
  }

  for(int i = 0; i < 8; i++){

    if(i == 3 || i == 5) continue;

      bool keyPressed = !((pcf2_in >> i) & 1); // Shifts important bit to end and sets all other bits to 0
      //Value keyPressed is 8'b00000000 if key is pressed and 8'b00000001 if not.

  if(keyPressed && !((keyStates >> (i + 8)) & 1)) {
    keyStates |= (1 << (i + 8));
    return pgm_read_byte(&keys[i + 8]);
  }

  if(!keyPressed) keyStates &= ~(1 << (i + 8));
  }

  return '-'; // returns - if no key is pressed

}

void key_handler(char key) {

  if(key == '-') return; // ends function if key was not pressed

  // If '*' pressed → request verification
  if(key == '*' && current_alarm_state == ENTER_CODE) {
    counter = 0;
    current_alarm_state = VERIFY_FOR_RESET;
    //Serial.println(F("ENTER CURRENT CODE TO RESET:"));
    return;
  }

  // Ignore '#' unless needed
  if(key == '#') {
    input[0] = '\0';
    counter = 0;
    return;
  }

  // Store digit
  if (counter < 4 && key >= '0' && key <= '9') {
    input[counter++] = key;
    input[counter] = '\0';
  }

  if(counter == 4) {

    switch(current_alarm_state) {

      case ENTER_CODE:
        if(strcmp(input,code) == 0) {
          //Serial.println(F("Code Correct"));
          alarm_on = false;
          alarm_start_time = 0;
          noTone(SPEAKER_PIN);
        } else {
            //Serial.println(F("Code Incorrect"));
        }
        break;

      case VERIFY_FOR_RESET:
        if(strcmp(input,code) == 0) {
          //Serial.println(F("VERIFIED. ENTER NEW CODE:"));
          current_alarm_state = SET_NEW_CODE;
        } else {
          //Serial.println(F("WRONG CODE. RESET DENIED."));
          current_alarm_state = ENTER_CODE;
        }
        break;

      case SET_NEW_CODE:
        strcpy(code, input);
        //Serial.println(F("NEW CODE SAVED."));
        current_alarm_state = ENTER_CODE;
        break;
    }
    input[0] = '\0';
    counter = 0;
  }
}


// Adjust a calibration value using joystick Y axis
int adjustValue(int value, int minVal, int maxVal) {
  int joyY = analogRead(JOY_Y);   // read vertical axis
  int offset = joyY - 512;        // center around 0

  // Dead zone around joystick center
  if (abs(offset) < JOY_THRESHOLD){
    char keypad_input = check_keys();

  if (keypad_input == '#') {
    if (counter > 0) {
      value = constrain(atoi(input), minVal, maxVal);
    }
    counter = 0;
    input[0] = '\0';
    return value;
  }

    if (keypad_input >= '0' && keypad_input <= '9' && counter < 4) {
    input[counter++] = keypad_input;
    input[counter] = '\0';
    }

  return value;
  }

  // Move value depending on joystick direction
  else{
    counter = 0;
    input[0] = '\0';
    if (offset > 0) value--;
    if(offset < 0) value++;
  }

  // Clamp to allowed range
  value = constrain(value, minVal, maxVal);

  return value;
}

void hapticFeedback() {
  tone(SPEAKER_PIN, 900, 80);
}

// ============================================================
// CALIBRATION
// ============================================================

// Generic calibration screen for one numeric value
void calibrateStep(const char* label, int &value, int minVal, int maxVal) {
    input[0] = '\0';
  counter = 0;
  
  while (true) {
    value = adjustValue(value, minVal, maxVal);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(label);
    display.println(F("Move joystick"));
    display.println(F("Press to confirm"));
    display.setCursor(0, 40);
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

// Dedicated calibration screen for sweep speed using potentiometer
void calibrateSpeed() {
  int lastShown = -1;

  while (true) {
    int current = readSpeedDelay();

    if (current != lastShown) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Set Sweep Speed"));
      display.println(F("Turn knob"));
      display.println(F("Press to confirm"));
      display.setCursor(0, 40);
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

// ============================================================
// SENSOR READING
// ============================================================

// Trigger ultrasonic sensor and return measured distance in cm
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;

  if (distance > MAX_DISTANCE) distance = 0;

  return distance;
}

// ============================================================
// DISPLAY DRAWING FUNCTIONS
// ============================================================

// ---------- Mode 0: Radar ----------
void drawMode1Radar(int currentAngle, int currentDistance) {
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT - 1;

  // Shift radar trail history
  for (int i = TRAIL_LENGTH - 1; i > 0; i--) {
    angleTrail[i] = angleTrail[i - 1];
    distanceHistory[i] = distanceHistory[i - 1];
  }

  angleTrail[0] = currentAngle;
  distanceHistory[0] = currentDistance;

  display.clearDisplay();

  // Radar range circles
  for (int r = 16; r <= SWEEP_RADIUS; r += 10) {
    display.drawCircle(centerX, centerY, r, WHITE);
  }

  // Baseline
  display.drawLine(0, centerY, SCREEN_WIDTH, centerY, WHITE);

  // Angle guide lines
  for (int a = 30; a <= 150; a += 30) {
    float rad = a * 3.14159 / 180.0;

    display.drawLine(
      centerX,
      centerY,
      centerX - SWEEP_RADIUS * cos(rad),
      centerY - SWEEP_RADIUS * sin(rad),
      WHITE
    );
  }

  // Draw sweep trail from oldest to newest
  for (int i = TRAIL_LENGTH - 1; i >= 0; i--) {
    float rad = angleTrail[i] * 3.14159 / 180.0;

    int lineLength = SWEEP_RADIUS;

    if (distanceHistory[i] > 0) {
      lineLength = map(distanceHistory[i], 0, thresholdDistance, 0, SWEEP_RADIUS);
      if (lineLength > SWEEP_RADIUS) lineLength = SWEEP_RADIUS;
    }

    int pixelStep = 1 + i;

    for (int l = 0; l < lineLength; l += pixelStep) {
      int x = centerX - l * cos(rad);
      int y = centerY - l * sin(rad);
      display.drawPixel(x, y, WHITE);
    }

    // Highlight the newest valid detection
    if (i == 0 && distanceHistory[i] > 0 && distanceHistory[i] <= thresholdDistance) {
      int x = centerX - lineLength * cos(rad);
      int y = centerY - lineLength * sin(rad);
      display.fillCircle(x, y, 2, WHITE);
    }
  }

  // Status text
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Angle: "));
  display.print(currentAngle);

  display.setCursor(0, 10);
  display.print(F("Dist: "));
  display.print(currentDistance);
  display.print(F("cm"));

  display.display();
}

// ---------- Mode 1: B-SCAN ----------
void drawMode2Graph(long currentDistance) {
  display.clearDisplay();

  const int graphX = 0;
  const int graphY = 10;
  const int graphW = SCREEN_WIDTH;
  const int graphH = 44;   // leave room for text at bottom

  const int xAxisY = graphY + graphH - 1;
  const int yAxisX = graphX;

  // Use calibrated threshold as graph range
  int graphMaxDist = thresholdDistance;

  // Axes
  display.drawLine(yAxisX, graphY, yAxisX, xAxisY, WHITE);
  display.drawLine(graphX, xAxisY, graphX + graphW - 1, xAxisY, WHITE);

  // Threshold marker
  int threshY = map(thresholdDistance, 0, graphMaxDist, xAxisY, graphY);
  display.drawLine(graphX, threshY, graphX + graphW - 1, threshY, WHITE);

  // Plot history
  for (int x = 1; x < GRAPH_SAMPLES; x++) {
    int d1 = constrain(distanceHistory[x - 1], 0, graphMaxDist);
    int d2 = constrain(distanceHistory[x], 0, graphMaxDist);

    int y1 = map(d1, 0, graphMaxDist, xAxisY, graphY);
    int y2 = map(d2, 0, graphMaxDist, xAxisY, graphY);

    display.drawLine(x - 1, y1, x, y2, WHITE);
  }

  // Status text
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("B-SCAN"));

  display.setCursor(0, 56);
  display.print(F("Distance: "));
  display.print(currentDistance);
  display.print(F(" cm"));

  display.display();
}

// ---------- Mode 2: Volumetric plane ----------
void drawMode3Plane(int hAngle, int vAngle, int distance) {
  if (distance <= 0 || distance > thresholdDistance) return;

  int x = map(hAngle, hMin, hMax, 0, SCREEN_WIDTH - 1);
  int y = map(vAngle, vMin, vMax, SCREEN_HEIGHT - 1, 0);

  int thickness = map(distance, 1, thresholdDistance, 5, 1);
  thickness = constrain(thickness, 1, 5);

  int half = thickness / 2;

  int left = max(0, x - half);
  int top = max(0, y - half);
  int width = min(SCREEN_WIDTH - left, thickness);
  int height = min(SCREEN_HEIGHT - top, thickness);

  display.fillRect(left, top, width, height, WHITE);

  display.setCursor(0, 0);
  display.print(F("Volumetric"));
}

// ---------- Mode 3: Manual HUD ----------
void drawManualHUD(long distance) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("MANUAL MODE"));

  display.setCursor(0, 16);
  display.print(F("H: "));
  display.print(hAngle);

  display.setCursor(0, 26);
  display.print(F("V: "));
  display.print(vAngle);

  display.setCursor(0, 40);
  display.print(F("Dist: "));
  display.print(distance);
  display.print(F("cm"));

  if (distance > 0 && distance < thresholdDistance) {
    display.setCursor(0, 54);
    display.print(F("OBJECT DETECTED"));
  }

  display.display();
}

// ============================================================
// SOUND
// ============================================================

// Continuous tone changes depending on sweep angle
void sonarSweepSound(int angle) {
  int center = (hMin + hMax) / 2;
  int distFromCenter = abs(angle - center);

  int freq = map(distFromCenter, 0, (hMax - hMin) / 2, 650, 350);

  tone(SPEAKER_PIN, freq);
}

// Short beep when an object is detected
void detectionBeep() {
  tone(SPEAKER_PIN, 1200, 100);
}

void alarm_sound() {
  if(millis() - lastBeep> 400){
    static bool alarmPhase = false;

    if (alarmPhase) {
      tone(SPEAKER_PIN, 1800);
    } else {
      tone(SPEAKER_PIN, 900);
    }
    alarmPhase = !alarmPhase;
    lastBeep = millis();
  }
}

void alarm(){
  key_handler(check_keys());
  if(currentMode != MODE_PLANE){
    display.setTextSize(1);
    display.setCursor(70, 0);
    display.print(F("Alarm On"));
    display.setCursor(70, 10);
    display.print(F("Code:"));
    display.setCursor(100, 10);
    display.print(input);
    display.display();
}

  if((millis() - alarm_start_time) > ((unsigned long)entry_delay * 1000)) alarm_sound();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  // Initialise graph history buffer
  for (int i = 0; i < GRAPH_SAMPLES; i++) {
    distanceHistory[i] = 0;
  }

  // Configure I/O pins
  pinMode(JOY_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  // Attach servos
  hServo.attach(H_SERVO_PIN);
  vServo.attach(V_SERVO_PIN);


  // Start OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    for (;;);
  }

  if (!pcf1.begin(0x27, &Wire)) while (1);
  if (!pcf2.begin(0x23, &Wire)) while (1);

  for (int i = 0; i < 8; i++) {
    pcf1.digitalWrite(i, HIGH);
    pcf2.digitalWrite(i, HIGH);
  }

  display.setTextColor(WHITE);
  display.setTextSize(1);

  // Calibration sequence
  calibrateStep("H Min", hMin, 0, hMax - 1);
  calibrateStep("H Max", hMax, hMin + 1, 180);
  calibrateStep("V Min", vMin, 0, vMax - 1);
  calibrateStep("V Max", vMax, vMin + 1, 180);
  calibrateStep("Max Distance", thresholdDistance, 1, 50);
  alarm_range = thresholdDistance; // sets the default value of alarm_range to threshold distance that was just set
  calibrateStep("Arm Alarm", alarm_armed, 0, 1);
  if (alarm_armed) {

    calibrateStep("Entry Delay", entry_delay, 0, 240);
    calibrateStep("Alarm trigger range", alarm_range, 1, 50);
  }

  calibrateSpeed();

  // Start servos at calibrated minimums
  hAngle = hMin;
  vAngle = vMin;

  hServo.write(hAngle);
  vServo.write(vAngle);

  // Ready screen
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println(F("Ready"));
  display.display();

  delay(1000);
  display.clearDisplay();


}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  // ----------------------------------------------------------
  // Mode toggle using touch input (edge detect)
  // ----------------------------------------------------------
  static bool lastTouchState = LOW;
  bool currentTouchState = digitalRead(TOUCH_PIN);

  if (currentTouchState == HIGH && lastTouchState == LOW) {
    currentMode = (DisplayMode)((currentMode + 1) % 4);
    hapticFeedback();

    display.clearDisplay();
    display.display();

    delay(10);
  }

  lastTouchState = currentTouchState;

  // ----------------------------------------------------------
  // Manual mode
  // ----------------------------------------------------------
  if (currentMode == MODE_MANUAL) {
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

    // Refresh manual HUD at a slower rate
    if (millis() - lastDisplayUpdate > 120) {
      long distance = readDistanceCM();
      drawManualHUD(distance);

      lastDisplayUpdate = millis();
    }

    delay(getSweepDelay());
    return;
  }

  // ----------------------------------------------------------
  // Sweep-based modes
  // ----------------------------------------------------------
  long distance = readDistanceCM();

  // Draw current mode
  if (currentMode == MODE_GRAPH) {
    drawMode2Graph(distance);
  }
  else if (currentMode == MODE_RADAR) {
    drawMode1Radar(hAngle, distance);
  }
  else if (currentMode == MODE_PLANE) {
    drawMode3Plane(hAngle, vAngle, distance);
    display.display();
  }

  // Wrap vertical sweep when needed
  if (vAngle > vMax) {
    vAngle = vMin;

    if (currentMode == MODE_PLANE) {
      display.clearDisplay();
    }
  }

  if (alarm_armed && distance > 0 && distance <= alarm_range && !(alarm_on)) {
  alarm_start_time = millis();
  alarm_on = true;
  noTone(SPEAKER_PIN);

}

  if (alarm_on) {
    alarm();
  }

  // Detection beep / idle sonar tone
  else if (distance > 0 && distance <= thresholdDistance) {
    if (millis() - lastBeep > 400) {
      detectionBeep();
      pushDistanceSample((uint8_t)constrain(distance, 0, MAX_DISTANCE));
      lastBeep = millis();
    }
  }
  else {
    sonarSweepSound(hAngle);
  }

  // Horizontal sweep motion
  hAngle += hStep;

  if (hAngle >= hMax || hAngle <= hMin) {
    hStep = -hStep;
    vAngle += vStep;
  }

  hServo.write(hAngle);
  vServo.write(vAngle);

  delay(getSweepDelay());
}


