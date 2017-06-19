// compilation settings
// Board: Teensy 3.2
// Compiler options: 
//   Optimize: "Faster"
//   CPU Speed: 96 MHz (Overclock)

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
// container for all the LEDs, but not very useful in this form
CRGBArray<N_LED_PROJECT> leds;
// that's a big malloc()
// let's make some useful containers that map to the physical layout
CRGBSet C0( leds(0*N_LED_CONTROL, 1*N_LED_CONTROL-1) );
CRGBSet C0r = C0( C0.size()-1, 0 ); // reversed version.  other forms of "reverse" defined in pixelsets.h are garbage and don't work well.  YMMV.
CRGBSet C1( leds(1*N_LED_CONTROL, 2*N_LED_CONTROL-1) );
CRGBSet C1r = C1( C1.size()-1, 0 );
CRGBSet C2( leds(2*N_LED_CONTROL, 3*N_LED_CONTROL-1) );
CRGBSet C2r = C2( C2.size()-1, 0 );
CRGBSet C3( leds(3*N_LED_CONTROL, 4*N_LED_CONTROL-1) );
CRGBSet C3r = C3( C3.size()-1, 0 );
// etc.  these are used in showPalleteBackground().

// my advice, basically, is to create a CRGBSet that eases implementation of
// each animation you want to run (aka "sugar").  

// the first face, using C0 and C2, accessing C2 in reverse so acessing an led gets the "opposite" side of the clock
CRGBSet Face0d[2] = { C0( 0*N_LED_STRIP, 1*N_LED_STRIP-1 ), C2( 1*N_LED_STRIP-1, 0*N_LED_STRIP ) };
// I could keep defining these...
CRGBSet Face1f[2] = { C0( 1*N_LED_STRIP, 2*N_LED_STRIP-1 ), C2( 2*N_LED_STRIP-1, 1*N_LED_STRIP ) };
// ...etc....
// backside of the first face,
CRGBSet Face0b[2] = { C1( 0*N_LED_STRIP, 1*N_LED_STRIP-1 ), C3( 1*N_LED_STRIP-1, 0*N_LED_STRIP ) };
// ...etc...

