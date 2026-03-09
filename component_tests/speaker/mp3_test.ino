#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define DFPLAYER_RX 2 // Arduino RX ← DFPlayer TX
#define DFPLAYER_TX 4 // Arduino TX → DFPlayer RX
#define POT_PIN A4    // potentiometer for volume

SoftwareSerial mySoftwareSerial(DFPLAYER_RX, DFPLAYER_TX);
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);

  Serial.println("=== DFPlayer Mini Debug Test ===");

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("ERROR: DFPlayer initialization failed!");
    Serial.println("Check wiring and SD card files: 0001.mp3, 0002.mp3, 0003.mp3");
    while(true);
  }
  Serial.println("DFPlayer initialized successfully!");

  myDFPlayer.volume(20);
  Serial.println("Initial volume set to 20");
  delay(500);
}

void loop() {
  int potVal = analogRead(POT_PIN);
  int volume = map(potVal, 0, 1023, 0, 30);
  myDFPlayer.volume(volume);
  Serial.print("Potentiometer: "); Serial.print(potVal);
  Serial.print(" → Volume: "); Serial.println(volume);

  Serial.println("Playing file 1 (Alarm)");
  myDFPlayer.play(1);
  delay(2000);

  Serial.println("Playing file 2 (Haptic)");
  myDFPlayer.play(2);
  delay(2000);

  Serial.println("Playing file 3 (Sweep)");
  myDFPlayer.play(3);
  delay(2000);

  Serial.println("--- Loop finished, repeating ---");
}
