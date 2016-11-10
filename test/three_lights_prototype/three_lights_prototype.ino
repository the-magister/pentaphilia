#include <Streaming.h>
#include <Metro.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

// 30mm 12V modules
#define DATA_PIN_30mm    7
#define LED_TYPE_30mm    WS2812
#define COLOR_ORDER_30mm RGB
#define N_LED_30mm    14
CRGB leds_30mm[N_LED_30mm];


// 45mm 12V modules
#define DATA_PIN_45mm    3
#define CLOCK_PIN_45mm    4
#define LED_TYPE_45mm    WS2801
#define COLOR_ORDER_45mm RGB
#define N_LED_45mm    14
CRGB leds_45mm[N_LED_45mm];

// general controls
#define FRAMES_PER_SECOND   20
#define BRIGHTNESS          255

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void setup() {
  Serial.begin(115200);
  Serial << F("Startup.") << endl;

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE_30mm, DATA_PIN_30mm, COLOR_ORDER_30mm>(leds_30mm, N_LED_30mm).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE_45mm, DATA_PIN_45mm, CLOCK_PIN_45mm, COLOR_ORDER_45mm>(leds_45mm, N_LED_45mm).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  fill_solid(leds_30mm, N_LED_30mm, CRGB::Red);
  fill_solid(leds_45mm, N_LED_45mm, CRGB::Red);
  FastLED.show();  
  delay(1000);
  fill_solid(leds_30mm, N_LED_30mm, CRGB::Green);
  fill_solid(leds_45mm, N_LED_45mm, CRGB::Green);
  FastLED.show();  
  delay(1000);
  fill_solid(leds_30mm, N_LED_30mm, CRGB::Blue);
  fill_solid(leds_45mm, N_LED_45mm, CRGB::Blue);
  FastLED.show();  
  delay(1000);

}

void loop() {

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  rainbow();
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds_30mm, N_LED_30mm, gHue, 7);
  fill_rainbow( leds_45mm, N_LED_45mm, gHue, 7);
}