// Or I could "go for the throat".  This is used in showTracerBore() and showPalleteRainbow().
#define N_FACE_DP 2
#define N_P_DP 2
CRGBSet Face[N_STRIP_CONTROL][N_FACE_DP][N_P_DP] = {
  { { C0( 0 *N_LED_STRIP, 1 *N_LED_STRIP-1 ), C2( 1 *N_LED_STRIP-1, 0 *N_LED_STRIP ) }, { C1( 0 *N_LED_STRIP, 1 *N_LED_STRIP-1 ), C2( 1 *N_LED_STRIP-1, 0 *N_LED_STRIP ) } },
  { { C0( 1 *N_LED_STRIP, 2 *N_LED_STRIP-1 ), C2( 2 *N_LED_STRIP-1, 1 *N_LED_STRIP ) }, { C1( 1 *N_LED_STRIP, 2 *N_LED_STRIP-1 ), C3( 2 *N_LED_STRIP-1, 1 *N_LED_STRIP ) } },
  { { C0( 2 *N_LED_STRIP, 3 *N_LED_STRIP-1 ), C2( 3 *N_LED_STRIP-1, 2 *N_LED_STRIP ) }, { C1( 2 *N_LED_STRIP, 3 *N_LED_STRIP-1 ), C3( 3 *N_LED_STRIP-1, 2 *N_LED_STRIP ) } },
  { { C0( 3 *N_LED_STRIP, 4 *N_LED_STRIP-1 ), C2( 4 *N_LED_STRIP-1, 3 *N_LED_STRIP ) }, { C1( 3 *N_LED_STRIP, 4 *N_LED_STRIP-1 ), C3( 4 *N_LED_STRIP-1, 3 *N_LED_STRIP ) } },
  { { C0( 4 *N_LED_STRIP, 5 *N_LED_STRIP-1 ), C2( 5 *N_LED_STRIP-1, 4 *N_LED_STRIP ) }, { C1( 4 *N_LED_STRIP, 5 *N_LED_STRIP-1 ), C3( 5 *N_LED_STRIP-1, 4 *N_LED_STRIP ) } },
  { { C0( 5 *N_LED_STRIP, 6 *N_LED_STRIP-1 ), C2( 6 *N_LED_STRIP-1, 5 *N_LED_STRIP ) }, { C1( 5 *N_LED_STRIP, 6 *N_LED_STRIP-1 ), C3( 6 *N_LED_STRIP-1, 5 *N_LED_STRIP ) } },
  { { C0( 6 *N_LED_STRIP, 7 *N_LED_STRIP-1 ), C2( 7 *N_LED_STRIP-1, 6 *N_LED_STRIP ) }, { C1( 6 *N_LED_STRIP, 7 *N_LED_STRIP-1 ), C3( 7 *N_LED_STRIP-1, 6 *N_LED_STRIP ) } },
  { { C0( 7 *N_LED_STRIP, 8 *N_LED_STRIP-1 ), C2( 8 *N_LED_STRIP-1, 7 *N_LED_STRIP ) }, { C1( 7 *N_LED_STRIP, 8 *N_LED_STRIP-1 ), C3( 8 *N_LED_STRIP-1, 7 *N_LED_STRIP ) } },
  { { C0( 8 *N_LED_STRIP, 9 *N_LED_STRIP-1 ), C2( 9 *N_LED_STRIP-1, 8 *N_LED_STRIP ) }, { C1( 8 *N_LED_STRIP, 9 *N_LED_STRIP-1 ), C3( 9 *N_LED_STRIP-1, 8 *N_LED_STRIP ) } },
  { { C0( 9 *N_LED_STRIP, 10*N_LED_STRIP-1 ), C2( 10*N_LED_STRIP-1, 9 *N_LED_STRIP ) }, { C1( 9 *N_LED_STRIP, 10*N_LED_STRIP-1 ), C3( 10*N_LED_STRIP-1, 9 *N_LED_STRIP ) } },
  { { C0( 10*N_LED_STRIP, 11*N_LED_STRIP-1 ), C2( 11*N_LED_STRIP-1, 10*N_LED_STRIP ) }, { C1( 10*N_LED_STRIP, 11*N_LED_STRIP-1 ), C3( 11*N_LED_STRIP-1, 10*N_LED_STRIP ) } },
  { { C0( 11*N_LED_STRIP, 12*N_LED_STRIP-1 ), C2( 12*N_LED_STRIP-1, 11*N_LED_STRIP ) }, { C1( 11*N_LED_STRIP, 12*N_LED_STRIP-1 ), C3( 12*N_LED_STRIP-1, 11*N_LED_STRIP ) } },
  { { C0( 12*N_LED_STRIP, 13*N_LED_STRIP-1 ), C2( 13*N_LED_STRIP-1, 12*N_LED_STRIP ) }, { C1( 12*N_LED_STRIP, 13*N_LED_STRIP-1 ), C3( 13*N_LED_STRIP-1, 12*N_LED_STRIP ) } },
  { { C0( 13*N_LED_STRIP, 14*N_LED_STRIP-1 ), C2( 14*N_LED_STRIP-1, 13*N_LED_STRIP ) }, { C1( 13*N_LED_STRIP, 14*N_LED_STRIP-1 ), C3( 14*N_LED_STRIP-1, 13*N_LED_STRIP ) } },
  { { C0( 14*N_LED_STRIP, 15*N_LED_STRIP-1 ), C2( 15*N_LED_STRIP-1, 14*N_LED_STRIP ) }, { C1( 14*N_LED_STRIP, 15*N_LED_STRIP-1 ), C3( 15*N_LED_STRIP-1, 14*N_LED_STRIP ) } },
  { { C0( 15*N_LED_STRIP, 16*N_LED_STRIP-1 ), C2( 16*N_LED_STRIP-1, 15*N_LED_STRIP ) }, { C1( 15*N_LED_STRIP, 16*N_LED_STRIP-1 ), C3( 16*N_LED_STRIP-1, 15*N_LED_STRIP ) } },
  { { C0( 16*N_LED_STRIP, 17*N_LED_STRIP-1 ), C2( 17*N_LED_STRIP-1, 16*N_LED_STRIP ) }, { C1( 16*N_LED_STRIP, 17*N_LED_STRIP-1 ), C3( 17*N_LED_STRIP-1, 16*N_LED_STRIP ) } },
  { { C0( 17*N_LED_STRIP, 18*N_LED_STRIP-1 ), C2( 18*N_LED_STRIP-1, 17*N_LED_STRIP ) }, { C1( 17*N_LED_STRIP, 18*N_LED_STRIP-1 ), C3( 18*N_LED_STRIP-1, 17*N_LED_STRIP ) } },
  { { C0( 18*N_LED_STRIP, 19*N_LED_STRIP-1 ), C2( 19*N_LED_STRIP-1, 18*N_LED_STRIP ) }, { C1( 18*N_LED_STRIP, 19*N_LED_STRIP-1 ), C3( 19*N_LED_STRIP-1, 18*N_LED_STRIP ) } },
  { { C0( 19*N_LED_STRIP, 20*N_LED_STRIP-1 ), C2( 20*N_LED_STRIP-1, 19*N_LED_STRIP ) }, { C1( 19*N_LED_STRIP, 20*N_LED_STRIP-1 ), C3( 20*N_LED_STRIP-1, 19*N_LED_STRIP ) } },
  { { C0( 20*N_LED_STRIP, 21*N_LED_STRIP-1 ), C2( 21*N_LED_STRIP-1, 20*N_LED_STRIP ) }, { C1( 20*N_LED_STRIP, 21*N_LED_STRIP-1 ), C3( 21*N_LED_STRIP-1, 20*N_LED_STRIP ) } },
  { { C0( 21*N_LED_STRIP, 22*N_LED_STRIP-1 ), C2( 22*N_LED_STRIP-1, 21*N_LED_STRIP ) }, { C1( 21*N_LED_STRIP, 22*N_LED_STRIP-1 ), C3( 22*N_LED_STRIP-1, 21*N_LED_STRIP ) } },
  { { C0( 22*N_LED_STRIP, 23*N_LED_STRIP-1 ), C2( 23*N_LED_STRIP-1, 22*N_LED_STRIP ) }, { C1( 22*N_LED_STRIP, 23*N_LED_STRIP-1 ), C3( 23*N_LED_STRIP-1, 22*N_LED_STRIP ) } },
  { { C0( 23*N_LED_STRIP, 24*N_LED_STRIP-1 ), C2( 24*N_LED_STRIP-1, 23*N_LED_STRIP ) }, { C1( 23*N_LED_STRIP, 24*N_LED_STRIP-1 ), C3( 24*N_LED_STRIP-1, 23*N_LED_STRIP ) } },
  { { C0( 24*N_LED_STRIP, 25*N_LED_STRIP-1 ), C2( 25*N_LED_STRIP-1, 24*N_LED_STRIP ) }, { C1( 24*N_LED_STRIP, 25*N_LED_STRIP-1 ), C3( 25*N_LED_STRIP-1, 24*N_LED_STRIP ) } }
};

