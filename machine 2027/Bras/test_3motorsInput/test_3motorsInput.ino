// Trois moteurs : M1 = NEMA17 (driver STEP/DIR), M2 et M3 = NEMA23 (TB6600)
// Saisie des pas M1, M2, M3 puis mouvement simultané
// TB6600 : SW1 ON, SW2 ON, SW3 OFF, SW4 OFF, SW5 OFF, SW6 ON

//TODO: m1 vrm + vite que les deux autres

// Moteur 1 (NEMA17)
const int STPM1 = 18;
const int DIRM1 = 19;

// Moteur 2 (NEMA23)
const int PULM2 = 32;
const int DIRM2 = 33;
const int ENAM2 = 13;

// Moteur 3 (NEMA23)
const int PULM3 = 25;
const int DIRM3 = 26;
const int ENAM3 = 27;

const int  STEP_US      = 1500;   // demi-période (µs) : 500 = plus rapide
const bool POSITIVE_DIR = LOW;   // mets HIGH si le sens est inversé

long posM1 = 0;
long posM2 = 0;
long posM3 = 0;

// Stopper execution
String readLine(const char* prompt) {
  Serial.println(prompt);
  while (!Serial.available()) delay(10);
  String s = Serial.readStringUntil('\n');
  s.trim();
  return s;
}

void printPositions() {
  Serial.print("Position M1 : "); Serial.print(posM1);
  Serial.print(" | M2 : ");       Serial.print(posM2);
  Serial.print(" | M3 : ");       Serial.println(posM3);
}

// Retourne true si la saisie était une commande ('p' ou 'z')
bool handleCommand(const String& s) {
  if (s == "p") { printPositions(); return true; }
  if (s == "z") {
    posM1 = posM2 = posM3 = 0;
    Serial.println("Positions remises a 0");
    return true;
  }
  return false;
}

void moveAll(long s1, long s2, long s3) {
  if (s1 == 0 && s2 == 0 && s3 == 0) return;

  while (Serial.available()) Serial.read();   // vide le buffer

  bool pos1 = s1 > 0;
  bool pos2 = s2 > 0;
  bool pos3 = s3 > 0;
  digitalWrite(DIRM1, pos1 ? POSITIVE_DIR : !POSITIVE_DIR);
  digitalWrite(DIRM2, pos2 ? POSITIVE_DIR : !POSITIVE_DIR);
  digitalWrite(DIRM3, pos3 ? POSITIVE_DIR : !POSITIVE_DIR);
  delayMicroseconds(20);

  long n1 = labs(s1);
  long n2 = labs(s2);
  long n3 = labs(s3);
  long N  = max(n1, max(n2, n3));   // nombre de "temps" total
  long err1 = 0, err2 = 0, err3 = 0;

  for (long i = 0; i < N; i++) {
    bool step1 = false, step2 = false, step3 = false;

    err1 += n1;
    if (err1 >= N) { err1 -= N; step1 = true; }
    err2 += n2;
    if (err2 >= N) { err2 -= N; step2 = true; }
    err3 += n3;
    if (err3 >= N) { err3 -= N; step3 = true; }

    if (step1) digitalWrite(STPM1, HIGH);
    if (step2) digitalWrite(PULM2, HIGH);
    if (step3) digitalWrite(PULM3, HIGH);
    delayMicroseconds(STEP_US);
    digitalWrite(STPM1, LOW);
    digitalWrite(PULM2, LOW);
    digitalWrite(PULM3, LOW);
    delayMicroseconds(STEP_US);

    if (step1) posM1 += pos1 ? 1 : -1;
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

  pinMode(STPM1, OUTPUT); pinMode(DIRM1, OUTPUT);
  pinMode(PULM2, OUTPUT); pinMode(DIRM2, OUTPUT); pinMode(ENAM2, OUTPUT);
  pinMode(PULM3, OUTPUT); pinMode(DIRM3, OUTPUT); pinMode(ENAM3, OUTPUT);

  digitalWrite(DIRM1, POSITIVE_DIR); digitalWrite(STPM1, LOW);
  digitalWrite(ENAM2, LOW); digitalWrite(DIRM2, POSITIVE_DIR); digitalWrite(PULM2, LOW);
  digitalWrite(ENAM3, LOW); digitalWrite(DIRM3, POSITIVE_DIR); digitalWrite(PULM3, LOW);

  Serial.println("Pas M1, M2 puis M3 (+ ou -), les trois moteurs bougent ensemble.");
  Serial.println("'p' = positions, 'z' = remise a zero.");
}

void loop() {
  String a = readLine("Pas M1 ?");
  if (handleCommand(a)) return;

  String b = readLine("Pas M2 ?");
  if (handleCommand(b)) return;

  String c = readLine("Pas M3 ?");
  if (handleCommand(c)) return;

  long s1 = a.toInt();
  long s2 = b.toInt();
  long s3 = c.toInt();

  Serial.print("Deplacement M1 : "); Serial.print(s1);
  Serial.print(" | M2 : ");          Serial.print(s2);
  Serial.print(" | M3 : ");          Serial.println(s3);
  moveAll(s1, s2, s3);
}