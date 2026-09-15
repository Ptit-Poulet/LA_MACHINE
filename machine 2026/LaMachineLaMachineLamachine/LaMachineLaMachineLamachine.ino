#include <ESP32Servo.h>

// --- PIN DEFINITIONS ---
// Receiver Inputs
#define CH1_PIN 32
#define CH2_PIN 35      // Right side control
#define CH4_PIN 34      // Left side control
#define CH9_PIN 4       // 3-State Switch for Continuous Servo

// Motor Driver (Actuator) Pins
#define in1 16
#define in2 17
#define in3 23
#define in4 22

// Drive Servo Pins
#define L_SERVO_PIN_f 25
#define L_SERVO_PIN_b 33 
#define R_SERVO_PIN_f 27
#define R_SERVO_PIN_b 26


// --- OBJECTS & CONSTANTS ---
Servo leftServof, rightServof, leftServob, rightServob;


// Calibration for Continuous Servo
const int CONT_STOP  = 95;  // Adjusted to 91 to stop the rightward creep
const int CONT_LEFT  = 70;
const int CONT_RIGHT = 120;

float speedMultiplier = 1.0; 

// --- FUNCTIONS ---
int applyDeadband(int value, int threshold = 10) {
  return (abs(value) < threshold) ? 0 : value;
}

void stopMotors() {
  leftServof.write(90);
  rightServof.write(90);
  leftServob.write(90);
  rightServob.write(90);
}

void setup() {
  Serial.begin(115200);

  // Receiver & Switch Pins
  pinMode(CH1_PIN, INPUT);
  pinMode(CH2_PIN, INPUT);
  pinMode(CH4_PIN, INPUT);
  pinMode(CH9_PIN, INPUT);


  // Actuator Pins
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // Timer Allocation for ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Attach Drive Servos
  leftServof.attach(L_SERVO_PIN_f, 1000, 2000);
  rightServof.attach(R_SERVO_PIN_f, 1000, 2000);
  leftServob.attach(L_SERVO_PIN_b, 1000, 2000);
  rightServob.attach(R_SERVO_PIN_b, 1000, 2000);

  // Attach Continuous Servo

  
  stopMotors();

}

void loop() {
  // 1. READ RC SIGNALS
  int ch1 = pulseIn(CH1_PIN, HIGH, 25000);
  int ch2 = pulseIn(CH2_PIN, HIGH, 25000);  // Right side control
  int ch4 = pulseIn(CH4_PIN, HIGH, 25000);  // Left side control
  int ch9 = pulseIn(CH9_PIN, HIGH, 25000);  // 3-state switch



  // Failsafe: Stop all if radio signal is lost
  if (ch2 < 500 || ch2 > 2500 || ch4 < 500 || ch4 > 2500) {
    stopMotors();

    return;
  }

  // 3. DRIVE SYSTEM (Independent Left/Right Control)
  // Map CH4 (Left stick) to left side servos
  int leftSpeed = map(ch4, 1000, 2000, -90, 90) * speedMultiplier;
  leftSpeed = applyDeadband(leftSpeed);
  leftSpeed = constrain(leftSpeed, -90, 90);

  // Map CH2 (Right stick) to right side servos
  int rightSpeed = map(ch2, 1000, 2000, -90, 90) * speedMultiplier;
  rightSpeed = applyDeadband(rightSpeed);
  rightSpeed = constrain(rightSpeed, -90, 90);

  // Apply to servos
  leftServof.write(90 + leftSpeed);
  leftServob.write(90 + leftSpeed);
  
  rightServof.write(90 - rightSpeed);
  rightServob.write(90 - rightSpeed);

  // 4. DC ACTUATOR (CH1)
  if (ch1 > 1700) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (ch1 < 1300 && ch1 > 500) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  // 5. DC ACTUATOR (CH9)
  if (ch9 > 1700 && ch9 < 2200) {
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  } else if (ch9 < 1300 && ch9 > 500) {
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  } else {
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
  }


  delay(20);  // 4. DC ACTUATOR (CH1)
  if (ch1 > 1700) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (ch1 < 1300 && ch1 > 500) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  // 5. DC ACTUATOR (CH9)
  if (ch9 > 1700 && ch9 < 2200) {
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  } else if (ch9 < 1300 && ch9 > 500) {
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  } else {
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
  }


  delay(20);
}
