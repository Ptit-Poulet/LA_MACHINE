//Lecture NFC tag (module v3 chip #PN532) pour allumer des LEDs

/*Informations branchement et + :
 Branchement SDA -> A4
 Branchement SCL -> A5

 RSTQ -> D3
 IRQ -> D2
 Module prend 5v

Led green wire (WS2811) -> D5 With 5v (RED)
*/

#include <Wire.h>
#include <Adafruit_PN532.h>
#include <FastLED.h>

// LED setup
#define LED_PIN 5
#define NUM_LEDS 1  // Nombre de LED

#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

//Branchement physique seulement, s'assurerr que module switch sur I2C
#define PN532_IRQ   (2) 
#define PN532_RESET (3) 
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// --- Lecture mémoire NTAG ---
const uint8_t START_PAGE = 4;
const uint8_t MAX_PAGES_TO_READ = 45;
const size_t MAX_BYTES = MAX_PAGES_TO_READ * 4;
uint8_t allBytes[MAX_BYTES];

String currentColor = "";

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

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

String color = "";
if (text.indexOf("rouge") != -1 || text.indexOf("red") != -1)        color = "red";
else if (text.indexOf("bleu") != -1 || text.indexOf("blue") != -1)   color = "blue";
else if (text.indexOf("vert") != -1 || text.indexOf("green") != -1)  color = "green";

  if (color != "") {
    setColor(color);
  } else {
    Serial.println("Couleur non reconnue.");
  }

  delay(150); // anti-rebond
}

void setColor(String color) {
  if (color == currentColor) return; // déjà allumée

  CRGB c = CRGB::Black;
  if (color == "red")   c = CRGB::Red;
  if (color == "blue")  c = CRGB::Blue;
  if (color == "green") c = CRGB::Green;

  fill_solid(leds, NUM_LEDS, c);
  FastLED.show();

  currentColor = color;
  Serial.println("LED: " + color);
}

