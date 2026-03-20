#include <Wire.h>
#include <Adafruit_PCF8574.h>

Adafruit_PCF8574 pcf1;

  bool pressedKeys[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
  bool keyStates[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
  char keys[12] = {'a','b','*','d','7','f','1','0','i','j','k','l'} ;
  char code[4] = {'1','2','3','4'};
  char input[4];
  int counter = 0;
void setup() {

  Serial.begin(9600);

  Wire.begin();

  if (!pcf1.begin(0x27, &Wire)) {
    Serial.println("PCF8574 not found!");
    while (1);
  }

  // Write HIGH to all pins so they act as inputs
  for (int i = 0; i < 8; i++) {
    pcf1.digitalWrite(i, HIGH);
  }

  Serial.println("Ready");
}

bool keyPressed(int pin){
  


}

void checkKeys(){

}

void loop() {


  for(int i = 0; i < 8; i++){

    if(i == 5) continue;


    if(pcf1.digitalRead(i) < keyStates[i]){  // If key is pressed and keyState is = not pressed
      Serial.println(keys[i]);
      input[counter] = keys[i];
      counter++;
      delay(200);
    }
    keyStates[i] = pcf1.digitalRead(i);
  }

  if(counter == 4) {
    if(input[0] == code[0] && input[1] == code[1] && input[2] == code[2] && input[3] == code[3]){

       Serial.println("Code Correct");
       digitalWrite(15,HIGH); // Set D15 to HIGH if code is entered correctly
    }
    else Serial.println("Code INCORRECT");

    counter = 0;
  }
}
