


  bool pressedKeys[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
  bool keyStates[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
  char keys[12] = {'*','7','4','1','0','8','5','2','#','9','6','3'} ;
  char code[4] = {'1','2','3','4'};
  char input[4];
  int counter = 0;
void setup() {

  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
  pinMode(6,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);
  pinMode(8,INPUT_PULLUP);
  pinMode(9,INPUT_PULLUP);
  pinMode(10,INPUT_PULLUP);
  pinMode(11,INPUT_PULLUP);
  pinMode(12,INPUT_PULLUP);
  pinMode(13,INPUT_PULLUP);
  pinMode(14,INPUT_PULLUP);
  pinMode(15,OUTPUT);
  pinMode(21,INPUT);

  


  Serial.begin(9600);
}

bool keyPressed(int pin){
  


}

void checkKeys(){

}

void loop() {


  for(int i = 0; i < 12; i++){


    if(digitalRead(i+3) < keyStates[i]){  // If key is pressed and keyState is = not pressed
      Serial.println(keys[i]);
      input[counter] = keys[i];
      counter++;
      delay(200);
    }
    keyStates[i] = digitalRead(i+3);
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