// general controls
byte masterBrightness = 255;
#define COLOR_ORDER RGB
#define COLOR_CORRECTION TypicalLEDStrip

//const unsigned long powerSupplyAmps = 10UL; // 10A supply on the control rig

const byte targetFPS = 50; // frames per scond, Hertz.  

const int ledPin = 13; // useful for showing what's going on

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
  FastLED.setMaxRefreshRate(targetFPS);
  // can define how much power to draw.  Set for 10A power supply
//  FastLED.setMaxPowerInVoltsAndMilliamps(12, powerSupplyAmps * 1000UL); // V, mA
  
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
  FastLED.show();  // will simply call FastLED.delay() if it's too soon to show by
                   // fps.  _very_ desireable, as the software-level dithering is 
                   // called so we get smooth colors at low duty.
  updates++;

  // do some updates to the background.  
  // this will light all of the LEDs at brightness with a pallette wash.
  static CRGBPalette16 palette = PartyColors_p;
  byte bright = 128;
  showPalleteBackground(palette, bright);

  // send tracers down the bore
  byte tracerSaturation = 128;
  byte tracerBrightness = 255;
  CHSV tracerColor = CHSV(HUE_BLUE, tracerSaturation, tracerBrightness);
  byte tracerCount = 3;
  showTracerBore(tracerCount, tracerColor);

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

void showPalleteBackground(CRGBPalette16 palette, byte bright) {
  // do some periodic updates
  static byte indexStart = 0;

  // apply the pallette to the first face
  for( byte i=0; i<N_LED_STRIP; i++ ) {
    Face[0][0][0][i] = ColorFromPalette(palette, indexStart+i, bright);
    Face[0][1][0][i] = Face[0][0][0][i]; // back the same as front
    // invert the color progression on the other side of the DP
    Face[0][0][1][i] = ColorFromPalette(palette, indexStart+128-i, bright);
    Face[0][1][1][i] = Face[0][0][1][i]; // back the same as front
    // replicate
    for( byte j=1; j<N_STRIP_CONTROL; j++ ) {
      Face[j][0][0][i] = Face[0][0][0][i];
      Face[j][0][1][i] = Face[0][1][0][i];
      Face[j][1][0][i] = Face[1][0][0][i];
      Face[j][1][1][i] = Face[1][1][0][i];
    }
  }

  /*
  C0.fill_rainbow(hue, hueDelta);
  C0.fadeToBlackBy(fadeBy);
  C1 = C0;

  // and invert the colors on the other side of the DP
  C2.fill_rainbow(hue+128, -hueDelta);
  C2.fadeToBlackBy(fadeBy);
  C3 = C2;
  */
  
  // bump the index up
  indexStart++;
}

void showTracerBore(byte tracerCount, CHSV tracerColor) {

  const byte nMax = 50;
  if( tracerCount > nMax) { Serial << F("showTracerBore. tracerCount>nMax. halting.") << endl; while(1); }

  static int tracerPos[nMax];
  static int tracerDir[nMax];
  static byte tracerSide[nMax];

  static byte inStartup = true;
  if( inStartup ) {
    // set all of the active tracers to "at the end", so we bootstrap correctly.
    for(byte i=0; i<tracerCount; i++) {
      tracerDir[i] = +1;
      tracerPos[i] = N_STRIP_CONTROL;
      tracerSide[i] = 0;
    }
    inStartup = false;
  }

  for( byte i=0; i<tracerCount; i++ ) {
    // step
    tracerPos[i] += tracerDir[i];
    // are we OOB?
    if( tracerPos[i] < 0 || tracerPos[i] > N_STRIP_CONTROL-1 ) {
      // need a new tracer
      tracerPos[i] = random8(0, N_STRIP_CONTROL);
      tracerDir[i] = random8()>128 ? 1 : -1;
      tracerSide[i] = random8(0, N_P_DP);
    }
    // paint
    Face[tracerPos[i]][0][tracerSide[i]] = tracerColor;
    Face[tracerPos[i]][1][tracerSide[i]] = tracerColor;
  }
}

