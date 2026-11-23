// Détection d'aimant (QMC5883P, 0x2C) -> LED flash rouge/vert/bleu sur D6
// SDA -> A4, SCL -> A5, LED data -> D6

#include <Wire.h>
#include <FastLED.h>

#define LED_PIN     6
#define NUM_LEDS    1
#define BRIGHTNESS  64
#define LED_TYPE    WS2812     // ou WS2811
#define COLOR_ORDER RGB        // si les couleurs sont fausses, essaie GRB

#define MAG_ADDR    0x2C
#define THRESHOLD   2000       // sensibilité : plus haut = moins sensible

CRGB leds[NUM_LEDS];
float bx, by, bz;
bool magnetPresent = false;

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MAG_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void readMag(float &x, float &y, float &z) {
  uint8_t b[6] = {0};
  Wire.beginTransmission(MAG_ADDR);
  Wire.write(0x01);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)MAG_ADDR, (uint8_t)6);
  for (uint8_t i = 0; i < 6 && Wire.available(); i++) b[i] = Wire.read();
  x = (int16_t)((b[1] << 8) | b[0]);
  y = (int16_t)((b[3] << 8) | b[2]);
  z = (int16_t)((b[5] << 8) | b[4]);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(500);

  Wire.beginTransmission(MAG_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("Capteur non trouve !");
    while (1);
  }
  writeReg(0x0B, 0x08);   // plage +/-8 gauss
  writeReg(0x0A, 0xCF);   // mode continu, 200 Hz

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Référence : champ ambiant (ne pas approcher d'aimant au démarrage)
  delay(200);
  float sx = 0, sy = 0, sz = 0, x, y, z;
  for (int i = 0; i < 30; i++) {
    readMag(x, y, z);
    sx += x; sy += y; sz += z;
    delay(30);
  }
  bx = sx / 30; by = sy / 30; bz = sz / 30;
}

void loop() {
  float x, y, z;
  readMag(x, y, z);
  float dx = x - bx, dy = y - by, dz = z - bz;
  float delta = sqrt(dx * dx + dy * dy + dz * dz);

  // Hystérésis pour éviter le scintillement
  if (!magnetPresent && delta > THRESHOLD) magnetPresent = true;
  else if (magnetPresent && delta < THRESHOLD * 0.6) magnetPresent = false;

  // Flash : allumé 150 ms, éteint 150 ms, couleur différente à chaque flash
  bool on = magnetPresent && ((millis() / 150) % 2 == 0);
  uint8_t i = (millis() / 300) % 3;
  CRGB color = (i == 0) ? CRGB::Red : (i == 1) ? CRGB::Green : CRGB::Blue;

  fill_solid(leds, NUM_LEDS, on ? color : CRGB::Black);
  FastLED.show();

  // Serial.println(delta);   // décommente pour régler la sensibilité

  delay(50);
}