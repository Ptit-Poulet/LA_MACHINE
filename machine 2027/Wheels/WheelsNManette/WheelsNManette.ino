// ============================================================
//  ROBOT MECANUM 4 ROUES PILOTÉ PAR MANETTE FLYSKY (iBUS)
// ============================================================
//
//  COMMANDES :
//    Stick GAUCHE haut/bas       (CH2) -> avancer / reculer
//    Stick GAUCHE gauche/droite  (CH4) -> déplacement latéral (strafe)
//    Stick DROIT  gauche/droite  (CH1) -> tourner gauche / droite
//
//  CÂBLAGE DU RÉCEPTEUR FS-iA10B (port "SERVO" = iBUS) :
//    Signal -> GPIO 22 de l'ESP32   (PAS le 16 : utilisé par une roue)
//    +      -> 5V / VIN
//    -      -> GND
//
//  SÉCURITÉ : si le récepteur n'envoie plus rien pendant
//  500 ms (manette éteinte, fil débranché), le robot s'arrête.
// ============================================================


// Broche qui reçoit le signal iBUS.
#define IBUS_RX_PIN   22

// Nombre de canaux d'une trame iBUS.
#define NUM_CHANNELS  14

// Numéros des canaux dans le tableau ch[] (attention : ch[0] = CH1).
#define CH_TURN       0   // CH1 : stick droit,  gauche/droite -> rotation
#define CH_FORWARD    1   // CH2 : stick gauche, haut/bas      -> avant/arrière
#define CH_STRAFE     3   // CH4 : stick gauche, gauche/droite -> latéral

// Zone morte : un stick ne bouge jamais exactement à 1500 au repos.
// En dessous de ce seuil (8 % de la course), on considère "stick au centre".
const float DEADZONE = 0.08;

// Si un mouvement part dans le MAUVAIS sens
const bool INV_FORWARD = false;
const bool INV_STRAFE  = false;
const bool INV_TURN    = false;

// Délai sans trame valide au-delà duquel le robot s'arrête (ms).
const unsigned long FAILSAFE_MS = 500;

// Chaque roue a : un nom, une broche PWM (vitesse), une broche de
// direction, une broche de retour de vitesse (FG, compte les impulsions),
// et des variables pour la régulation de vitesse.
struct Wheel {
  const char* name;
  uint8_t pwmPin, dirPin, fgPin;
  bool invert;            // à inverser si la roue tourne dans le mauvais sens
  volatile uint32_t pulses; // impulsions comptées (modifié par l'interruption)
  float target;           // vitesse voulue en tours/min (+ = vers l'avant)
  float rpm;              // vitesse mesurée
  float integral;         // mémoire du régulateur (partie "I" du PI)
  bool fwd;               // sens actuel de la roue
  bool reversing;         // true pendant un changement de sens
  unsigned long revStart; // moment où le changement de sens a commencé
};

Wheel wheels[4] = {
// name  pwm dir fg
  {"FL", 18, 19, 34, false, 0, 0, 0, 0, true, false, 0},
  {"BL", 16, 17, 35, false, 0, 0, 0, 0, true, false, 0},
  {"FR", 25, 26, 32, true,  0, 0, 0, 0, true, false, 0},
  {"_BR", 27, 13, 33, true, 0, 0, 0, 0, true, false, 0},
};
// Noms pratiques pour accéder aux roues : wheels[FL], wheels[BL]...
enum { FL, BL, FR, _BR };

// Réglages du moteur et du régulateur de vitesse
const int   PWM_FREQ   = 20000;        // fréquence PWM (20 kHz, inaudible)
const int   PWM_RES    = 8;            // résolution 8 bits : duty de 0 à 255
const float PULSES_REV = 9.0 * 30.0;   // 270 impulsions par tour de roue
const float MAX_RPM    = 150;          // vitesse max visée (le moteur monte à 200 à vide)
const float KP = 0.4, KI = 0.8, FF = 1.2; // gains du régulateur de vitesse

uint8_t  buf[32];             // trame iBUS en cours de réception (32 octets)
uint8_t  idx = 0;             // position actuelle dans le tampon
uint16_t ch[NUM_CHANNELS];    // valeur de chaque canal (~1000 à ~2000)
unsigned long lastFrameMs = 0; // moment de la dernière trame valide

unsigned long lastMs = 0;      // pour cadencer la régulation (100 ms)
unsigned long lastPrintMs = 0; // pour cadencer l'affichage

