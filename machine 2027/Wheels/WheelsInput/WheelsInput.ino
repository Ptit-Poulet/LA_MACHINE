// 4-wheel mecanum robot, each wheel with its own speed loop
// Serial commands (115200 baud):
//  w=forward  s=backward  a=strafe left  d=strafe right
//  q=turn left  e=turn right  x=stop

struct Wheel {
  const char* name;
  uint8_t pwmPin, dirPin, fgPin; // Blue, White, Yellow
  bool invert;            // flip if this wheel spins the wrong way
  volatile uint32_t pulses;
  float target;           // signed RPM (+ = wheel rolls forward)
  float rpm;
  float integral;
  bool fwd;               // current direction
  bool reversing;
  unsigned long revStart;
};

Wheel wheels[4] = {
// name  pwm dir fg  
  {"FL", 18, 19, 34, false, 0, 0, 0, 0, true, false, 0},
  {"BL", 16, 17, 35, false, 0, 0, 0, 0, true, false, 0},
  {"FR", 25, 26, 32, true,  0, 0, 0, 0, true, false, 0},
  {"_BR", 27, 13, 33, true,  0, 0, 0, 0, true, false, 0},
};
enum { FL, BL, FR, _BR };

const int   PWM_FREQ   = 20000;
const int   PWM_RES    = 8;
const float PULSES_REV = 9.0 * 30.0;   // 270 per output rev
const float MAX_RPM    = 150;          // keep below the 200 no-load max
const float KP = 0.4, KI = 0.8, FF = 1.2;

void IRAM_ATTR onFG(void* arg) { ((Wheel*)arg)->pulses++; }

void setDuty(Wheel& w, int duty) {
  duty = constrain(duty, 0, 255);
  ledcWrite(w.pwmPin, 255 - duty);     // inverted: LOW = ON
}

void writeDir(Wheel& w, bool fwd) {
  // HIGH = CCW, LOW = CW. 'invert' accounts for mirrored mounting.
  bool level = fwd ? w.invert : !w.invert;
  digitalWrite(w.dirPin, level ? HIGH : LOW);
}

// vx: forward(+), vy: strafe right(+), wz: turn right/clockwise(+)
// each in range -1..1
void drive(float vx, float vy, float wz) {
  float s[4];
  s[FL] = vx + vy + wz;
  s[FR] = vx - vy - wz;
  s[BL] = vx - vy + wz;
  s[_BR] = vx + vy - wz;

  float m = 1;
  for (int i = 0; i < 4; i++) if (fabs(s[i]) > m) m = fabs(s[i]);
  for (int i = 0; i < 4; i++) wheels[i].target = (s[i] / m) * MAX_RPM;
}

void setup() {
  Serial.begin(115200);
  for (auto& w : wheels) {
    pinMode(w.dirPin, OUTPUT);
    pinMode(w.fgPin, INPUT);
    ledcAttach(w.pwmPin, PWM_FREQ, PWM_RES);
    setDuty(w, 0);
    writeDir(w, true);
    attachInterruptArg(w.fgPin, onFG, &w, RISING);
  }
  Serial.println("w/s/a/d/q/e = move, x = stop");
}

unsigned long lastMs = 0;

void loop() {
  // --- commands ---
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'w': drive( 1,  0,  0); break;   // forward
      case 's': drive(-1,  0,  0); break;   // backward
      case 'a': drive( 0, -1,  0); break;   // strafe left
      case 'd': drive( 0,  1,  0); break;   // strafe right
      case 'q': drive( 0,  0, -1); break;   // turn left
      case 'e': drive( 0,  0,  1); break;   // turn right
      case 'x': drive( 0,  0,  0); break;   // stop
    }
  }

  // --- speed control every 100 ms ---
  unsigned long now = millis();
  if (now - lastMs < 100) return;
  float dt = (now - lastMs) / 1000.0;
  lastMs = now;

  for (auto& w : wheels) {
    noInterrupts();
    uint32_t p = w.pulses; w.pulses = 0;
    interrupts();
    w.rpm = (p / dt) * 60.0 / PULSES_REV;

    float mag = fabs(w.target);
    bool wantFwd = (w.target >= 0);

    if (mag < 1) {                       // stopped
      setDuty(w, 0);
      w.integral = 0;
      continue;
    }

    if (wantFwd != w.fwd) {              // reverse: stop first
      if (!w.reversing) { w.reversing = true; w.revStart = now; }
      setDuty(w, 0);
      w.integral = 0;
      if (w.rpm < 15 || now - w.revStart > 600) {
        w.fwd = wantFwd;
        writeDir(w, w.fwd);
        w.reversing = false;
      }
      continue;
    }

    float err = mag - w.rpm;
    w.integral += err * dt;
    w.integral = constrain(w.integral, -150, 150);
    float out = KP * err + KI * w.integral + mag * FF;
    setDuty(w, (int)out);
  }

  Serial.printf("FL %.0f BL %.0f FR %.0f _BR %.0f RPM\n",
                wheels[FL].rpm, wheels[BL].rpm, wheels[FR].rpm, wheels[_BR].rpm);
}