#define STATE_PIN0 A4
#define STATE_PIN1 A5

void setup() {
  Serial.begin(9600);

  pinMode(STATE_PIN0, INPUT);
  pinMode(STATE_PIN1, INPUT);
}

void loop() {
  byte state = (digitalRead(STATE_PIN1) << 1) | digitalRead(STATE_PIN0);

  switch(state){
    case 0:
      Serial.println("Distance Calibration");
      break;
    case 1:
      Serial.println("Rotation Calibration");
      break;
    case 2:
      Serial.println("Monitoring / Ready");
      break;
    case 3:
      Serial.println("Object Detected");
      break;
  }

  delay(200);
}
