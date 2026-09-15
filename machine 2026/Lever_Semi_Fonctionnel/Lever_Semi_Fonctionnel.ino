/*
 * ESP32 MG90S Servo Control with Spectrum DX8e and Limit Switches
 * DEBUG VERSION - Servo on D21
 */

#include <ESP32Servo.h>

// Pin definitions - SERVO ON D21
const int SERVO_PIN = 21;         // MG90S control pin (D21) - CORRECTED
const int RC_INPUT_PIN = 25;      // Spectrum receiver channel input
const int LIMIT_SWITCH_MIN_PIN = 35;  // Limit switch at 0° position (D35)
const int LIMIT_SWITCH_MAX_PIN = 34;  // Limit switch at 180° position (D34)

// Servo configuration
Servo myServo;
const int MG90S_MIN_PULSE = 500;
const int MG90S_MAX_PULSE = 2400;
const int SERVO_FREQ = 50;

// Servo limits
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;

// RC signal parameters
const int RC_MIN_PULSE = 1000;
const int RC_MAX_PULSE = 2000;
const int RC_CENTER_PULSE = 1500;

// Variables
volatile unsigned long rcPulseStart = 0;
volatile unsigned long rcPulseWidth = 1500;
int currentAngle = 90;
bool emergencyStop = false;

void IRAM_ATTR rcInterrupt();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("SERVO CONTROL DEBUG - SERVO ON GPIO 21");
  Serial.println("========================================\n");
  
  // Setup limit switches with internal pullup
  pinMode(LIMIT_SWITCH_MIN_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_MAX_PIN, INPUT_PULLUP);
  
  // Setup RC input
  pinMode(RC_INPUT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RC_INPUT_PIN), rcInterrupt, CHANGE);
  
  // Setup servo on GPIO 21
  myServo.setPeriodHertz(SERVO_FREQ);
  myServo.attach(SERVO_PIN, MG90S_MIN_PULSE, MG90S_MAX_PULSE);
  myServo.write(90);
  currentAngle = 90;
  
  Serial.println("PINS CONFIGURED:");
  Serial.println("- Servo: GPIO 21 (D21)");
  Serial.println("- RC Input: GPIO 25");
  Serial.println("- Limit Min: GPIO 35 (D35)");
  Serial.println("- Limit Max: GPIO 34 (D34)");
  Serial.println("\nTESTING LIMIT SWITCHES...");
  Serial.println("Press each switch and verify values change:");
  Serial.println("========================================\n");
  
  // Initial limit switch test
  bool minStart = digitalRead(LIMIT_SWITCH_MIN_PIN);
  bool maxStart = digitalRead(LIMIT_SWITCH_MAX_PIN);
  
  Serial.print("STARTUP - Min: ");
  Serial.print(minStart);
  Serial.print(" | Max: ");
  Serial.println(maxStart);
  Serial.println("(Should be 1 when open, 0 when pressed)\n");
}

void loop() {
  // Read limit switches - SIMPLE DIRECT READING
  bool minValue = digitalRead(LIMIT_SWITCH_MIN_PIN);
  bool maxValue = digitalRead(LIMIT_SWITCH_MAX_PIN);
  
  // With INPUT_PULLUP: HIGH=not pressed, LOW=pressed
  bool minPressed = (minValue == LOW);
  bool maxPressed = (maxValue == LOW);
  
  // Map RC to servo angle
  int targetAngle = map(rcPulseWidth, RC_MIN_PULSE, RC_MAX_PULSE, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  targetAngle = constrain(targetAngle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  
  // EMERGENCY STOP if limit switches pressed
  if (minPressed || maxPressed) {
    if (!emergencyStop) {
      emergencyStop = true;
      myServo.detach(); // COMPLETELY STOP SERVO
      Serial.println("!!! EMERGENCY STOP - LIMIT SWITCH PRESSED !!!");
      if (minPressed) Serial.println("*** MIN LIMIT ACTIVATED ***");
      if (maxPressed) Serial.println("*** MAX LIMIT ACTIVATED ***");
    }
    
    // Print continuous warning
    static unsigned long lastWarn = 0;
    if (millis() - lastWarn > 1000) {
      lastWarn = millis();
      Serial.println(">>> SERVO STOPPED - Clear limit switch to resume <<<");
    }
    
    delay(100);
    return; // Skip all other processing
  }
  else {
    // No limits pressed - resume normal operation
    if (emergencyStop) {
      emergencyStop = false;
      myServo.attach(SERVO_PIN, MG90S_MIN_PULSE, MG90S_MAX_PULSE);
      Serial.println(">>> LIMITS CLEARED - NORMAL OPERATION RESUMED <<<");
    }
  }
  
  // Move servo if no limits pressed
  if (targetAngle != currentAngle) {
    currentAngle = targetAngle;
    myServo.write(currentAngle);
  }
  
  // Debug output
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    
    Serial.print("Limits: [");
    Serial.print(minPressed ? "MIN!" : "---");
    Serial.print("][");
    Serial.print(maxPressed ? "MAX!" : "---");
    Serial.print("]");
    
    Serial.print(" | RAW: [");
    Serial.print(minValue);
    Serial.print("][");
    Serial.print(maxValue);
    Serial.print("]");
    
    Serial.print(" | Servo: ");
    Serial.print(currentAngle);
    Serial.print("°");
    
    Serial.print(" | RC: ");
    Serial.print(rcPulseWidth);
    Serial.println("µs");
  }
  
  delay(50);
}

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

// Manual test function
void testServoMovement() {
  Serial.println("\n=== MANUAL SERVO TEST ===");
  
  detachInterrupt(digitalPinToInterrupt(RC_INPUT_PIN));
  
  Serial.println("Moving to 0°...");
  for (int i = currentAngle; i >= 0; i--) {
    if (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW) {
      Serial.println("MIN LIMIT HIT - STOPPED");
      break;
    }
    myServo.write(i);
    delay(50);
  }
  delay(1000);
  
  Serial.println("Moving to 180°...");
  for (int i = currentAngle; i <= 180; i++) {
    if (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW) {
      Serial.println("MAX LIMIT HIT - STOPPED");
      break;
    }
    myServo.write(i);
    delay(50);
  }
  delay(1000);
  
  Serial.println("Returning to 90°...");
  myServo.write(90);
  currentAngle = 90;
  
  attachInterrupt(digitalPinToInterrupt(RC_INPUT_PIN), rcInterrupt, CHANGE);
  Serial.println("Test complete");
}