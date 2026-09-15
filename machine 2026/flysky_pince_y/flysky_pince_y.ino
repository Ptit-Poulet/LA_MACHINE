#include <ESP32Servo.h>

const int SERVO_PIN = 23;          
const int RC_INPUT_PIN = 4;       
const int LIMIT_SWITCH_MIN_PIN = 14; 
const int LIMIT_SWITCH_MAX_PIN = 13; 

Servo myServo;

// --- ADJUST THIS IF IT CREEPS ---
const int STOP_SPEED = 95; // If it creeps right, try 88. If left, try 92.
// --------------------------------

const int FULL_RIGHT = 120; //70
const int FULL_LEFT = 70;//70
const int THRESHOLD_LOW = 1300;
const int THRESHOLD_HIGH = 1700;

volatile unsigned long rcPulseWidth = 1500;
volatile unsigned long rcPulseStart = 0;

void IRAM_ATTR rcInterrupt() {
  if (digitalRead(RC_INPUT_PIN) == HIGH) {
    rcPulseStart = micros();
  } else {
    unsigned long pulseWidth = micros() - rcPulseStart;
    if (pulseWidth >= 800 && pulseWidth <= 2200) {
      rcPulseWidth = pulseWidth;
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Use INTERNAL PULLUP: Switch MUST connect Pin to GND
  pinMode(LIMIT_SWITCH_MIN_PIN, INPUT_PULLDOWN);
  pinMode(LIMIT_SWITCH_MAX_PIN, INPUT_PULLDOWN);
  pinMode(RC_INPUT_PIN, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(RC_INPUT_PIN), rcInterrupt, CHANGE);
  
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(STOP_SPEED);
}

void loop() {
  // Read Switches: LOW means the switch is closed to Ground
  bool minLimitPressed = (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW);
  bool maxLimitPressed = (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW);
  
  int command = STOP_SPEED;

  // Determine intended direction
  if (rcPulseWidth < THRESHOLD_LOW) {
    command = FULL_LEFT;
  } else if (rcPulseWidth > THRESHOLD_HIGH) {
    command = FULL_RIGHT;
  }

  // --- HARD LIMIT LOGIC ---
  // If we want to go LEFT (speed < 90) but MIN limit is hit -> STOP
  if (command < STOP_SPEED && minLimitPressed) {
    command = STOP_SPEED;
  }
  
  // If we want to go RIGHT (speed > 90) but MAX limit is hit -> STOP
  if (command > STOP_SPEED && maxLimitPressed) {
    command = STOP_SPEED;
  }

  myServo.write(command);

  // DEBUG CONSOLE: Use this to test your switches!
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 100) {
    Serial.print("SW_MIN: "); Serial.print(minLimitPressed ? "[LOCKED]" : "[ OPEN ]");
    Serial.print(" | SW_MAX: "); Serial.print(maxLimitPressed ? "[LOCKED]" : "[ OPEN ]");
    Serial.print(" | Servo Write: "); Serial.println(command);
    lastPrint = millis();
  }
}