//  INTERRUPTION : appelée à chaque impulsion du capteur d'une roue.
//  Elle ne fait que compter. IRAM_ATTR = code placé en mémoire rapide.
void IRAM_ATTR onFG(void* arg) { ((Wheel*)arg)->pulses++; }


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
//  FONCTIONS MANETTE
// ------------------------------------------------------------

// Transforme la valeur d'un stick (1000-2000) en nombre de -1 à +1.
//   1500 -> 0,  1000 -> -1,  2000 -> +1
// La zone morte autour du centre est ramenée à 0 exactement.
float stick(uint16_t v, bool invert) {
  float x = ((int)v - 1500) / 500.0;
  x = constrain(x, -1.0, 1.0);

  if (fabs(x) < DEADZONE) return 0;          // stick au centre

  // On re-étale le reste de la course sur 0..1 pour que la vitesse
  // démarre doucement juste après la zone morte.
  float sign = (x > 0) ? 1.0 : -1.0;
  x = sign * (fabs(x) - DEADZONE) / (1.0 - DEADZONE);

  return invert ? -x : x;
}

// Lit les octets du récepteur et reconstitue une trame iBUS.
// Retourne true quand une trame complète ET valide est lue
// (les canaux sont alors à jour dans ch[]).
// Trame : 0x20 0x40 | 14 canaux x 2 octets | checksum (2 octets)
bool readIBus() {
  while (Serial2.available()) {
    uint8_t b = Serial2.read();

    // Synchronisation sur le début de trame (0x20 puis 0x40)
    if (idx == 0 && b != 0x20) continue;
    if (idx == 1 && b != 0x40) { idx = 0; continue; }

    buf[idx++] = b;

    if (idx == 32) {                    // trame complète
      idx = 0;

      // Vérification du checksum : si faux, trame corrompue -> on jette
      uint16_t sum = 0xFFFF;
      for (int i = 0; i < 30; i++) sum -= buf[i];
      uint16_t rx = buf[30] | (buf[31] << 8);
      if (sum != rx) continue;

      // Extraction des canaux (2 octets chacun, octet bas en premier)
      for (int i = 0; i < NUM_CHANNELS; i++) {
        ch[i] = buf[2 + i * 2] | (buf[3 + i * 2] << 8);
      }
      return true;
    }
  }
  return false;
}


// ------------------------------------------------------------
//  SETUP : une seule fois au démarrage
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Port série n°2 : écoute le récepteur FlySky (réception seulement).
  Serial2.begin(115200, SERIAL_8N1, IBUS_RX_PIN, -1);

  // Initialisation des 4 roues
  for (auto& w : wheels) {
    pinMode(w.dirPin, OUTPUT);
    pinMode(w.fgPin, INPUT);
    ledcAttach(w.pwmPin, PWM_FREQ, PWM_RES);
    setDuty(w, 0);                 // moteur à l'arrêt
    writeDir(w, true);
    attachInterruptArg(w.fgPin, onFG, &w, RISING); // compte les impulsions
  }

  Serial.println("Robot pret. Allume la manette.");
}


// ------------------------------------------------------------
//  LOOP : tourne en boucle en permanence
// ------------------------------------------------------------
void loop() {

  // --- 1. Lecture de la manette ---
  // Cette partie doit s'exécuter à chaque tour de loop, avant tout
  // 'return', pour ne perdre aucune trame.
  if (readIBus()) {
    lastFrameMs = millis();

    float vx = stick(ch[CH_FORWARD], INV_FORWARD);  // avant/arrière
    float vy = stick(ch[CH_STRAFE],  INV_STRAFE);   // latéral
    float wz = stick(ch[CH_TURN],    INV_TURN);     // rotation
    drive(vx, vy, wz);
  }

  // --- 2. Sécurité : plus de signal -> on arrête tout ---
  if (millis() - lastFrameMs > FAILSAFE_MS) {
    drive(0, 0, 0);
  }

  // --- 3. Régulation de vitesse toutes les 100 ms ---
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

    // Régulateur de vitesse : compare vitesse voulue et mesurée
    float err = mag - w.rpm;
    w.integral += err * dt;
    w.integral = constrain(w.integral, -150, 150);
    float out = KP * err + KI * w.integral + mag * FF;
    setDuty(w, (int)out);
  }

  // --- 4. Affichage de la vitesse des roues (toutes les 500 ms) ---
  if (now - lastPrintMs >= 500) {
    lastPrintMs = now;
    Serial.printf("FL %.0f BL %.0f FR %.0f _BR %.0f RPM\n",
                  wheels[FL].rpm, wheels[BL].rpm, wheels[FR].rpm, wheels[_BR].rpm);
  }
}