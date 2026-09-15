#include <Adafruit_NeoPixel.h>

// Define the pin for the data line
#define PIN 6 

// Define the number of LEDs in the strip
#define NUM_LEDS 5     

// Create the NeoPixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Initialize the LED strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  blinkLeds(255, 0, 0, 500);  // Red color, 500ms delay
  blinkLeds(0, 255, 0, 500);  // Green color, 500ms delay
  blinkLeds(0, 0, 255, 500);  // Blue color, 500ms delay
}

void blinkLeds(uint8_t red, uint8_t green, uint8_t blue, int delayTime) {
  // Turn all LEDs on with the specified color
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(red, green, blue));
  }
  strip.show();
  delay(delayTime); // Wait for the specified time

  // Turn all LEDs off
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  delay(delayTime); // Wait for the specified time
}
