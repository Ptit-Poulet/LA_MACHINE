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
  progressiveLighting(500);  // 500ms delay between lighting up each LED
}

// Function to progressively light up each LED with specified colors
void progressiveLighting(int delayTime) {
  // Define colors for each LED: red, green, blue, yellow, purple
  uint32_t colors[NUM_LEDS] = {
    strip.Color(255, 0, 0),    // Red
    strip.Color(0, 255, 0),    // Green
    strip.Color(0, 0, 255),    // Blue
    strip.Color(255, 255, 0),  // Yellow
    strip.Color(0, 128, 128)   // Purple --------------------------------------------attention led marche pas bien (G,R, B) aulieu de (R,G,B)
  };
  
  // Light up each LED with its assigned color, one by one
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, colors[i]);
    strip.show();
    delay(delayTime);  // Wait before lighting the next LED
  }
  
  // Optionally, turn all LEDs off after they have been lit
  delay(2000);  // Keep LEDs on for 2 seconds before turning off
  strip.clear();
  strip.show();
  delay(1000);  // Wait for 1 second before restarting the loop
}
