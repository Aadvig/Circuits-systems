const int speakerPin = 5;    // PWM output → LM386
const int touchPin = 12;      // Touch sensor input

bool alarmOn = false;         // Alarm state
bool lastTouchState = LOW;    // For detecting changes

void setup() {
  pinMode(speakerPin, OUTPUT);
  pinMode(touchPin, INPUT);
  Serial.begin(9600);         // Optional: debug
}

void loop() {
  bool touchState = digitalRead(touchPin);

  // Detect rising edge (touch just pressed)
  if (touchState == HIGH && lastTouchState == LOW) {
    alarmOn = !alarmOn;       // Toggle alarm
    Serial.print("Alarm state: ");
    Serial.println(alarmOn ? "ON" : "OFF");
  }
  lastTouchState = touchState;

  // If alarm is on, beep the speaker
  if (alarmOn) {
    digitalWrite(speakerPin, HIGH);
    delay(200);               // beep on duration
    digitalWrite(speakerPin, LOW);
    delay(200);               // beep off duration
  } else {
    digitalWrite(speakerPin, LOW); // ensure speaker stays off
  }
}
