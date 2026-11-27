// ============================================================
//  ROBOT MECANUM 4 ROUES PILOTÉ PAR MANETTE SPEKTRUM DX8e
//  Récepteur : AR8020T (sorties servo PWM)  |  Carte : ESP32
// ============================================================
//
//  COMMANDES :
//    Interrupteur A = 1   -> controlles roues ACTIVÉ 
//    Stick GAUCHE haut/bas       (CH1 THRO) -> avancer / reculer
//    Stick GAUCHE gauche/droite  (CH4 RUDD) -> déplacement latéral (strafe)
//    Stick DROIT  gauche/droite  (CH2 AILE) -> tourner gauche / droite
//
//  CÂBLAGE DU RÉCEPTEUR AR8020T :
//    Sortie 1 (THRO) signal -> GPIO 4 (D4)  de l'ESP32
//    Sortie 2 (AILE) signal -> GPIO 21 (D21) de l'ESP32
//    Sortie 4 (RUDD) signal -> GPIO 14 (D14) de l'ESP32
//    Sortie 5 (GEAR) signal -> GPIO 23 (D23) de l'ESP32  (interrupteur A)
//    Alimentation du récepteur : 5 V (port BATT/BIND ou une sortie)
//    GND du récepteur relié au GND de l'ESP32 (masse commune obligatoire)
//
//  RÉGLAGES DE LA DX8e :
//    - L'interrupteur A doit piloter la voie 5 (GEAR). Vérifie dans le
//      Moniteur de la manette que la barre de la voie 5 bouge avec A.
//      (Sinon, change IN_SWITCH / le branchement vers la voie qui bouge.)
//    - Failsafe : fais le "bind" avec l'interrupteur A à 0 et les sticks
//      au centre.
//
//  SÉCURITÉ : si un signal du récepteur disparaît pendant 500 ms
//  (manette éteinte, fil débranché), le robot s'arrête.
// ============================================================
 
 
// ------------------------------------------------------------
//  RÉGLAGES MANETTE
// ------------------------------------------------------------
 
// Broches de l'ESP32 reliées aux sorties du récepteur.
// L'ORDRE du tableau définit les numéros (index) utilisés plus bas.
const uint8_t PWM_PINS[4] = {4, 21, 23, 14};
 
// À quoi sert chaque entrée (index dans PWM_PINS) :
#define IN_FORWARD  0   // GPIO 4  <- sortie 1 (THRO) : avant/arrière
#define IN_TURN     1   // GPIO 21 <- sortie 2 (AILE) : rotation
#define IN_SWITCH   2   // GPIO 23 <- sortie 5 (GEAR) : interrupteur A
#define IN_STRAFE   3   // GPIO 14 <- sortie 4 (RUDD) : latéral
 
// Largeur d'impulsion Spektrum : ~1100 µs à ~1900 µs, centre 1500 µs.
const float STICK_HALF_RANGE = 400.0;
 
// Interrupteur A : au-dessus de ce seuil = position "1" = robot activé.
// Si c'est dans l'autre sens sur ta manette, mets SWITCH_ACTIVE_HIGH = false.
const uint16_t SWITCH_THRESHOLD_US = 1500;
const bool     SWITCH_ACTIVE_HIGH  = true;
 
// Zone morte : un stick ne revient jamais exactement à 1500 au repos.
// En dessous de ce seuil (8 % de la course), on considère "stick au centre".
const float DEADZONE = 0.08;
 
// Si un mouvement part dans le MAUVAIS sens, passe le réglage à true.
const bool INV_FORWARD = false;
const bool INV_STRAFE  = false;
const bool INV_TURN    = false;
 
// Délai sans impulsion valide au-delà duquel le robot s'arrête (ms).
const unsigned long FAILSAFE_MS = 500;
 
// Mettre true pour afficher les valeurs brutes des 4 entrées
// (utile pour vérifier le mapping, à remettre à false ensuite).
const bool DEBUG_INPUTS = true;
 
 
// ------------------------------------------------------------
//  RÉGLAGES MOTEURS
// ------------------------------------------------------------
 
const int   PWM_FREQ   = 20000;        // fréquence PWM (20 kHz, inaudible)
const int   PWM_RES    = 8;            // résolution 8 bits : duty de 0 à 255
const float PULSES_REV = 9.0 * 30.0;   // 270 impulsions par tour de roue
const float MAX_RPM    = 150;          // vitesse max visée
const float KP = 0.4, KI = 0.8, FF = 1.2; // gains du régulateur de vitesse
 
// true  = test en boucle ouverte (puissance proportionnelle, sans PI)
// false = régulation PI de la vitesse avec le retour FG
const bool OPEN_LOOP = true;
 
 
// ------------------------------------------------------------
//  ROUES
// ------------------------------------------------------------
 
