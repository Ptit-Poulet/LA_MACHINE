/*
  CINEMATIQUE INVERSE - Bras robot avec moteurs pas à pas
    - NEMA 17 : rotation de la base (épaule, axe vertical)
    - NEMA 23 : élévation de l'épaule
    - NEMA 23 : coude

  La manette déplace une POSITION CIBLE (theta, r, z) du bout du bras.
  La cinématique inverse calcule les angles de l'épaule et du coude.

  Manette :
    Joystick Y      -> le bras avance / recule      (r)
    Joystick X      -> rotation de l'épaule (base)  (theta)
    Switch 3 états  -> 2 = le bras monte, 1 = stop, 0 = descend  (z)

  Bibliothèque requise : AccelStepper (Gestionnaire de bibliothèques Arduino)

  IMPORTANT : sans capteur de position, le bras doit être placé À LA MAIN
  dans la pose de départ (THETA_START, Q1_START, Q2_START) AVANT d'alimenter
  les moteurs / démarrer le programme.
*/

#include <AccelStepper.h>

// ----------- Branchements drivers STEP / DIR (à adapter) -----------
const int PIN_STEP_BASE   = 4;   // NEMA 17
const int PIN_DIR_BASE    = 5;
const int PIN_STEP_EPAULE = 6;   // NEMA 23
const int PIN_DIR_EPAULE  = 7;
const int PIN_STEP_COUDE  = 8;   // NEMA 23
const int PIN_DIR_COUDE   = 9;

const int PIN_JOY_X = A0;
const int PIN_JOY_Y = A1;
const int PIN_SW_HAUT = 2;   // switch en position 2 -> relié à GND
const int PIN_SW_BAS  = 3;   // switch en position 0 -> relié à GND
                             // position 1 (milieu) : aucun des deux

// ----------- Inversion du sens de rotation (si un axe tourne à l'envers) -----------
const bool INV_BASE   = false;
const bool INV_EPAULE = false;
const bool INV_COUDE  = false;

// ----------- Pas par degré (à adapter) -----------
// pas/deg = (200 pas/tour x microstepping x réducteur) / 360
const float MICROPAS_BASE    = 16;    // réglage du driver
const float REDUCTION_BASE   = 5.0;   // ex. réducteur 5:1 (1 si entraînement direct)
const float MICROPAS_EPAULE  = 8;
const float REDUCTION_EPAULE = 20.0;
const float MICROPAS_COUDE   = 8;
const float REDUCTION_COUDE  = 20.0;

const float PAS_DEG_BASE   = 200.0 * MICROPAS_BASE   * REDUCTION_BASE   / 360.0;
const float PAS_DEG_EPAULE = 200.0 * MICROPAS_EPAULE * REDUCTION_EPAULE / 360.0;
const float PAS_DEG_COUDE  = 200.0 * MICROPAS_COUDE  * REDUCTION_COUDE  / 360.0;

// ----------- Vitesse / accélération des moteurs (pas/s et pas/s²) -----------
// Commencez bas, augmentez progressivement.
const float VMAX_BASE   = 2000, ACC_BASE   = 3000;
const float VMAX_EPAULE = 3000, ACC_EPAULE = 4000;
const float VMAX_COUDE  = 3000, ACC_COUDE  = 4000;

// ----------- Géométrie du bras (cm, à adapter) -----------
const float L1 = 15.0;  // épaule -> coude
const float L2 = 15.0;  // coude -> bout

// ----------- Vitesses de la cible -----------
const float VITESSE_R     = 6.0;    // cm/s  (joystick Y)
const float VITESSE_Z     = 5.0;    // cm/s  (switch)
const float VITESSE_THETA = 30.0;   // deg/s (joystick X)

// ----------- Limites des angles (degrés, à adapter) -----------
const float THETA_MIN = -90, THETA_MAX = 90;
const float Q1_MIN = -30,    Q1_MAX = 120;
const float Q2_MIN = -150,   Q2_MAX = 0;

// ----------- Pose de départ (bras placé à la main) -----------
const float THETA_START = 0;
const float Q1_START    = 45;
const float Q2_START    = -90;

AccelStepper moteurBase  (AccelStepper::DRIVER, PIN_STEP_BASE,   PIN_DIR_BASE);
AccelStepper moteurEpaule(AccelStepper::DRIVER, PIN_STEP_EPAULE, PIN_DIR_EPAULE);
AccelStepper moteurCoude (AccelStepper::DRIVER, PIN_STEP_COUDE,  PIN_DIR_COUDE);

float theta, r, z;                 // cible actuelle (calculée dans setup)
float q1Cmd, q2Cmd;                // angles correspondants
unsigned long tPrecedent, tAffichage;

