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
 * But, if we go up to 2 strands/DP or 4 strands/dp, we'll need 4 or 8 or 16 controllers.
 * Let's plan for 16 controllers (worst-case).
 */

// See examples/ParallelOutputDemo for parallel output setup
// Note that the OctoWS2811 library can be added for (reported) speedups,
//   but I couldn't get a responsive uC after trying that.  YMMV.
// Pin layouts on the teensy 3/3.1:
// WS2811_PORTD: 2,14,7,8,6,20,21,5
// WS2811_PORTC: 15,22,23,9,10,13,11,12,28,27,29,30 (these last 4 are pads on the bottom of the teensy)
// WS2811_PORTDC: 2,14,7,8,6,20,21,5,15,22,23,9,10,13,11,12 - 16 way parallel
#define PORT WS2811_PORTDC // makes 16 controllers available, easily
// MGD: note, I only wired up 2,14,7,8 to a voltage shifter

/*
 * Simplest way to wire these up would be for each controller (N=4) to be assigned one face of a P:
 * 
 *  /-C0              /-C2        
 * | =============   | =============    <- looking down (plan view) on DP0
 *  \          C1-\  \           C3-\ 
 *  /-C0           |  /-C2           |
 * | ============= | | ============= |  <- looking down (plan view) on DP1
 * |           C1-/  |           C3-/
 * |              |  |              |
 * 
 *             ...etc....
 */

#define N_CONTROL_PROJECT   4   // controller count
#define N_STRIP_CONTROL     25  // 25 P's per controller
#define N_LED_STRIP         20  // 20 led modules per P
// and derive
#define N_LED_CONTROL       N_STRIP_CONTROL * N_LED_STRIP
#define N_LED_PROJECT       N_CONTROL_PROJECT * N_LED_CONTROL

// top fps possible, by protocol
float theoreticalUpdate = N_LED_CONTROL * 30e-6 * 1000.0;
float theoreticalFPS = 1.0 / (N_LED_CONTROL * 30e-6);

/* 
 * if you had 125 DPs, and 4x 20 LED strips per DP, you'd have 10,000 LEDS.
 * with 16 controllers, that's 625 LED/controller.
 * 1/(625*30e-6) = 53 fps.  
 *
 */
 
// going to follow the CRGBSet development docs to get abstracted sections of the led array
// that map to physical objects using container classes.
//
// See: https://github.com/FastLED/FastLED/wiki/RGBSet-Reference
CRGBArray<N_LED_PROJECT> leds;
// that's a big malloc()

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
  Serial << F("Update time per controller (ms): ") << theoreticalUpdate << endl;
  Serial << F("Theoretical fps: ") << theoreticalFPS << endl;
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<PORT, N_CONTROL_PROJECT, COLOR_ORDER>(leds, N_LED_CONTROL).setCorrection(COLOR_CORRECTION);
  // set master brightness control
  FastLED.setBrightness(masterBrightness);
  // should probably set this to a ceiling that you want that's below the theoretic limit.
  // that way, the code will delay() appropriately if computation is fast, and then
  // slow down delays as the code base gets slower, without changing the look of the animations.
  FastLED.setMaxRefreshRate(30);
  
  unsigned long tic, toc;

  leds.fill_solid(CRGB::Red);
  tic = millis();
  FastLED.show();
  toc = millis();
  Serial << F("Strip update (ms) actual=") << toc - tic << F(" theoretical=") << theoreticalUpdate << endl;
  FastLED.delay(1000);
  
  leds.fill_solid(CRGB::Green);
  tic = millis();
  FastLED.show();
  toc = millis();
  Serial << F("strip update (ms) actual=") << toc - tic << F(" theoretical=") << theoreticalUpdate<< endl;
  FastLED.delay(1000);

  leds.fill_solid(CRGB::Blue);
  tic = millis();
  FastLED.show();
  toc = millis();
  Serial << F("strip update (ms) actual=") << toc - tic << F(" theoretical=") << theoreticalUpdate<< endl;
  FastLED.delay(1000);

  FastLED.clear();
  FastLED.show();

  Serial << F("Startup complete.") << endl;

}

void loop() {
  // send the 'leds' array out to the actual LED strip
  static word updates = 0;
  FastLED.show();  
  updates++;

  // do some periodic updates
  static byte hue = 0;
//  leds.fill_rainbow(hue++, 1);
  for(CRGB & pixel : leds) { pixel = CHSV(hue++,240,255); }
  
  // Show our FPS
  static boolean ledState = false;
  word reportInterval = 5;
  EVERY_N_SECONDS( reportInterval ) {
    // toggle LED
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    
    float reportedFPS = FastLED.getFPS();
    float actualFPS = (float)updates/(float)reportInterval;
    updates = 0;   
    float actualUpdate = 1.0/actualFPS * 1000.0;
    
    Serial << F("Metric (actual/theoretical).  FPS, Hz (") << actualFPS << F("/") << theoreticalFPS << F("=") << actualFPS/theoreticalFPS;
    Serial << F(").  Update, ms (") << actualUpdate << F("/") << theoreticalUpdate << F("=") << actualUpdate/theoreticalUpdate << F(").") << endl;
  }
}

