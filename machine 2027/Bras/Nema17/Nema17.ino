// test nema 17 avec DRV8825
const int STEP = 26;
const int DIR = 25;
const int ENA = 17; // Disable Driver HIGH = true, LOW = False
const int stepsPerRevolution = 1000;

void setup() {
 Serial.begin(115200);
 pinMode(STEP, OUTPUT);
 pinMode(DIR, OUTPUT);

}

void loop() {
  // Set motor direction clockwise
  digitalWrite(DIR, HIGH);

  // Spin motor slowly
  for (int x = 0; x < stepsPerRevolution; x++) {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(2000);
    digitalWrite(STEP, LOW);
    delayMicroseconds(2000);
  }
  delay(1000);  // Wait a second

  // Set motor direction counterclockwise
  digitalWrite(DIR, LOW);

  // Spin motor quickly
  for (int x = 0; x < stepsPerRevolution; x++) {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STEP, LOW);
    delayMicroseconds(1000);
  }
  delay(1000);  // Wait a second
}