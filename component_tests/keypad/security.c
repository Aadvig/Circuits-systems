bool keyStates[12] = {0};
char keys[12] = {'*','7','4','1','0','8','5','2','#','9','6','3'};

char code[4] = {'1','2','3','4'};
char input[4];

int counter = 0;

enum SystemState {
  ENTER_CODE,
  VERIFY_FOR_RESET,
  SET_NEW_CODE
};

SystemState currentState = ENTER_CODE;


// ======================================================
// ---------------- SETUP -------------------------------
// ======================================================

void setup() {

  for(int i = 3; i <= 14; i++){
    pinMode(i, INPUT_PULLUP);
  }

  pinMode(15, OUTPUT);

  Serial.begin(9600);
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


// ======================================================
// ---------------- LOOP --------------------------------
// ======================================================

void loop() {

  for(int i = 0; i < 12; i++){

    int pin = i + 3;
    bool currentStateKey = digitalRead(pin);

    // Detect press (falling edge)
    if(currentStateKey == LOW && keyStates[i] == HIGH) {
      handleKeyPress(keys[i]);
      delay(200);  // debounce
    }

    keyStates[i] = currentStateKey;
  }
}