// Chaque roue a : un nom, une broche PWM (vitesse), une broche de
// direction, une broche de retour de vitesse (FG, compte les impulsions),
// et des variables pour la régulation de vitesse.
struct Wheel {
  const char* name;
  uint8_t pwmPin, dirPin, fgPin;
  bool invert;              // à inverser si la roue tourne dans le mauvais sens
  volatile uint32_t pulses; // impulsions comptées (modifié par l'interruption)
  float target;             // vitesse voulue en tours/min (+ = vers l'avant)
  float rpm;                // vitesse mesurée
  float integral;           // mémoire du régulateur (partie "I" du PI)
  bool fwd;                 // sens actuel de la roue
  bool reversing;           // true pendant un changement de sens
  unsigned long revStart;   // moment où le changement de sens a commencé
};
 
Wheel wheels[4] = {
// name  pwm dir fg
  {"FL", 18, 19, 34, false, 0, 0, 0, 0, true, false, 0},
  {"BL", 16, 17, 35, false, 0, 0, 0, 0, true, false, 0},
  {"FR", 25, 33, 32, true,  0, 0, 0, 0, true, false, 0},
  {"_BR", 27, 13, 26, true, 0, 0, 0, 0, true, false, 0},
};
// Noms pratiques pour accéder aux roues : wheels[FL], wheels[BL]...
enum { FL, BL, FR, _BR };

 
 
// ------------------------------------------------------------
//  LECTURE DES SIGNAUX PWM DU RÉCEPTEUR
// ------------------------------------------------------------
 
// Pour chaque entrée, on mesure la durée de l'impulsion HAUTE.
struct PwmIn {
  uint8_t pin;
  volatile uint32_t rise;       // micros() au front montant
  volatile uint16_t width;      // largeur d'impulsion en µs
  volatile uint32_t lastUpdate; // millis() de la dernière impulsion valide
};
PwmIn pwmIn[4];
 
// INTERRUPTION : appelée à chaque changement (montant ou descendant)
// d'une entrée. IRAM_ATTR = code placé en mémoire rapide.
void IRAM_ATTR onPwm(void* arg) {
  PwmIn* p = (PwmIn*)arg;
  if (digitalRead(p->pin)) {
    p->rise = micros();                 // début de l'impulsion
  } else {
    uint32_t w = micros() - p->rise;    // fin : on calcule la durée
    if (w > 800 && w < 2200) {          // on ignore les valeurs aberrantes
      p->width = w;
      p->lastUpdate = millis();
    }
  }
}
 
// INTERRUPTION : appelée à chaque impulsion du capteur d'une roue.
void IRAM_ATTR onFG(void* arg) { ((Wheel*)arg)->pulses++; }
 
unsigned long lastCtrlMs  = 0;  // pour cadencer la lecture manette (20 ms)
unsigned long lastMs      = 0;  // pour cadencer la régulation (100 ms)
unsigned long lastPrintMs = 0;  // pour cadencer l'affichage
 
 
// ------------------------------------------------------------
//  FONCTIONS DES ROUES
// ------------------------------------------------------------
 
// Règle la puissance du moteur (0 à 255).
// Le signal est inversé : niveau BAS = moteur allumé.
void setDuty(Wheel& w, int duty) {
  duty = constrain(duty, 0, 255);
  ledcWrite(w.pwmPin, 255 - duty);
}
 
// Règle le sens de rotation. 'invert' compense les roues montées
// en miroir du côté droit.
void writeDir(Wheel& w, bool fwd) {
  bool level = fwd ? w.invert : !w.invert;
  digitalWrite(w.dirPin, level ? HIGH : LOW);
}
 
// Calcule la vitesse de chacune des 4 roues pour obtenir un mouvement.
//   vx : avancer (+) / reculer (-)
//   vy : strafe droite (+) / gauche (-)
//   wz : tourner à droite (+) / gauche (-)
// Chaque valeur va de -1 à 1.
void drive(float vx, float vy, float wz) {
  float s[4];
  s[FL]  = vx + vy + wz;
  s[FR]  = vx - vy - wz;
  s[BL]  = vx - vy + wz;
  s[_BR] = vx + vy - wz;
 
  // Si un mélange dépasse 1 (ex. avancer + strafe + tourner),
  // on réduit tout proportionnellement pour garder la bonne direction.
  float m = 1;
  for (int i = 0; i < 4; i++) if (fabs(s[i]) > m) m = fabs(s[i]);
  for (int i = 0; i < 4; i++) wheels[i].target = (s[i] / m) * MAX_RPM;
}
 
 
// ------------------------------------------------------------
//  FONCTION MANETTE
// ------------------------------------------------------------
 
