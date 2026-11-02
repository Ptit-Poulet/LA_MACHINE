// ============================================================
//  TEST DE MANETTE FLYSKY SUR ESP32 (protocole iBUS)
// ============================================================
//
//  Il lit tous les canaux de la manette (sticks, switchs, potards)
//  via UN SEUL fil, et affiche dans le moniteur série uniquement
//
//  CÂBLAGE :
//    Récepteur FS-iA10B, port "SERVO" (iBUS) :
//      Signal -> GPIO 16 de l'ESP32
//      +      -> 5V / VIN de l'ESP32
//      -      -> GND de l'ESP32
//
//  MONITEUR SÉRIE : régler la vitesse sur 115200 bauds.
// ============================================================

//  CANAUX (manette en mode 1) :
//    CH1 : stick droit,  gauche/droite
//    CH2 : stick gauche, haut/bas
//    CH3 : stick droit,  haut/bas
//    CH4 : stick gauche, gauche/droite
//    CH5 et suivants : switchs et potards (selon l'assignation
//    dans Functions setup -> Aux. channels)

// Broche de l'ESP32 qui reçoit le signal iBUS du récepteur.
#define IBUS_RX_PIN  16

// Nombre de canaux lus. Le protocole iBUS en transporte jusqu'à 14.
// Chaque stick, switch ou potard de la manette = 1 canal.
#define NUM_CHANNELS 14

// Variation minimale pour considérer qu'un canal a "changé".
// Les sticks et potards tremblent toujours un peu (quelques unités),
// sans ce seuil le moniteur afficherait des lignes en permanence.
//   - Plus petit (3-5)   = plus sensible
//   - Plus grand (15-20) = moins de lignes parasites
#define DEADBAND     8

// Tampon qui stocke la trame iBUS en cours de réception.
uint8_t  buf[32];

// Position actuelle dans le tampon (0 = on attend le début d'une trame).
uint8_t  idx = 0;

// Valeur actuelle de chaque canal (de ~1000 à ~2000).
// ch[0] = canal 1, ch[1] = canal 2, etc.
uint16_t ch[NUM_CHANNELS];

// Dernière valeur AFFICHÉE pour chaque canal.
// On la compare à la valeur actuelle pour savoir s'il y a eu un changement.
uint16_t lastShown[NUM_CHANNELS];

// Vrai tant qu'on n'a pas reçu la toute première trame valide.
bool     firstFrame = true;


// ------------------------------------------------------------
//  FONCTION : switchPos
//  Transforme une valeur (1000-2000) en position 0, 1 ou 2.
//  Pratique pour les switchs :
//    - switch 2 états : donne 0 ou 2
//    - switch 3 états : donne 0, 1 ou 2
// ------------------------------------------------------------
int switchPos(uint16_t v) {
  if (v < 1250) return 0;   // position basse  (valeur proche de 1000)
  if (v < 1750) return 1;   // position milieu (valeur proche de 1500)
  return 2;                 // position haute  (valeur proche de 2000)
}


