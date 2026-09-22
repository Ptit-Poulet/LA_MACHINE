#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>

// --- PN532 Setup ---
#define PN532_IRQ   -1
#define PN532_RESET 4
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// --- LED Setup ---
#define LED_PIN    12
#define LED_COUNT  3
Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);  // <— try NEO_GRB if colors are wrong

// --- NFC memory reading config ---
const uint8_t START_PAGE = 4;
const uint8_t MAX_PAGES_TO_READ = 45;
const size_t MAX_BYTES = MAX_PAGES_TO_READ * 4;

uint8_t allBytes[MAX_BYTES];
uint8_t payloadBuf[200];

// --- Color table ---
struct ColorMap {
  const char* names[15];
  uint8_t r, g, b;
};

ColorMap colors[] = {
  {{"rouge","red","rojo","vermelho","rosso","roșu","kırmızı","i kuq","rood","rot","wenhta’ ïohtih","nyekundu","wouj","merah","ʻulaʻula"}, 255,0,0},
  {{"jaune","yellow","amarillo","amarelo","giallo","galben","sarı","i verdhë","geel","gelb","Atiaren’ta’ ïohtih","njano","jòn","kuning","melemele"}, 255,255,0},
  {{"bleu","blue","azul","azul","blu","albastru","mavi","blu","blauw","blau","Yaronhia’ ïohtih","bluu","ble","biru","polū"}, 0,0,255}
};

// --- Keep track of LED states ---
uint8_t ledColors[LED_COUNT][3];  // r,g,b for each LED

void applyLEDColors() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds.setPixelColor(i, leds.Color(ledColors[i][0], ledColors[i][1], ledColors[i][2]));
  }
  leds.show();
}

// --- Extract color from text ---
bool extractColor(String text, uint8_t &r, uint8_t &g, uint8_t &b) {
  text.toLowerCase();
  for (int i = 0; i < sizeof(colors)/sizeof(colors[0]); i++) {
    for (int j = 0; j < 15; j++) {
      if (colors[i].names[j] && text.indexOf(colors[i].names[j]) != -1) {
        r = colors[i].r;
        g = colors[i].g;
        b = colors[i].b;
        return true;
      }
    }
  }
  if (text.startsWith("#") && text.length() >= 7) {
    r = strtol(text.substring(1,3).c_str(),NULL,16);
    g = strtol(text.substring(3,5).c_str(),NULL,16);
    b = strtol(text.substring(5,7).c_str(),NULL,16);
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21,22);
  leds.begin();
  leds.show();
  memset(ledColors, 0, sizeof(ledColors)); // initialize all to off

  if (!nfc.begin()) {
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

  // Read memory pages
  int len = 0;
  uint8_t pageBuf[4];
  for (uint8_t p = START_PAGE; p < START_PAGE + MAX_PAGES_TO_READ; p++) {
    if (!nfc.ntag2xx_ReadPage(p, pageBuf)) break;
    for (int i = 0; i < 4; i++) allBytes[len++] = pageBuf[i];
  }

  // Find NDEF TLV
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

  Serial.print("Payload: ");
  Serial.println(text);

  // --- Determine block (A/B/C) ---
  int ledIndex = -1;
  if (text.indexOf("A") != -1) ledIndex = 0;
  if (text.indexOf("B") != -1) ledIndex = 1;
  if (text.indexOf("C") != -1) ledIndex = 2;

  // --- Extract color ---
  uint8_t r,g,b;
  if (ledIndex != -1 && extractColor(text,r,g,b)) {
    ledColors[ledIndex][0] = r;
    ledColors[ledIndex][1] = g;
    ledColors[ledIndex][2] = b;
    applyLEDColors();
    Serial.println("Block + Color applied!");
  } else {
    Serial.println("No valid block or color.");
  }

  delay(1000);
}
