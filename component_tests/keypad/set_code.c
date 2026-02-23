bool keyStates[12] = {0};
char keys[12] = {'*','7','4','1','0','8','5','2','#','9','6','3'};

char code[4] = {'1','2','3','4'};   // Default code (can be changed)
char input[4];

int counter = 0;
bool settingMode = false;   // false = normal mode, true = setting new code


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
// ---------------- KEY HANDLING ------------------------
// ======================================================

void handleKeyPress(char key) {

  // Enter code setting mode
  if(key == '*') {
    settingMode = true;
    counter = 0;
    Serial.println("ENTER NEW 4 DIGIT CODE:");
    return;
  }

  // Confirm key (#)
  if(key == '#') {

    if(settingMode && counter == 4) {
      // Save new code
      for(int i = 0; i < 4; i++){
        code[i] = input[i];
      }

      Serial.println("NEW CODE SET!");
      settingMode = false;
    }

    counter = 0;
    return;
  }

  // Store digit if 0–9
  if(counter < 4) {
    input[counter] = key;
    counter++;
    Serial.println(key);
  }

  // If 4 digits entered in normal mode → check code
  if(counter == 4 && !settingMode) {
    checkCode();
    counter = 0;
  }
}


// ======================================================
// ---------------- CODE CHECK --------------------------
// ======================================================

void checkCode() {

  bool correct = true;

  for(int i = 0; i < 4; i++){
    if(input[i] != code[i]) {
      correct = false;
      break;
    }
  }

  if(correct) {
    Serial.println("CODE CORRECT");
    digitalWrite(15, HIGH);
  }
  else {
    Serial.println("CODE INCORRECT");
    digitalWrite(15, LOW);
  }
}


// ======================================================
// ---------------- LOOP --------------------------------
// ======================================================

void loop() {

  for(int i = 0; i < 12; i++){

    int pin = i + 3;
    bool currentState = digitalRead(pin);

    // Detect falling edge (button press)
    if(currentState == LOW && keyStates[i] == HIGH) {
      handleKeyPress(keys[i]);
      delay(200);  // debounce
    }

    keyStates[i] = currentState;
  }
}
