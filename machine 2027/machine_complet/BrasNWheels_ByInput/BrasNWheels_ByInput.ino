// ============================================================
//  ROBOT : 4 roues mecanum + bras a 3 moteurs pas-a-pas
// ------------------------------------------------------------
//  Port serie 115200
//
//    1  = mode BRAS
//    2  = mode ROUES
//    x  = TOUT ARRETER (dans tous les modes)
//
//  Mode ROUES :
//    w = avancer          s = reculer
//    a = decaler gauche   d = decaler droite
//    q = tourner gauche   e = tourner droite
//
//  Mode BRAS :
//    200 -100 50 = bouge M1, M2, M3 de ces nombres de pas
//    p = afficher les positions
//    z = remettre les positions a zero
//
//  IMPORTANT : ne mets JAMAIS de delay() dans loop(), sinon le
//  bras et les roues deviennent saccades.
// ============================================================

// Types utilises par plusieurs fonctions : doivent etre declares en premier
enum Mode { MODE_NONE, MODE_ARM, MODE_WHEELS };
Mode mode = MODE_NONE;

// ============================================================
//  1) ROUES (moteurs BLDC avec driver integre)
// ============================================================

struct Wheel {
  const char* name;
  uint8_t pwmPin, dirPin, fgPin;  // Bleu, Blanc, Jaune
  bool invert;                    // inverse si la roue tourne a l'envers
  volatile uint32_t pulses;       // impulsions FG comptees
  float target;                   // vitesse voulue en RPM (+ = avant)
  float rpm;                      // vitesse mesuree
  float integral;                 // terme integral du regulateur
  bool fwd;                       // sens actuel
  bool reversing;                 // en train de s'arreter pour changer de sens
  unsigned long revStart;
};

Wheel wheels[4] = {
// nom   pwm dir fg  invert
  {"FL",  18, 19, 34, false, 0, 0, 0, 0, true, false, 0},
  {"BL",  16, 17, 35, false, 0, 0, 0, 0, true, false, 0},
  {"FR",  25, 26, 36, true,  0, 0, 0, 0, true, false, 0},
  {"_BR", 27, 13, 39, true,  0, 0, 0, 0, true, false, 0},
};
//36 = VP et 39 VN
enum { FL, BL, FR, _BR };

const int   PWM_FREQ   = 20000;        // 20 kHz (hors audible)
const int   PWM_RES    = 8;            // 0..255
const float PULSES_REV = 9.0 * 30.0;   // 270 impulsions par tour de roue
const float MAX_RPM    = 150;          // reste sous les 200 RPM a vide
const float KP = 0.4, KI = 0.8, FF = 1.2;   // reglages du regulateur

// Appelee a chaque impulsion FG : on compte seulement
void IRAM_ATTR onFG(void* arg) { ((Wheel*)arg)->pulses++; }

// Puissance 0..255. Le driver est INVERSE : signal BAS = moteur ON
void setDuty(Wheel& w, int duty) {
  duty = constrain(duty, 0, 255);
  ledcWrite(w.pwmPin, 255 - duty);
}

// Sens de rotation (HAUT = antihoraire, BAS = horaire)
void writeDir(Wheel& w, bool fwd) {
  bool level = fwd ? w.invert : !w.invert;
  digitalWrite(w.dirPin, level ? HIGH : LOW);
}

// vx : avant (+) | vy : droite (+) | wz : tourner a droite (+), de -1 a 1
void drive(float vx, float vy, float wz) {
  float s[4];
  s[FL]  = vx + vy + wz;
  s[FR]  = vx - vy - wz;
  s[BL]  = vx - vy + wz;
  s[_BR] = vx + vy - wz;

  // Si une valeur depasse 1, on reduit tout pour garder les proportions
  float m = 1;
  for (int i = 0; i < 4; i++) if (fabs(s[i]) > m) m = fabs(s[i]);
  for (int i = 0; i < 4; i++) wheels[i].target = (s[i] / m) * MAX_RPM;
}

// Regulation de vitesse, appelee en boucle (agit toutes les 100 ms)
unsigned long lastWheelMs = 0;
unsigned long lastPrintMs = 0;

