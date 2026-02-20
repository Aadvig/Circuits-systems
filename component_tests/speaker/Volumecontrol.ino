const int potPin = A2;  // Potentiometer input

void setup() {
  Serial.begin(9600);
}

void loop() {
  int potValue = analogRead(potPin);   // 0–1023
  int flippedValue = 1023 - potValue;  // Flip direction
  Serial.println(flippedValue);
  delay(200);
}
