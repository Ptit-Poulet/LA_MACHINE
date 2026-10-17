// Deux moteurs avec TB6600 : saisie des pas M2 puis M3, mouvement simultané
// Configuration DIP : SW1 ON, SW2 ON, SW3 OFF, SW4 OFF, SW5 OFF, SW6 ON

// Moteur 2
const int PULM2 = 25;
const int DIRM2 = 26;
const int ENAM2 = 27;
// Moteur 3
const int PULM3 = 32;
const int DIRM3 = 33;
const int ENAM3 = 13;

const int  STEP_US      = 500;  // demi-période (µs) du moteur le plus long. 500 = plus rapide
const bool POSITIVE_DIR = LOW;   // mets HIGH si le sens est inversé

long posM2 = 0;
long posM3 = 0;

// Lit une ligne au clavier (bloquant). Retourne la chaîne nettoyée.
String readLine(const char* prompt) {
  Serial.println(prompt);
  while (!Serial.available()) delay(10);
  String s = Serial.readStringUntil('\n');
  s.trim();
  return s;
}

void printPositions() {
  Serial.print("Position M2 : "); Serial.print(posM2);
  Serial.print(" | M3 : ");       Serial.println(posM3);
}

void moveBoth(long s2, long s3) {
  if (s2 == 0 && s3 == 0) return;

  while (Serial.available()) Serial.read();   // vide le buffer

  bool pos2 = s2 > 0;
  bool pos3 = s3 > 0;
  digitalWrite(DIRM2, pos2 ? POSITIVE_DIR : !POSITIVE_DIR);
  digitalWrite(DIRM3, pos3 ? POSITIVE_DIR : !POSITIVE_DIR);
  delayMicroseconds(20);

  long n2 = labs(s2);
  long n3 = labs(s3);
  long N  = max(n2, n3);          // nombre de "temps" total
  long err2 = 0, err3 = 0;

  for (long i = 0; i < N; i++) {
    bool step2 = false, step3 = false;

    err2 += n2;
    if (err2 >= N) { err2 -= N; step2 = true; }
    err3 += n3;
    if (err3 >= N) { err3 -= N; step3 = true; }

    if (step2) digitalWrite(PULM2, HIGH);
    if (step3) digitalWrite(PULM3, HIGH);
    delayMicroseconds(STEP_US);
    digitalWrite(PULM2, LOW);
    digitalWrite(PULM3, LOW);
    delayMicroseconds(STEP_US);

    if (step2) posM2 += pos2 ? 1 : -1;
    if (step3) posM3 += pos3 ? 1 : -1;

    // Arrêt d'urgence : n'importe quel caractère envoyé
    if (i > 0 && (i % 50) == 0 && Serial.available()) {
      while (Serial.available()) Serial.read();
      Serial.println("Arret !");
      break;
    }
  }
  printPositions();
}

void setup() {
  Serial.begin(115200);
  pinMode(PULM2, OUTPUT); pinMode(DIRM2, OUTPUT); pinMode(ENAM2, OUTPUT);
  pinMode(PULM3, OUTPUT); pinMode(DIRM3, OUTPUT); pinMode(ENAM3, OUTPUT);

  digitalWrite(ENAM2, LOW); digitalWrite(DIRM2, POSITIVE_DIR); digitalWrite(PULM2, LOW);
  digitalWrite(ENAM3, LOW); digitalWrite(DIRM3, POSITIVE_DIR); digitalWrite(PULM3, LOW);

  Serial.println("Pas M2 puis pas M3 (+ ou -), les deux moteurs bougent ensemble.");
  Serial.println("'p' = positions, 'z' = remise a zero.");
}

void loop() {
  String a = readLine("Pas M2 ?");

  if (a == "p") { printPositions(); return; }
  if (a == "z") { posM2 = 0; posM3 = 0; Serial.println("Positions remises a 0"); return; }

  String b = readLine("Pas M3 ?");
  if (b == "p") { printPositions(); return; }
  if (b == "z") { posM2 = 0; posM3 = 0; Serial.println("Positions remises a 0"); return; }

  long s2 = a.toInt();
  long s3 = b.toInt();

  Serial.print("Deplacement M2 : "); Serial.print(s2);
  Serial.print(" | M3 : ");         Serial.println(s3);
  moveBoth(s2, s3);
}