void updateWheels() {
  unsigned long now = millis();
  if (now - lastWheelMs < 100) return;          // pas encore le moment
  float dt = (now - lastWheelMs) / 1000.0;
  lastWheelMs = now;

  bool anyMoving = false;

  for (auto& w : wheels) {
    // 1) Mesurer la vitesse
    noInterrupts();
    uint32_t p = w.pulses; w.pulses = 0;
    interrupts();
    w.rpm = (p / dt) * 60.0 / PULSES_REV;

    float mag = fabs(w.target);                 // vitesse voulue (toujours positive)
    bool wantFwd = (w.target >= 0);

    // 2) Roue a l'arret
    if (mag < 1) {
      setDuty(w, 0);
      w.integral = 0;
      continue;
    }
    anyMoving = true;

    // 3) Changement de sens : on s'arrete d'abord
    if (wantFwd != w.fwd) {
      if (!w.reversing) { w.reversing = true; w.revStart = now; }
      setDuty(w, 0);
      w.integral = 0;
      if (w.rpm < 15 || now - w.revStart > 600) {   // quasi arretee -> on inverse
        w.fwd = wantFwd;
        writeDir(w, w.fwd);
        w.reversing = false;
      }
      continue;
    }

    // 4) Regulateur PI + terme de base (feedforward)
    float err = mag - w.rpm;
    w.integral += err * dt;
    w.integral = constrain(w.integral, -150, 150);   // anti-emballement
    float out = KP * err + KI * w.integral + mag * FF;
    setDuty(w, (int)out);
  }

  // Affiche les vitesses 2 fois par seconde, seulement si ca roule
  if (anyMoving && now - lastPrintMs >= 500) {
    lastPrintMs = now;
    Serial.printf("RPM  FL %.0f | BL %.0f | FR %.0f | BR %.0f\n",
                  wheels[FL].rpm, wheels[BL].rpm, wheels[FR].rpm, wheels[_BR].rpm);
  }
}


// ============================================================
//  2) BRAS (3 moteurs pas-a-pas)
//     M1 = NEMA17 (driver STEP/DIR), M2 et M3 = NEMA23 (TB6600)
//     TB6600 : SW1 ON, SW2 ON, SW3 OFF, SW4 OFF, SW5 OFF, SW6 ON
// ============================================================

// Moteur 1 (NEMA17)
const int STPM1 = 23;
const int DIRM1 = 22;

// Moteur 2 (NEMA23)
const int PULM2 = 32;
const int DIRM2 = 33;
const int ENAM2 = 14;

// Moteur 3 (NEMA23)
const int PULM3 = 21;
const int DIRM3 = 4;
const int ENAM3 = 5;

// Tableaux pour traiter les 3 moteurs avec une boucle (index 0,1,2 = M1,M2,M3)
const int STEP_PIN[3] = {STPM1, PULM2, PULM3};
const int DIR_PIN[3]  = {DIRM1, DIRM2, DIRM3};

const int  STEP_US      = 1500;  // demi-periode en microsecondes (500 = plus rapide)
const bool POSITIVE_DIR = LOW;   // mets HIGH si le sens est inverse

long pos[3] = {0, 0, 0};         // positions actuelles en pas

// Etat du deplacement en cours (le bras avance un petit peu a chaque loop())
struct ArmMove {
  bool active = false;           // un deplacement est en cours ?
  long total = 0;                // nombre de "temps" total (le plus grand nb de pas)
  long done = 0;                 // temps deja effectues
  long n[3];                     // nb de pas de chaque moteur
  long err[3];                   // accumulateur (repartit les pas de facon reguliere)
  bool positive[3];              // sens de chaque moteur
  bool stepping[3];              // ce moteur fait un pas pendant ce temps ?
  bool pulseHigh = false;        // impulsion en cours (signal HAUT) ?
  unsigned long nextUs = 0;      // prochain moment ou agir
};
ArmMove arm;

void printPositions() {
  Serial.printf("Position M1 : %ld | M2 : %ld | M3 : %ld\n", pos[0], pos[1], pos[2]);
}

// Prepare un deplacement : regle les sens, puis updateArm() fait le reste
void startArm(long s1, long s2, long s3) {
  if (arm.active) {
    Serial.println("Bras deja en mouvement (x pour arreter).");
    return;
  }
  long s[3] = {s1, s2, s3};
  arm.total = 0;

  for (int m = 0; m < 3; m++) {
    arm.n[m] = labs(s[m]);
    arm.err[m] = 0;
    arm.positive[m] = s[m] > 0;
    digitalWrite(DIR_PIN[m], arm.positive[m] ? POSITIVE_DIR : !POSITIVE_DIR);
    if (arm.n[m] > arm.total) arm.total = arm.n[m];
  }
  if (arm.total == 0) return;              // rien a faire

  arm.done = 0;
  arm.pulseHigh = false;
  arm.active = true;
  arm.nextUs = micros() + 50;              // petit delai pour que DIR soit stable
  Serial.printf("Deplacement M1 : %ld | M2 : %ld | M3 : %ld\n", s1, s2, s3);
}

// Arret immediat du bras
void stopArm() {
  for (int m = 0; m < 3; m++) digitalWrite(STEP_PIN[m], LOW);
  arm.active = false;
}

// Appelee en boucle : avance le bras d'une demi-impulsion si c'est l'heure
void updateArm() {
  if (!arm.active) return;

  unsigned long now = micros();
  if ((long)(now - arm.nextUs) < 0) return;   // pas encore l'heure

  if (!arm.pulseHigh) {
    // --- Debut d'un temps : decider quels moteurs font un pas ---
    if (arm.done >= arm.total) {              // tout est fini
      arm.active = false;
      printPositions();
      return;
    }
    for (int m = 0; m < 3; m++) {
      arm.err[m] += arm.n[m];
      if (arm.err[m] >= arm.total) {          // ce moteur doit faire un pas
        arm.err[m] -= arm.total;
        arm.stepping[m] = true;
        digitalWrite(STEP_PIN[m], HIGH);      // front montant = 1 pas
      } else {
        arm.stepping[m] = false;
      }
    }
    arm.pulseHigh = true;
  } else {
    // --- Fin du temps : remettre les signaux a BAS et compter ---
    for (int m = 0; m < 3; m++) {
      digitalWrite(STEP_PIN[m], LOW);
      if (arm.stepping[m]) pos[m] += arm.positive[m] ? 1 : -1;
    }
    arm.pulseHigh = false;
    arm.done++;
  }
  arm.nextUs = now + STEP_US;
}