// ------------------------------------------------------------
//  FONCTION : readIBus
//  Lit les octets reçus du récepteur et reconstitue une trame.
//  Retourne true quand une trame complète ET valide a été lue
//  (les valeurs des canaux sont alors mises à jour dans ch[]).
//
//  Format d'une trame iBUS (32 octets) :
//    octet 0      : 0x20 (début de trame)
//    octet 1      : 0x40 (début de trame)
//    octets 2-29  : 14 canaux, 2 octets chacun
//    octets 30-31 : checksum (contrôle d'erreur)
// ------------------------------------------------------------
bool readIBus() {

  // Tant qu'il y a des octets en attente dans la mémoire du port série...
  while (Serial2.available()) {

    // On lit un octet.
    uint8_t b = Serial2.read();

    // --- Synchronisation sur le début de trame ---
    // Le premier octet d'une trame doit être 0x20.
    // Sinon on ignore et on attend le suivant.
    if (idx == 0 && b != 0x20) continue;

    // Le deuxième octet doit être 0x40.
    // Sinon ce n'était pas une vraie trame : on recommence à zéro.
    if (idx == 1 && b != 0x40) { idx = 0; continue; }

    // L'octet est valide : on le range dans le tampon.
    buf[idx++] = b;

    // --- La trame est complète (32 octets reçus) ---
    if (idx == 32) {
      idx = 0;  // on se prépare pour la trame suivante

      // --- Vérification du checksum ---
      // On part de 0xFFFF et on soustrait chacun des 30 premiers octets.
      // Le résultat doit être égal aux 2 derniers octets de la trame.
      uint16_t sum = 0xFFFF;
      for (int i = 0; i < 30; i++) sum -= buf[i];

      // Les 2 derniers octets forment le checksum envoyé par le récepteur
      // (octet de poids faible en premier, c'est le "little endian").
      uint16_t rx = buf[30] | (buf[31] << 8);

      // Si les deux ne correspondent pas, la trame est corrompue : on la jette.
      if (sum != rx) continue;

      // --- Extraction des canaux ---
      // Chaque canal est stocké sur 2 octets (little endian) :
      // on assemble l'octet bas et l'octet haut pour obtenir la valeur.
      for (int i = 0; i < NUM_CHANNELS; i++) {
        ch[i] = buf[2 + i * 2] | (buf[3 + i * 2] << 8);
      }

      return true;  // trame valide, les canaux sont à jour
    }
  }

  return false;  // pas de trame complète pour l'instant
}

void setup() {

  // Port série vers l'ordinateur (moniteur série de l'Arduino IDE).
  Serial.begin(115200);

  // Port série n°2 de l'ESP32, qui écoute le récepteur FlySky.  
  // - SERIAL_8N1 : format standard (8 bits, pas de parité, 1 bit d'arrêt)
  // - IBUS_RX_PIN : broche de réception (RX)
  // - -1 : pas de broche d'émission (TX), on ne fait qu'écouter
  Serial2.begin(115200, SERIAL_8N1, IBUS_RX_PIN, -1);

  Serial.println("Test manette FlySky (iBUS) - affichage sur changement");
}

void loop() {

  // On essaie de lire une trame. Si aucune trame complète n'est
  // disponible (ou si elle est invalide), on sort et on réessaie.
  // Cela veut dire : rien n'est affiché si la manette est éteinte.
  if (!readIBus()) return;

  // --- Toute première trame reçue ---
  // On enregistre simplement l'état de départ de la manette comme
  // référence, sans l'afficher. Ce message confirme aussi que le
  // câblage et le pairage fonctionnent.
  if (firstFrame) {
    firstFrame = false;
    for (int i = 0; i < NUM_CHANNELS; i++) lastShown[i] = ch[i];
    Serial.println("Manette detectee. En attente de changements...");
    return;
  }

  // --- Détection des changements ---
  // On passe en revue chaque canal et on le compare à la dernière
  // valeur affichée.
  for (int i = 0; i < NUM_CHANNELS; i++) {

    // abs() = valeur absolue : la différence, qu'elle soit
    // positive ou négative. Si elle atteint DEADBAND, c'est un
    // vrai mouvement et non du bruit.
    if (abs((int)ch[i] - (int)lastShown[i]) >= DEADBAND) {

      // Affiche : numéro du canal, ancienne valeur, nouvelle valeur,
      // et la position 0/1/2 (utile surtout pour les switchs).
      // Exemple : "CH5  : 1000 -> 1500  (pos 1)"
      Serial.printf("CH%-2d : %4d -> %4d  (pos %d)\n",
                    i + 1, lastShown[i], ch[i], switchPos(ch[i]));

      // On mémorise cette valeur comme nouvelle référence, pour ne pas
      // ré-afficher tant que le canal ne bouge pas à nouveau.
      lastShown[i] = ch[i];
    }
  }
}