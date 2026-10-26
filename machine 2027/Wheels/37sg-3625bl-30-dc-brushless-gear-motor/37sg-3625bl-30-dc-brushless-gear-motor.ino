const int PWM_PIN = 18;   // blue  (inverted: LOW = ON)
const int DIR_PIN = 19;   // white (HIGH = CCW, LOW = CW)
const int FG_PIN  = 21;   // yellow

const int   PWM_FREQ   = 20000;
const int   PWM_RES    = 8;
const float PULSES_REV = 9.0 * 30.0;   // 270 per output rev

volatile uint32_t pulses = 0;
void IRAM_ATTR onFG() { pulses++; }

float targetRPM = 100;    // change this
float kp = 0.4, ki = 0.8;
float integral = 0;
unsigned long lastMs = 0;

void setSpeedPWM(int duty) {              // duty 0..255 = motor effort
  duty = constrain(duty, 0, 255);
  ledcWrite(PWM_PIN, 255 - duty);         // inverted
}

void setDirection(bool ccw) {
  setSpeedPWM(0);
  delay(300);                             // let it stop first
  digitalWrite(DIR_PIN, ccw ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(FG_PIN, INPUT);
  attachInterrupt(FG_PIN, onFG, RISING);
  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);
  setSpeedPWM(0);
  setDirection(true);
  lastMs = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastMs >= 100) {
    float dt = (now - lastMs) / 1000.0;
    lastMs = now;

    noInterrupts();
    uint32_t p = pulses; pulses = 0;
    interrupts();

    float rpm = (p / dt) * 60.0 / PULSES_REV;

    float err = targetRPM - rpm;
    integral += err * dt;
    integral = constrain(integral, -150, 150);   // anti-windup

    float out = kp * err + ki * integral + targetRPM * 1.0; // feedforward
    setSpeedPWM((int)out);

    Serial.printf("target %.0f  actual %.1f RPM\n", targetRPM, rpm);
  }
}