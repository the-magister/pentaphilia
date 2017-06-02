// compilation settings
// Board: Adafruit HUZZAH ESP8266, 80 Mhz, 4M (3M SPIFFS).
// See here for installation of the IDE: https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide

#include <Streaming.h>
#include <FastLED.h>
FASTLED_USING_NAMESPACE
extern "C" {
#include "user_interface.h"
}

// See: https://github.com/FastLED/FastLED/wiki/ESP8266-notes


/// pentaphilia dimensions
#define N_CONTROL 4 // number of (parallel) controllers
#define N_LED_CONTROL 20 // number of LEDs on a controller
#define N_LED N_CONTROL*N_LED_CONTROL // number of LEDs on a single data/clock pair

// that's a big malloc()
CRGB leds[N_LED];
// this does "work", per se, but we can achieve only ~16 fps
// if you double the leds, 8 fps.
// if you quadruple the leds, 4 fps.
// in either case, you want to segment to multiple controller pin pairs to increase fps.

/* 
 * better to segment the controllers, but do so in a way that makese sense for addressing and wiring: 
 *  
 *  Overhead view of a DP. "center" is where the pentagons meet; "left" and "right" are arbitary ways to
 *  denote a pentagon.
 *  
 *  left P===========center===========right P
 *             L                R
 *  left P===========center===========right P
 *             
 *             
 *  L and R are the strands installed on the left and right sides, respectively
 *  
 *  See: https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples for ideas on segmenting with
 *  multiple controllers
 */


// available blinkers on hardware
#define RED_LED 0
#define BLUE_LED 2

// Parallel output.  See https://github.com/FastLED/FastLED/wiki/Parallel-Output
// WS2811_PORTA - pins 12, 13, 14 and 15 (or pins 6,7,5 and 8 on the NodeMCU boards/pin layout).
#define PORT WS2811_PORTA

// general controls
#define BRIGHTNESS 255
#define COLOR_ORDER RGB


// rotating "base color" used by many of the patterns
uint8_t gHue = 0; 

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;
  Serial << F("Startup") << endl;
    
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  
  Serial << F("LED count: ") << N_LED << endl;
  Serial << F("Byte size of LED container: ") << sizeof(leds) << endl;
  
  Serial << F("Heap size: ") << system_get_free_heap_size() << endl;
  Serial << F("show_malloc():") << endl;
  system_show_malloc();

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<PORT, COLOR_ORDER>(leds, N_LED).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  p_all_one(CRGB::Red);
  delay(1000);
  p_all_one(CRGB::Green);
  delay(1000);
  p_all_one(CRGB::Blue);
  delay(1000);
  p_all_one(CRGB::Black);
  delay(1000);

}

void loop() {

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  // animation
  fill_rainbow(leds, N_LED, gHue, 1);
    
  // heartbeat
  static byte led_state = LOW;
  led_state = !led_state;
  digitalWrite(RED_LED, led_state);

  // Show our FPS
  EVERY_N_SECONDS( 10 ) {
    Serial << F("Reported FPS:") << FastLED.getFPS() << endl;
  }
}

void p_all_one(CRGB color) {
  fill_solid(leds, N_LED, color);
  FastLED.show();  
}

