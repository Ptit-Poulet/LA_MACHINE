#include <ESP32Servo.h>

// Output pins
#define STEER_PIN   18
#define THROTTLE_PIN 19

Servo steer;
Servo throttle;

// Convert -100 → +100 to 1000–2000 µs servo pulse
int mapToPulse(int val) {
  val = constrain(val, -100, 100);
  return map(val, -100, 100, 1000, 2000);
}

void setup() {
  Serial.begin(115200);

  steer.attach(STEER_PIN, 1000, 2000);
  throttle.attach(THROTTLE_PIN, 1000, 2000);

  // Neutral startup
  steer.writeMicroseconds(1500);
  throttle.writeMicroseconds(1500);

  delay(1000);
  Serial.println("Scorpion Mini ready!");
}

void loop() {
  // Example demo movement

  // Forward
  throttle.writeMicroseconds(mapToPulse(60));
  steer.writeMicroseconds(mapToPulse(0)); 
  delay(2000);

  // Reverse
  throttle.writeMicroseconds(mapToPulse(-60));
  delay(2000);

  // Turn Right
  throttle.writeMicroseconds(mapToPulse(40));
  steer.writeMicroseconds(mapToPulse(50));
  delay(2000);

  // Turn Left
  steer.writeMicroseconds(mapToPulse(-50));
  delay(2000);

  // Stop
  throttle.writeMicroseconds(mapToPulse(0));
  steer.writeMicroseconds(mapToPulse(0));
  delay(2000);
}