// Transforme la largeur d'impulsion d'un stick (1100-1900 µs) en
// nombre de -1 à +1.
//   1500 -> 0,  1100 -> -1,  1900 -> +1
// La zone morte autour du centre est ramenée à 0 exactement.
float stick(uint16_t us, bool invert) {
  float x = ((int)us - 1500) / STICK_HALF_RANGE;
  x = constrain(x, -1.0, 1.0);
 
  if (fabs(x) < DEADZONE) return 0;          // stick au centre
 
  // On re-étale le reste de la course sur 0..1 pour que la vitesse
  // démarre doucement juste après la zone morte.
  float sign = (x > 0) ? 1.0 : -1.0;
  x = sign * (fabs(x) - DEADZONE) / (1.0 - DEADZONE);
 
  return invert ? -x : x;
}
 
 
// ------------------------------------------------------------
//  SETUP : une seule fois au démarrage
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
 
  // Initialisation des 4 entrées PWM du récepteur
  for (int i = 0; i < 4; i++) {
    pwmIn[i].pin = PWM_PINS[i];
    pwmIn[i].width = 1500;          // valeur neutre au départ
    pwmIn[i].lastUpdate = 0;
    pinMode(PWM_PINS[i], INPUT);
    attachInterruptArg(PWM_PINS[i], onPwm, &pwmIn[i], CHANGE);
  }
 
  // Initialisation des 4 roues
  for (auto& w : wheels) {
    pinMode(w.dirPin, OUTPUT);
    pinMode(w.fgPin, INPUT);
    ledcAttach(w.pwmPin, PWM_FREQ, PWM_RES);
    setDuty(w, 0);                 // moteur à l'arrêt
    writeDir(w, true);
    attachInterruptArg(w.fgPin, onFG, &w, RISING); // compte les impulsions
  }
 
  Serial.println("Robot pret. Allume la manette, interrupteur A a 1 pour activer.");
}
 
 
// ------------------------------------------------------------
//  LOOP : tourne en boucle en permanence
// ------------------------------------------------------------
void loop() {
 
  // --- 1. Lecture de la manette (toutes les 20 ms) ---
  if (millis() - lastCtrlMs >= 20) {
    lastCtrlMs = millis();
 
    // Copie atomique des valeurs et vérification que chaque signal est frais
    uint16_t v[4];
    bool ok = true;
    unsigned long nowMs = millis();
    noInterrupts();
    for (int i = 0; i < 4; i++) {
      v[i] = pwmIn[i].width;
      if (nowMs - pwmIn[i].lastUpdate > FAILSAFE_MS) ok = false;
    }
    interrupts();
 
    // L'interrupteur A est-il en position "activé" ?
    bool swOn = SWITCH_ACTIVE_HIGH ? (v[IN_SWITCH] > SWITCH_THRESHOLD_US)
                                   : (v[IN_SWITCH] < SWITCH_THRESHOLD_US);
 
    if (ok && swOn) {
      float vx = stick(v[IN_FORWARD], INV_FORWARD);  // avant/arrière
      float vy = stick(v[IN_STRAFE],  INV_STRAFE);   // latéral
      float wz = stick(v[IN_TURN],    INV_TURN);     // rotation
      drive(vx, vy, wz);
    } else {
      drive(0, 0, 0);   // interrupteur à 0 ou signal perdu -> arrêt
    }
 
    // Affichage des valeurs brutes pour vérifier le mapping (toutes les 200 ms)
    static unsigned long lastDbg = 0;
    if (DEBUG_INPUTS && millis() - lastDbg >= 200) {
      lastDbg = millis();
      Serial.printf("FWD:%u TURN:%u SW:%u STRAFE:%u | %s%s\n",
                    v[IN_FORWARD], v[IN_TURN], v[IN_SWITCH], v[IN_STRAFE],
                    ok ? "signal OK" : "SIGNAL PERDU",
                    swOn ? " | ACTIF" : " | ARRET (inter. A)");
    }
  }
 
  // --- 2. Régulation de vitesse toutes les 100 ms ---
  unsigned long now = millis();
  if (now - lastMs < 100) return;
  float dt = (now - lastMs) / 1000.0;
  lastMs = now;
 
  for (auto& w : wheels) {
    // Lecture atomique du compteur d'impulsions, puis remise à zéro
    noInterrupts();
    uint32_t p = w.pulses; w.pulses = 0;
    interrupts();
 
    // Impulsions -> tours par minute
    w.rpm = (p / dt) * 60.0 / PULSES_REV;
 
    float mag = fabs(w.target);        // vitesse voulue (sans signe)
    bool wantFwd = (w.target >= 0);    // sens voulu
 
    // Cible quasi nulle : moteur coupé
    if (mag < 1) {
      setDuty(w, 0);
      w.integral = 0;
      continue;
    }
 
    // Changement de sens : on laisse d'abord la roue s'arrêter
    if (wantFwd != w.fwd) {
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
 
    if (OPEN_LOOP) {
      // TEST EN BOUCLE OUVERTE (sans PI) : puissance proportionnelle
      setDuty(w, (int)(mag * FF));
    } else {
      // Régulateur PI : compare vitesse voulue et mesurée
      float err = mag - w.rpm;
      w.integral += err * dt;
      w.integral = constrain(w.integral, -150, 150);
      float out = KP * err + KI * w.integral + mag * FF;
      setDuty(w, (int)out);
    }
  }
 
  // --- 3. Affichage de la vitesse des roues (toutes les 500 ms) ---
  if (now - lastPrintMs >= 500) {
    lastPrintMs = now;
    Serial.printf("FL %.0f BL %.0f FR %.0f _BR %.0f RPM\n",
                  wheels[FL].rpm, wheels[BL].rpm, wheels[FR].rpm, wheels[_BR].rpm);
  }
}
 