#include <Streaming.h>
#include <Metro.h>
#include <FastLED.h>
#include <Bounce.h>

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

// rotating "base color" used by many of the patterns
uint8_t gHue = 0; 

// starting position
byte whichStrip=2;
byte pixelGap=5;

// PIR sensor
#define PIR1_PIN 8
Bounce pir1 = Bounce(PIR1_PIN, 5);

void setup() {
  Serial.begin(115200);
  Serial << F("Startup.") << endl;

  // need voltage steering
  pinMode(PIR1_PIN, INPUT_PULLUP);

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

  blackout();
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

  // animation
  rainbow();

  // check for inputs
  if( Serial.available()>0 ) {
    byte input = Serial.read();
    switch( input ) {
      case '1': pixelGap=1; break;
      case '2': pixelGap=2; break;
      case '3': pixelGap=3; break;
      case '4': pixelGap=4; break;
      case '5': pixelGap=5; break;
      case '6': pixelGap=6; break;
      case '7': pixelGap=7; break;
      case '8': pixelGap=8; break;
      case '9': pixelGap=9; break;
      case 'a': whichStrip=0; break;
      case 'b': whichStrip=1; break;
      case 'c': whichStrip=2; break;
      default:
        Serial << F("Enter [1-9] to set pixel module spacing.  Currently=") << pixelGap << endl;
        Serial << F("Enter [a-c] to set string usage.  Currently=") << whichStrip << endl;
    }
  }

  // check for people
  static Metro retriggerInterval(2000UL);
  if( pir1.update() && pir1.read()==LOW ) {
    if( retriggerInterval.check() ) {
      Serial << F("Trigger!") << endl;
      
      if( whichStrip==1 ) { 
        for(byte i=0; i<N_LED_30mm; i+=pixelGap) leds_30mm[i]=CRGB::White;
      } else if( whichStrip==2 ) {
        for(byte i=0; i<N_LED_45mm; i+=pixelGap) leds_45mm[i]=CRGB::White;
      }      
      FastLED.show();
      
      delay(500);
    }
  }
}

void blackout() {
  // wipe clean
  fill_solid(leds_45mm, N_LED_45mm, CRGB::Black);
  fill_solid(leds_30mm, N_LED_30mm, CRGB::Black);
}

void rainbow() 
{

  blackout();
  
  if( whichStrip==1 ) { 
    for(byte i=0; i<N_LED_30mm; i+=pixelGap) leds_30mm[i]=CHSV(gHue, 255, 255);
  } else if( whichStrip==2 ) {
    for(byte i=0; i<N_LED_45mm; i+=pixelGap) leds_45mm[i]=CHSV(gHue, 255, 255);
  }
}
