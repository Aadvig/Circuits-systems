// Thumb Joystick V1.1 Direction Mapping with Degrees

const int VRx = A0; // X-axis
const int VRy = A1; // Y-axis

// Joystick calibration
const int analogMin = 0;     // minimum ADC reading
const int analogMax = 1023;  // maximum ADC reading
const int deadZone = 50;     // ignore small movements around center

void setup() {
  Serial.begin(9600);
}

void loop() {
  int xVal = analogRead(VRx);
  int yVal = analogRead(VRy);

  // Map analog values to -512..+511
  int xOffset = xVal - 512;
  int yOffset = yVal - 512;

  // Apply dead zone
  if (abs(xOffset) < deadZone) xOffset = 0;
  if (abs(yOffset) < deadZone) yOffset = 0;

  // Map to degrees: full left/right = 90°, full up/down = 90°
  int xDegrees = map(xOffset, -512, 511, -90, 90);
  int yDegrees = map(yOffset, -512, 511, -90, 90);

  // Determine horizontal direction
  String xDir = "Center";
  if (xDegrees > 0) xDir = "Right";
  else if (xDegrees < 0) xDir = "Left";

  // Determine vertical direction
  String yDir = "Center";
  if (yDegrees > 0) yDir = "Down";  // Arduino analog Y is usually inverted
  else if (yDegrees < 0) yDir = "Up";

  // Print results
  Serial.print("Horizontal: ");
  Serial.print(xDir);
  Serial.print(" (");
  Serial.print(abs(xDegrees));
  Serial.print("°)\tVertical: ");
  Serial.print(yDir);
  Serial.print(" (");
  Serial.print(abs(yDegrees));
  Serial.println("°)");

  delay(200);
}
