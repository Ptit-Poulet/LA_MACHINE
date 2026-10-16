// test Nema 23 avec TB6600
// SW1 ON, SW2 ON, SW3 OFF, SW4 OFF, SW5 OFF, SW6 ON 

const int PUL = 25;
const int DIR = 26;
const int ENA = 27;

const long STEPS_PER_REV = 2000; 
const int PULSE_US = 425;        // demi-période : 1000 = ~150 tr/min. Baisse à 500 pour aller plus vite
const bool POSITIVE_DIR = LOW;    // mets HIGH si le sens est inversé

long position = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PUL, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(ENA, OUTPUT);

  digitalWrite(ENA, LOW);
  digitalWrite(DIR, POSITIVE_DIR);
  digitalWrite(PUL, LOW);

  Serial.println("Entre un nombre de pas (+ ou -). 'p' = position, 'z' = remise a zero.");
}

void moveSteps(long steps) {
  if (steps == 0) return;

  while (Serial.available()) Serial.read();   // vide le buffer avant de bouger

  bool positive = steps > 0;
  digitalWrite(DIR, positive ? POSITIVE_DIR : !POSITIVE_DIR);
  delayMicroseconds(20);

  long n = labs(steps);
  for (long i = 0; i < n; i++) {
    digitalWrite(PUL, HIGH);
    delayMicroseconds(PULSE_US);
    digitalWrite(PUL, LOW);
    delayMicroseconds(PULSE_US);

    position += positive ? 1 : -1;

    if (i > 0 && (i % 50) == 0 && Serial.available()) {
      while (Serial.available()) Serial.read();
      Serial.println("Arret !");
      break;
    }
  }
  Serial.print("Position : ");
  Serial.println(position);
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line == "p") {
      Serial.print("Position : ");
      Serial.println(position);
    } else if (line == "z") {
      position = 0;
      Serial.println("Position remise a 0");
    } else {
      long steps = line.toInt();
      Serial.print("Deplacement : ");
      Serial.println(steps);
      moveSteps(steps);
    }
  }
}