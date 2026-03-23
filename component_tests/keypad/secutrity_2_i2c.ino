#include <Wire.h>
#include <Adafruit_PCF8574.h>

Adafruit_PCF8574 pcf1;
Adafruit_PCF8574 pcf2;

  bool pressedKeys[16] = {0};
  bool keyStates[16] = {0};
  char keys[16] = {'4','9','*','d','7','f','1','0','8','5','2','l','#','n','6','3'} ;
  char code[4] = {'1','2','3','4'};
  char input[4];
  int counter = 0;

enum SystemState {
  ENTER_CODE,
  VERIFY_FOR_RESET,
  SET_NEW_CODE
};

SystemState currentState = ENTER_CODE;

void setup() {

  Serial.begin(9600);

  Wire.begin();

  if (!pcf1.begin(0x27, &Wire)) {
    Serial.println("PCF8574 not found!");
    while (1);
  }

    if (!pcf2.begin(0x23, &Wire)) {
    Serial.println("PCF8574 not found!");
    while (1);
  }

  // Write HIGH to all pins so they act as inputs
  for (int i = 0; i < 8; i++) {
    pcf1.digitalWrite(i, HIGH);
    pcf2.digitalWrite(i, HIGH);
  }

  Serial.println("Ready");
}

// ======================================================
// ---------------- CODE CHECK --------------------------
// ======================================================

bool isCodeCorrect() {
  for(int i = 0; i < 4; i++){
    if(input[i] != code[i]) return false;
  }
  return true;
}


// ======================================================
// ---------------- KEY HANDLER -------------------------
// ======================================================

void handleKeyPress(char key) {

  // If '*' pressed → request verification
  if(key == '*' && currentState == ENTER_CODE) {
    counter = 0;
    currentState = VERIFY_FOR_RESET;
    Serial.println("ENTER CURRENT CODE TO RESET:");
    return;
  }

  // Ignore '#' unless needed
  if(key == '#') {
    counter = 0;
    return;
  }

  // Store digit
  if(counter < 4) {
    input[counter++] = key;
    Serial.println(key);
  }

  if(counter == 4) {

    switch(currentState) {

      case ENTER_CODE:
        if(isCodeCorrect()) {
          Serial.println("CODE CORRECT");
          digitalWrite(15, HIGH);
        } else {
          Serial.println("CODE INCORRECT");
          digitalWrite(15, LOW);
        }
        break;

      case VERIFY_FOR_RESET:
        if(isCodeCorrect()) {
          Serial.println("VERIFIED. ENTER NEW CODE:");
          currentState = SET_NEW_CODE;
        } else {
          Serial.println("WRONG CODE. RESET DENIED.");
          currentState = ENTER_CODE;
        }
        break;

      case SET_NEW_CODE:
        for(int i = 0; i < 4; i++){
          code[i] = input[i];
        }
        Serial.println("NEW CODE SAVED.");
        currentState = ENTER_CODE;
        break;
    }

    counter = 0;
  }
}

void loop() {


  for(int i = 0; i < 8; i++){

    if(i == 5) continue;


    if(pcf1.digitalRead(i) < keyStates[i]){  // If key is pressed and keyState is = not pressed
      handleKeyPress(keys[i]);
      delay(200);
    }
    keyStates[i] = pcf1.digitalRead(i);
  }

    for(int i = 0; i < 8; i++){

    if(i == 5) continue;


    if(pcf2.digitalRead(i) < keyStates[i]){  // If key is pressed and keyState is = not pressed
      handleKeyPress(keys[i + 8]);
      delay(200);
    }
    keyStates[i + 8] = pcf2.digitalRead(i);
  }
}
