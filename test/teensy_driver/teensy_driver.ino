// compilation settings
// Board: Teensy 3.2
// Compiler options: 
//   Fast=20 fps  (tested Faster=19 fps. Smallest Code=19 fps)
//   CPU Speed: 120 MHz (overclock)

#include <Streaming.h>

// need to turn interrupts off to get maximal fps.
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
FASTLED_USING_NAMESPACE

/*
 * WS2811/12/12b family operate at 800 kHz, so 30e-6 s to write one led.
 * if a "strand" is defined as 20 leds, and we have 125 DPs to light (124 DP gaps):
 * 1 strand/DP = 124*20*1 = 2480 leds * 30e-6 s/led = 0.0744 s = 13.4 fps.
 * Ergo, the updates are every ~75 ms, which humans can perceive as "choppy".
 * 
 * If we instead add a second controller in parallel, each handling 62 DPs,
 * our fps jumps to 26.9 with a 37.2 ms update time.
 * Ergo, the updates are every ~27 ms, which should look "smooth enough".
 * 
 * But, if we go up to 2 strands/DP or 4 strands/dp, we'll need 4 or 8 controllers.
 * Let's plan for 8 controllers (worst-case).
 */

// See examples/ParallelOutputDemo for parallel output setup
// Note that the OctoWS2811 library can be added for (reported) speedups,
//   but I couldn't get a responsive uC after trying that.  YMMV.
// Pin layouts on the teensy 3/3.1:
// WS2811_PORTD: 2,14,7,8,6,20,21,5 // 8 lines
#define PORT WS2811_PORTD // makes 8 strips available, easily

#define N_CONTROL_PROJECT   8   // 8 controllers
#define N_STRIP_CONTROL     62  // 62 gaps to light per controller
#define N_LED_STRIP         20  // 20 led modules per strip
// and derive
#define N_LED_CONTROL       N_STRIP_CONTROL * N_LED_STRIP
#define N_LED_PROJECT       N_CONTROL_PROJECT * N_LED_CONTROL

// top fps possible, by protocol
float updateTime = N_LED_CONTROL * 30e-6 * 1000.0;
float theoreticalFPS = 1.0 / (N_LED_CONTROL * 30e-6);

// that's a big malloc()
// See: https://github.com/FastLED/FastLED/wiki/RGBSet-Reference
CRGBArray<N_LED_PROJECT> leds;

// general controls
byte masterBrightness = 255;
#define COLOR_ORDER RGB
#define COLOR_CORRECTION TypicalLEDStrip

const int ledPin = 13;

void setup() {
  // initialize the digital pin as an output.
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(5000);
  digitalWrite(ledPin, LOW);
  
  Serial.begin(115200);
  Serial << endl << endl;
  Serial << F("Startup begins.") << endl;
     
  Serial << F("Total LED count: ") << N_LED_PROJECT << endl;
  Serial << F("Byte size of LED container: ") << sizeof(leds) << endl;

  Serial << F("LEDs per controller: ") << N_LED_CONTROL << endl;
  Serial << F("Update time per controller (ms): ") << updateTime << endl;
  Serial << F("Theoretical fps: ") << theoreticalFPS << endl;
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<PORT, N_CONTROL_PROJECT, COLOR_ORDER>(leds, N_LED_CONTROL).setCorrection(COLOR_CORRECTION);
  // set master brightness control
  FastLED.setBrightness(masterBrightness);

  unsigned long tic, toc;

  leds.fill_solid(CRGB::Red);
  tic = millis();
  FastLED.show();
  toc = millis();
  Serial << F("Strip update (ms) actual=") << toc - tic << F(" theoretical=") << updateTime << endl;
  FastLED.delay(1000);
  
  leds.fill_solid(CRGB::Green);
  tic = millis();
  FastLED.show();
  toc = millis();
  Serial << F("strip update (ms) actual=") << toc - tic << F(" theoretical=") << updateTime<< endl;
  FastLED.delay(1000);

  leds.fill_solid(CRGB::Blue);
  tic = millis();
  FastLED.show();
  toc = millis();
  Serial << F("strip update (ms) actual=") << toc - tic << F(" theoretical=") << updateTime<< endl;
  FastLED.delay(1000);

  FastLED.clear();
  FastLED.show();

  Serial << F("Startup complete.") << endl;

}

void loop() {
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  

  // do some periodic updates
  static byte hue = 0;
  EVERY_N_MILLISECONDS( 20 ) { 
    // slowly cycle the "base color" through the rainbow
    hue++; 
  } 

  // animation
  leds.fill_rainbow(hue,1);
    
  // Show our FPS
  static boolean ledState = false;
  EVERY_N_SECONDS( 2 ) {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    Serial << F("FPS actual=") << FastLED.getFPS() << F(" theoretical=") << theoreticalFPS << endl;
  }
}

