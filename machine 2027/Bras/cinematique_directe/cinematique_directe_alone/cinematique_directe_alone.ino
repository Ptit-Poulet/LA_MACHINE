// Test cinématique directe sans manette avec des points décidés

/*
  CINEMATIQUE DIRECTE - Bras robot avec moteurs pas à pas
    - NEMA 17 : rotation de la base (épaule, axe vertical)
    - NEMA 23 : élévation de l'épaule
    - NEMA 23 : coude

  On commande directement les ANGLES, et le programme calcule
  la position (x, y, z) du bout du bras (moniteur série, 115200 bauds).
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

// ----------- Limites des angles (degrés, à adapter) -----------
const float THETA_MIN = -90, THETA_MAX = 90;
const float Q1_MIN = -30,    Q1_MAX = 120;
const float Q2_MIN = -150,   Q2_MAX = 0;

// ----------- Pose de départ (bras placé à la main) -----------
const float THETA_START = 0;
const float Q1_START    = 45;
const float Q2_START    = -90;

// ----------- Vitesse de commande (deg/s) -----------
const float VITESSE_ANGLE = 30.0;

AccelStepper moteurBase  (AccelStepper::DRIVER, PIN_STEP_BASE,   PIN_DIR_BASE);
AccelStepper moteurEpaule(AccelStepper::DRIVER, PIN_STEP_EPAULE, PIN_DIR_EPAULE);
AccelStepper moteurCoude (AccelStepper::DRIVER, PIN_STEP_COUDE,  PIN_DIR_COUDE);

float theta = THETA_START, q1 = Q1_START, q2 = Q2_START;
unsigned long tPrecedent, tAffichage;

float lireJoystick(int pin) {
  float v = (analogRead(pin) - 512) / 512.0;
  if (abs(v) < 0.10) return 0;      // zone morte
  return v;
}

// +1 = position 2, 0 = position 1, -1 = position 0
int lireSwitch() {
  if (digitalRead(PIN_SW_HAUT) == LOW) return +1;
  if (digitalRead(PIN_SW_BAS)  == LOW) return -1;
  return 0;
}

// ---------- CINEMATIQUE DIRECTE ----------
void cinematiqueDirecte(float theta_d, float q1_d, float q2_d,
                        float &x, float &y, float &z) {
  float th = radians(theta_d);
  float a1 = radians(q1_d);
  float a2 = radians(q2_d);

  float r = L1 * cos(a1) + L2 * cos(a1 + a2);  // portée horizontale
  z       = L1 * sin(a1) + L2 * sin(a1 + a2);  // hauteur
  x = r * cos(th);
  y = r * sin(th);
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

    theta += lireJoystick(PIN_JOY_X) * VITESSE_ANGLE * dt;
    q1    += lireJoystick(PIN_JOY_Y) * VITESSE_ANGLE * dt;
    q2    += lireSwitch()            * VITESSE_ANGLE * dt;

    theta = constrain(theta, THETA_MIN, THETA_MAX);
    q1    = constrain(q1, Q1_MIN, Q1_MAX);
    q2    = constrain(q2, Q2_MIN, Q2_MAX);

    moteurBase.moveTo(lround(theta * PAS_DEG_BASE));
    moteurEpaule.moveTo(lround(q1  * PAS_DEG_EPAULE));
    moteurCoude.moveTo(lround(q2   * PAS_DEG_COUDE));
  }

  if (maintenant - tAffichage >= 250) {         // affichage 4 fois / s
    tAffichage = maintenant;
    float x, y, z;
    cinematiqueDirecte(theta, q1, q2, x, y, z);

    Serial.print("theta="); Serial.print(theta, 1);
    Serial.print(" q1=");   Serial.print(q1, 1);
    Serial.print(" q2=");   Serial.print(q2, 1);
    Serial.print(" | x=");  Serial.print(x, 1);
    Serial.print(" y=");    Serial.print(y, 1);
    Serial.print(" z=");    Serial.println(z, 1);
  }
}
