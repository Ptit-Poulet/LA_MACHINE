#include <ESP32Servo.h>

// --- PIN DEFINITIONS ---
// Receiver Inputs
#define CH1_PIN 32
#define CH2_PIN 35      // Forward/Backward
#define CH3_PIN 39      // Joystick Second Actuator
#define CH4_PIN 34      // Left/Right
#define CH9_PIN 4       // 3-State Switch for Continuous Servo

// Motor Driver (Actuator) Pins
#define in1 16
#define in2 17

#define in3 18
#define in4 19

// Drive Servo Pins
#define L_SERVO_PIN_f 25
#define L_SERVO_PIN_b 33 
#define R_SERVO_PIN_f 27
#define R_SERVO_PIN_b 26

// Continuous Servo + Limit Switches
const int SERVOY_PIN = 23;          
const int LIMIT_SWITCH_MIN_PIN = 14;  
const int LIMIT_SWITCH_MAX_PIN = 13;  

// --- OBJECTS & CONSTANTS ---
Servo leftServof, rightServof, leftServob, rightServob;
Servo contServoY; // The 5th servo (Continuous)

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
  pinMode(CH3_PIN, INPUT);
  pinMode(CH4_PIN, INPUT);
  pinMode(CH9_PIN, INPUT);
  pinMode(LIMIT_SWITCH_MIN_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_MAX_PIN, INPUT_PULLUP);

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
  contServoY.attach(SERVOY_PIN, 500, 2400);
  
  stopMotors();
  contServoY.write(CONT_STOP);
}

void loop() {
  // 1. READ RC SIGNALS
  int ch1 = pulseIn(CH1_PIN, HIGH, 25000);
  int ch2 = pulseIn(CH2_PIN, HIGH, 25000);
  int ch3 = pulseIn(CH3_PIN, HIGH, 25000);
  int ch4 = pulseIn(CH4_PIN, HIGH, 25000);
  int ch9 = pulseIn(CH9_PIN, HIGH, 25000); // 3-state switch

  // 2. READ LIMIT SWITCHES (LOW = Pressed)
  bool minLimit = (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW);
  bool maxLimit = (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW);

  // Failsafe: Stop all if radio signal is lost
  if (ch2 < 500 || ch2 > 2500) {
    stopMotors();
    contServoY.write(CONT_STOP);
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
    return;
  }

  // 3. DRIVE SYSTEM (Differential Mixing)
  int x = map(ch2, 1000, 2000, -90, 90) * speedMultiplier;
  int y = map(ch4, 1000, 2000, -90, 90) * speedMultiplier;
  x = applyDeadband(x);
  y = applyDeadband(y);

  int leftSpeed = y + x;
  int rightSpeed = y - x;
  leftSpeed = constrain(leftSpeed, -90, 90);
  rightSpeed = constrain(rightSpeed, -90, 90);

  leftServof.write(90 + leftSpeed);
  rightServof.write(90 - rightSpeed);
  leftServob.write(90 + leftSpeed);
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

  // 5. DC ACTUATOR 2 (CH3)
  if (ch3 > 1700) {
    // Extension
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  } 
  else if (ch3 < 1300 && ch3 > 500) {
    // Rétraction
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  } 
  else {
    // Stop
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
  }


  // 6. CONTINUOUS SERVO WITH LIMITS (CH9)
  int contTarget = CONT_STOP;

  if (ch9 > 1700) {
    contTarget = CONT_RIGHT;
  } else if (ch9 < 1300 && ch9 > 500) {
    contTarget = CONT_LEFT;
  }

  // Safety Lock: Check limits against direction
  if (contTarget > CONT_STOP && maxLimit) {
    contTarget = CONT_STOP; // Block Right
  }
  if (contTarget < CONT_STOP && minLimit) {
    contTarget = CONT_STOP; // Block Left
  }

  contServoY.write(contTarget);

  // Debugging
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 500) {
    Serial.printf("CH9: %d | Limits: L[%d] R[%d] | Servo: %d\n", 
                  ch9, minLimit, maxLimit, contTarget);
    lastLog = millis();
  }

  delay(20);
}
