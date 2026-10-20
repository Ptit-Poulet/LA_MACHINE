// // test nema 17 avec DRV8825
// const int STEP = 26;
// const int DIR = 25;
// const int ENA = 17; // Disable Driver HIGH = true, LOW = False
// const int stepsPerRevolution = 1000;

// void setup() {
//  Serial.begin(115200);
//  pinMode(STEP, OUTPUT);
//  pinMode(DIR, OUTPUT);

// }

// void loop() {
//   // Set motor direction clockwise
//   digitalWrite(DIR, HIGH);

//   // Spin motor slowly
//   for (int x = 0; x < stepsPerRevolution; x++) {
//     digitalWrite(STEP, HIGH);
//     delayMicroseconds(2000);
//     digitalWrite(STEP, LOW);
//     delayMicroseconds(2000);
//   }
//   delay(1000);  // Wait a second

//   // Set motor direction counterclockwise
//   digitalWrite(DIR, LOW);

//   // Spin motor quickly
//   for (int x = 0; x < stepsPerRevolution; x++) {
//     digitalWrite(STEP, HIGH);
//     delayMicroseconds(1000);
//     digitalWrite(STEP, LOW);
//     delayMicroseconds(1000);
//   }
//   delay(1000);  // Wait a second
// }

// Stepper Motor Control (Forward & Reverse)
// Using ESP32 + DRV8825 + NEMA 17

// #define STEP_PIN 18
// #define DIR_PIN 19

// // Steps per revolution (depends on your microstepping setup)
// // Example: 200 steps/rev * 16 microsteps = 3200
// const int stepsPerRevolution = 800;

// void setup() {
//   pinMode(STEP_PIN, OUTPUT);
//   pinMode(DIR_PIN, OUTPUT);
// }

// void loop() {
//   // ----- FORWARD -----
//   digitalWrite(DIR_PIN, HIGH);  // Set direction to forward
//   rotateOneRevolution();
//   delay(500); // Pause before reversing

//   // ----- REVERSE -----
//   digitalWrite(DIR_PIN, LOW);   // Set direction to reverse
//   rotateOneRevolution();
//   delay(500); // Pause before next cycle
// }

// void rotateOneRevolution() {
//   for (int i = 0; i < stepsPerRevolution; i++) {
//     digitalWrite(STEP_PIN, HIGH);
//     delayMicroseconds(1600);  // Adjust for speed (higher = slower)
//     digitalWrite(STEP_PIN, LOW);
//     delayMicroseconds(100);
//   }
// }

/* Example sketch to control a stepper motor with 
   A4988/DRV8825 stepper motor driver and 
   Arduino without a library. 
   More info: https://www.makerguides.com */

// Define stepper motor connections and steps per revolution:
#define dirPin 19
#define stepPin 18
#define stepsPerRevolution 200

void setup() {
  // Declare pins as output:
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
}

void loop() {
  // Set the spinning direction clockwise:
  digitalWrite(dirPin, HIGH);

  // Spin the stepper motor 1 revolution slowly:
  for (int i = 0; i < stepsPerRevolution; i++) {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(2000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(2000);
  }

  delay(1000);

  // Set the spinning direction counterclockwise:
  digitalWrite(dirPin, LOW);

  // Spin the stepper motor 1 revolution quickly:
  for (int i = 0; i < stepsPerRevolution; i++) {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000);
  }

  delay(1000);

  // Set the spinning direction clockwise:
  digitalWrite(dirPin, HIGH);

  // Spin the stepper motor 5 revolutions fast:
  for (int i = 0; i < 5 * stepsPerRevolution; i++) {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }

  delay(1000);

  // Set the spinning direction counterclockwise:
  digitalWrite(dirPin, LOW);

  //Spin the stepper motor 5 revolutions fast:
  for (int i = 0; i < 5 * stepsPerRevolution; i++) {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }

  delay(1000);
}