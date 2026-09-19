//Lecture NFC tag (module v3 chip #PN532) pour allumer des LEDs

/*Informations branchement et + :
 Branchement SDA -> A4
 Branchement SCL -> A5
 Module prend 5v
*/

#include <Wire.h>
#include <Adafruit_PN532.h>

//Simplement pour initialiser nfc tag
#define PN532_IRQ   (2) 
#define PN532_RESET (3) 
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

//A voir si on utilise le NEO_Pixel comme l'an passé
#define LED_RED   10
#define LED_BLUE  11
#define LED_GREEN 12

// --- Lecture mémoire NTAG ---
const uint8_t START_PAGE = 4;
const uint8_t MAX_PAGES_TO_READ = 45;
const size_t MAX_BYTES = MAX_PAGES_TO_READ * 4;
uint8_t allBytes[MAX_BYTES];

String currentColor = "";

void setup() {
  Serial.begin(9600);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, LOW);

  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not found!");
    while (1);
  }
  nfc.SAMConfig();
  Serial.println("Ready to scan NFC tag...");
}

void loop() {
  uint8_t uid[7], uidLength;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    return;
  }

  // Lecture des pages mémoire du tag
  int len = 0;
  uint8_t pageBuf[4];
  for (uint8_t p = START_PAGE; p < START_PAGE + MAX_PAGES_TO_READ; p++) {
    if (!nfc.ntag2xx_ReadPage(p, pageBuf)) break;
    for (int i = 0; i < 4; i++) allBytes[len++] = pageBuf[i];
  }

  // Cherche le TLV NDEF
  int tlvIndex = -1;
  for (int i = 0; i < len; i++) {
    if (allBytes[i] == 0x03) { tlvIndex = i; break; }
  }
  if (tlvIndex < 0) return;

  int idx = tlvIndex + 1;
  int payloadLen = allBytes[idx++];

  String text = "";
  for (int i = 0; i < payloadLen; i++) {
    char c = allBytes[idx + i];
    if (c >= 32 && c <= 126) text += c;
  }
  text.toLowerCase();

  Serial.print("Payload: ");
  Serial.println(text);

  String color = "";
  if (text.indexOf("red") != -1)        color = "red";
  else if (text.indexOf("blue") != -1)  color = "blue";
  else if (text.indexOf("green") != -1) color = "green";

  if (color != "") {
    setColor(color);
  } else {
    Serial.println("Couleur non reconnue.");
  }

  delay(500); // anti-rebond
}

void setColor(String color) {
  if (color == currentColor) return; // déjà allumée

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, LOW);

  if (color == "red")   digitalWrite(LED_RED, HIGH);
  if (color == "blue")  digitalWrite(LED_BLUE, HIGH);
  if (color == "green") digitalWrite(LED_GREEN, HIGH);

  currentColor = color;
  Serial.println("LED: " + color);
}