float lireJoystick(int pin) {
  float v = (analogRead(pin) - 512) / 512.0;
  if (abs(v) < 0.10) return 0;      // zone morte
  return v;
}

// +1 = position 2 (monte), 0 = position 1 (stop), -1 = position 0 (descend)
int lireSwitch() {
  if (digitalRead(PIN_SW_HAUT) == LOW) return +1;
  if (digitalRead(PIN_SW_BAS)  == LOW) return -1;
  return 0;
}

// ---------- CINEMATIQUE INVERSE ----------
// (r, z) -> (q1, q2) en degrés. Retourne false si la cible est inatteignable.
bool cinematiqueInverse(float r_, float z_, float &q1, float &q2) {
  float d2 = r_ * r_ + z_ * z_;
  float c2 = (d2 - L1 * L1 - L2 * L2) / (2.0 * L1 * L2);
  if (c2 < -1.0 || c2 > 1.0) return false;   // trop loin ou trop près

  float a2 = -acos(c2);                      // coude vers le haut
  float a1 = atan2(z_, r_) - atan2(L2 * sin(a2), L1 + L2 * cos(a2));

  q1 = degrees(a1);
  q2 = degrees(a2);
  return true;
}

void configMoteur(AccelStepper &m, bool inv, float vmax, float acc,
                  float angleDepart, float pasDeg) {
  m.setPinsInverted(inv, false, false);   // (direction, step, enable)
  m.setMaxSpeed(vmax);
  m.setAcceleration(acc);
  m.setCurrentPosition(lround(angleDepart * pasDeg));
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SW_HAUT, INPUT_PULLUP);
  pinMode(PIN_SW_BAS,  INPUT_PULLUP);

  configMoteur(moteurBase,   INV_BASE,   VMAX_BASE,   ACC_BASE,   THETA_START, PAS_DEG_BASE);
  configMoteur(moteurEpaule, INV_EPAULE, VMAX_EPAULE, ACC_EPAULE, Q1_START,    PAS_DEG_EPAULE);
  configMoteur(moteurCoude,  INV_COUDE,  VMAX_COUDE,  ACC_COUDE,  Q2_START,    PAS_DEG_COUDE);

  // Cible de départ = position du bout du bras dans la pose de départ
  float a1 = radians(Q1_START), a2 = radians(Q2_START);
  theta = THETA_START;
  r = L1 * cos(a1) + L2 * cos(a1 + a2);
  z = L1 * sin(a1) + L2 * sin(a1 + a2);
  q1Cmd = Q1_START;
  q2Cmd = Q2_START;

  tPrecedent = tAffichage = millis();
}

void loop() {
  // À appeler le plus souvent possible : génère les impulsions STEP
  moteurBase.run();
  moteurEpaule.run();
  moteurCoude.run();

  unsigned long maintenant = millis();

  if (maintenant - tPrecedent >= 20) {          // lecture manette à 50 Hz
    float dt = (maintenant - tPrecedent) / 1000.0;
    tPrecedent = maintenant;

    // Nouvelle cible demandée par la manette
    float newTheta = theta + lireJoystick(PIN_JOY_X) * VITESSE_THETA * dt;
    float newR     = r     + lireJoystick(PIN_JOY_Y) * VITESSE_R     * dt;
    float newZ     = z     + lireSwitch()            * VITESSE_Z     * dt;

    // Cinématique inverse
    float q1, q2;
    bool ok = cinematiqueInverse(newR, newZ, q1, q2);

    // On accepte la cible seulement si atteignable et dans les limites
    if (ok &&
        newTheta >= THETA_MIN && newTheta <= THETA_MAX &&
        q1 >= Q1_MIN && q1 <= Q1_MAX &&
        q2 >= Q2_MIN && q2 <= Q2_MAX) {

      theta = newTheta;
      r = newR;
      z = newZ;
      q1Cmd = q1;
      q2Cmd = q2;

      moteurBase.moveTo(lround(theta * PAS_DEG_BASE));
      moteurEpaule.moveTo(lround(q1  * PAS_DEG_EPAULE));
      moteurCoude.moveTo(lround(q2   * PAS_DEG_COUDE));
    }
    // sinon : le bras reste à la dernière cible valide
  }

  if (maintenant - tAffichage >= 250) {         // affichage 4 fois / s
    tAffichage = maintenant;
    Serial.print("theta="); Serial.print(theta, 1);
    Serial.print(" r=");    Serial.print(r, 1);
    Serial.print(" z=");    Serial.print(z, 1);
    Serial.print(" | q1="); Serial.print(q1Cmd, 1);
    Serial.print(" q2=");   Serial.println(q2Cmd, 1);
  }
}
