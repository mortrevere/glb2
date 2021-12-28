#include <FastLED.h>
#define NUM_LEDS 240 //the number of LEDS we have in total
CRGBArray<NUM_LEDS> leds; //an array of LEDs, used by FastLED to set colors on each of them
#define ONBOARD_LED  2 //the ID of the onboard LED

//defines the current color for the globe
CRGB currentColor;

void setup() {
  currentColor = CRGB(255, 0, 0);
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, 25>(leds, NUM_LEDS);
  FastLED.show();
}

unsigned char dimmer = 0;
unsigned int counter = 0;
unsigned char prev_buffer[16] = {0};
void loop() {
  counter++;
  leds.fadeToBlackBy(50);
  leds[random(0, NUM_LEDS - 1)] = currentColor;
  FastLED.delay(10);

}