// ============================================================
//  3) COMMANDES SERIE (lecture sans jamais attendre)
//     Mode 1 = BRAS, Mode 2 = ROUES
// ============================================================

String inputBuf = "";   // texte recu en cours de saisie

void printHelp() {
  if (mode == MODE_ARM) {
    Serial.println("MODE BRAS : tape 3 nombres  ex: 200 -100 50  (pas M1 M2 M3)");
    Serial.println("            p = positions | z = zero | x = stop");
  } else if (mode == MODE_WHEELS) {
    Serial.println("MODE ROUES : w=avant s=arriere a=gauche d=droite q/e=tourner | x = stop");
  } else {
    Serial.println("Choisis un mode : 1 = BRAS, 2 = ROUES");
  }
}

// Change de mode : arrete tout avant
void setMode(Mode m) {
  drive(0, 0, 0);     // roues a l'arret
  stopArm();          // bras a l'arret
  mode = m;
  printHelp();
}

// --- Commandes du mode BRAS ---
void handleArm(const String& line) {
  if (line == "p") { printPositions(); return; }
  if (line == "z") {
    pos[0] = pos[1] = pos[2] = 0;
    Serial.println("Positions remises a 0");
    return;
  }
  long a, b, c;
  // accepte "200 -100 50" ou "m 200 -100 50"
  if (sscanf(line.c_str(), "%ld %ld %ld", &a, &b, &c) == 3 ||
      sscanf(line.c_str(), "m %ld %ld %ld", &a, &b, &c) == 3) {
    startArm(a, b, c);
  } else {
    Serial.println("Format : pasM1 pasM2 pasM3   ex: 200 -100 50");
  }
}

// --- Commandes du mode ROUES ---
void handleWheels(const String& line) {
  if (line.length() != 1) { Serial.println("Commande inconnue"); return; }
  switch (line[0]) {
    case 'w': drive( 1,  0,  0); break;   // avancer
    case 's': drive(-1,  0,  0); break;   // reculer
    case 'a': drive( 0, -1,  0); break;   // decaler a gauche
    case 'd': drive( 0,  1,  0); break;   // decaler a droite
    case 'q': drive( 0,  0, -1); break;   // tourner a gauche
    case 'e': drive( 0,  0,  1); break;   // tourner a droite
    default:  Serial.println("Commande inconnue");
  }
}

// Traite une ligne complete
void handleLine(String line) {
  line.trim();
  line.toLowerCase();
  if (line.length() == 0) return;

  // Commandes valables dans tous les modes
  if (line == "1") { setMode(MODE_ARM);    return; }
  if (line == "2") { setMode(MODE_WHEELS); return; }
  if (line == "x") {
    drive(0, 0, 0);
    stopArm();
    Serial.println("Arret !");
    return;
  }

  // Commandes propres a chaque mode
  if      (mode == MODE_ARM)    handleArm(line);
  else if (mode == MODE_WHEELS) handleWheels(line);
  else                          printHelp();
}

// Lit les caracteres disponibles, et traite la ligne quand Entree est recu
void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuf.length() > 0) { handleLine(inputBuf); inputBuf = ""; }
    } else {
      inputBuf += c;
    }
  }
}


// ============================================================
//  4) SETUP ET LOOP
// ============================================================

void setup() {
  Serial.begin(115200);

  // --- Roues ---
  for (auto& w : wheels) {
    pinMode(w.dirPin, OUTPUT);
    pinMode(w.fgPin, INPUT);
    ledcAttach(w.pwmPin, PWM_FREQ, PWM_RES);
    setDuty(w, 0);                              // moteur arrete
    writeDir(w, true);
    attachInterruptArg(w.fgPin, onFG, &w, RISING);
  }

  // --- Bras ---
  for (int m = 0; m < 3; m++) {
    pinMode(STEP_PIN[m], OUTPUT);
    pinMode(DIR_PIN[m], OUTPUT);
    digitalWrite(STEP_PIN[m], LOW);
    digitalWrite(DIR_PIN[m], POSITIVE_DIR);
  }
  pinMode(ENAM2, OUTPUT); digitalWrite(ENAM2, LOW);
  pinMode(ENAM3, OUTPUT); digitalWrite(ENAM3, LOW);

  Serial.println("Pret.");
  printHelp();
}

// loop() ne bloque jamais : chaque morceau fait un petit travail et rend la main
void loop() {
  readSerial();     // 1) lire les commandes
  updateArm();      // 2) faire avancer le bras d'un petit pas
  updateWheels();   // 3) regler les roues (toutes les 100 ms)
}