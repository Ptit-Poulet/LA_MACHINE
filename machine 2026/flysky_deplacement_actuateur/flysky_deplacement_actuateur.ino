
#include <ESP32Servo.h>

// Receiver Inputs
#define CH1_PIN 32
#define CH2_PIN 35  // Forward/Backward
#define CH4_PIN 34  // Left/Right

#define in1 16
#define in2 17


// Servo Signal Pins
#define L_SERVO_PIN_f 25
#define L_SERVO_PIN_b 33 
#define R_SERVO_PIN_f 27
#define R_SERVO_PIN_b 26

Servo leftServof;
Servo rightServof;

Servo leftServob;
Servo rightServob;

float speedMultiplier = 1.0; 

int applyDeadband(int value, int threshold = 10) {
  return (abs(value) < threshold) ? 0 : value;
}

void setup() {
  Serial.begin(115200);

  // Set receiver pins
  pinMode(CH1_PIN, INPUT);
  pinMode(CH2_PIN, INPUT);
  pinMode(CH4_PIN, INPUT);

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Attach servos with standard pulse widths (1000us to 2000us)
  leftServof.setPeriodHertz(50);    // Standard 50Hz servo frequency
  leftServof.attach(L_SERVO_PIN_f, 1000, 2000);
  
  rightServof.setPeriodHertz(50);
  rightServof.attach(R_SERVO_PIN_f, 1000, 2000);

  leftServob.setPeriodHertz(50);    // Standard 50Hz servo frequency
  leftServob.attach(L_SERVO_PIN_b, 1000, 2000);
  
  rightServob.setPeriodHertz(50);
  rightServob.attach(R_SERVO_PIN_b, 1000, 2000);

  stopMotors();

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void loop() {
  // Read RC Receiver Pulses
  int ch1 = pulseIn(CH1_PIN, HIGH, 25000);
  int ch2 = pulseIn(CH2_PIN, HIGH, 25000);
  int ch4 = pulseIn(CH4_PIN, HIGH, 25000);

  // Failsafe: Stop if signal is lost
  if (ch2 < 500 || ch2 > 2200 || ch4 < 500 || ch4 > 2200) {
    stopMotors();
    return;
  }

  // Map joystick to speed values (-90 to 90 for servo offset from center)
  // 0 is stop, 90 is full forward, -90 is full backward
  int x = map(ch2, 1000, 2000, -90, 90) * speedMultiplier;
  int y = map(ch4, 1000, 2000, -90, 90) * speedMultiplier;

  x = applyDeadband(x);
  y = applyDeadband(y);

  // Differential Drive Mixing
  int leftSpeed = y + x;
  int rightSpeed = y - x;

  // Constrain to servo limits
  leftSpeed = constrain(leftSpeed, -90, 90);
  rightSpeed = constrain(rightSpeed, -90, 90);

  // Write to Servos (90 is the midpoint/stop for continuous rotation servos)
  // If using standard servos, this translates to 0-180 degrees.
  leftServof.write(90 + leftSpeed);
  rightServof.write(90 - rightSpeed); // Note: Right motor often needs inversion

  leftServob.write(90 + leftSpeed);
  rightServob.write(90 - rightSpeed); // Note: Right motor often needs inversion

  //actuateur
  if (ch1 > 1700 && ch1 < 2200){
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    //Serial.println("oui");
   
  }

  else if (ch1 > 500 && ch1 < 1400){
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    //Serial.println("non");
 
  }
  else{
    Serial.println(ch1);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    
  }

  delay(20); // Small delay for servo stability
}

void stopMotors() {
  leftServof.write(90);
  rightServof.write(90);

  leftServob.write(90);
  rightServob.write(90);
}
    
