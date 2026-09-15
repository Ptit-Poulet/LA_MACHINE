#include <Bluepad32.h>
#include <ESP32Servo.h>

// Scorpion Mini signal pins
#define STEER_PIN    18
#define THROTTLE_PIN 19

Servo steerServo;
Servo throttleServo;

ControllerPtr myController;

// Convert -100 to +100 into 1000–2000 µs
int mapToPulse(int v) {
  v = constrain(v, -100, 100);
  return map(v, -100, 100, 1000, 2000);
}

void setup() {
  Serial.begin(115200);

  // Attach servo outputs
  steerServo.attach(STEER_PIN, 1000, 2000);
  throttleServo.attach(THROTTLE_PIN, 1000, 2000);

  // Neutral on startup
  steerServo.writeMicroseconds(1500);
  throttleServo.writeMicroseconds(1500);

  // Init Bluepad32
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();  // Optional
  Serial.println("Ready. Put PS5 controller in pairing mode (Share + PS).");
}

void loop() {
  BP32.update();

  if (myController && myController->isConnected()) {
    int lx = myController->axisX();     // steering
    int ly = myController->axisY();     // forward/back

    // Convert PS5 stick (-511 to +511) → -100 to +100
    int steer = map(lx, -511, 511, -100, 100);
    int throttle = map(ly, -511, 511, 100, -100); // Y inverted

    // Deadzone
    if (abs(steer) < 8) steer = 0;
    if (abs(throttle) < 8) throttle = 0;

    // Send to Scorpion Mini
    steerServo.writeMicroseconds(mapToPulse(steer));
    throttleServo.writeMicroseconds(mapToPulse(throttle));

  } else {
    // Neutral if no controller
    steerServo.writeMicroseconds(1500);
    throttleServo.writeMicroseconds(1500);
  }

  delay(10);
}

// -------------------- Controller Callbacks --------------------

void onConnectedController(ControllerPtr ctl) {
  Serial.println("PS5 controller connected!");
  myController = ctl;
}

void onDisconnectedController(ControllerPtr ctl) {
  Serial.println("PS5 controller disconnected!");
  steerServo.writeMicroseconds(1500);
  throttleServo.writeMicroseconds(1500);
  if (ctl == myController) myController = nullptr;
}
