#include <Streaming.h>
#include <Metro.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

// general definitions
#define N_LED 20

// 35x35mm WS2801 modules, "square"
#define PROTOCOL_SQUARE WS2801
#define COLOR_ORDER_SQUARE RGB
#define SQ_1_CLK 2
#define SQ_1_DATA 3
#define SQ_2_CLK 4
#define SQ_2_DATA 5

// 30mm WS2811 modules, "round"
#define PROTOCOL_ROUND WS2811
#define COLOR_ORDER_ROUND RGB
#define RND_1_DATA 6
#define RND_2_DATA 7

// malloc
CRGB sq1[20];
CRGB sq2[20];
CRGB rnd1[20];
CRGB rnd2[20];

// general controls
#define FRAMES_PER_SECOND   20
#define BRIGHTNESS          255
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
boolean stripOn[4] = {true, true, true, true};
byte animation = 2;
byte pixelGap = 1;
const byte LED = 13;

void setup() {
  Serial.begin(115200);
  Serial << F("Startup.") << endl;

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<PROTOCOL_SQUARE, SQ_1_DATA, SQ_1_CLK, COLOR_ORDER_SQUARE>(sq1, N_LED).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<PROTOCOL_SQUARE, SQ_2_DATA, SQ_2_CLK, COLOR_ORDER_SQUARE>(sq2, N_LED).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<PROTOCOL_ROUND, RND_1_DATA, COLOR_ORDER_ROUND>(rnd1, N_LED).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<PROTOCOL_ROUND, RND_2_DATA, COLOR_ORDER_ROUND>(rnd2, N_LED).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  // set up LED to show hearbeat
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);  

  solid(CHSV(HUE_RED, 255, 255));
  FastLED.show();  
  delay(1000);
  
  solid(CHSV(HUE_GREEN, 255, 255));
  FastLED.show();  
  delay(1000);

  solid(CHSV(HUE_BLUE, 255, 255));
  FastLED.show();  
  delay(1000);

  wipe();
  FastLED.show();  
}

void loop() {

  // blink
  digitalWrite(LED, LOW);  

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // blink
  digitalWrite(LED, HIGH);  

  // animation
  if( animation == 0 ) {
    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    wipe();
    rainbow(CHSV(gHue, 255, 255));
  }
  if( animation == 1 ) {
    // do some periodic updates
    EVERY_N_SECONDS( 3 ) { gHue+=32; } // slowly cycle the "base color" through the rainbow
    wipe();
    solid(CHSV(gHue, 255, 255));
  }
  if( animation == 2 ) { // Cylon
    fade();
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    cylon(CHSV(gHue, 255, 255));
  }


  // check for inputs
  if( Serial.available()>0 ) {
    char input = Serial.read();
    Serial << F("Input received: ") << input << endl;
    switch( input ) {
      case '1': stripOn[0]=!stripOn[0]; break;
      case '2': stripOn[1]=!stripOn[1]; break;
      case '3': stripOn[2]=!stripOn[2]; break;
      case '4': stripOn[3]=!stripOn[3]; break;
      case '-': if(pixelGap>1) { pixelGap--; }; break;
      case '+': if(pixelGap<20) { pixelGap++; }; break;
      case 'r': wipe(); animation = 0; break;
      case 's': wipe(); animation = 1; gHue = 0; break;
      case 'c': wipe(); animation = 2; break;
    }
    Serial << F("[+, -]: set pixel module spacing.  Currently=") << pixelGap << endl;
    Serial << F("[1,2,3,4]: toggle string.  Currently=");
    Serial << stripOn[0] << F(" ");
    Serial << stripOn[1] << F(" ");
    Serial << stripOn[2] << F(" ");
    Serial << stripOn[3] << endl;
    Serial << F("[r,s,c]: toggle animation.  Currently=") << animation << endl;
  }
}

void wipe() {
   // wipe clean
  fill_solid(sq1, N_LED, CRGB::Black);
  fill_solid(sq2, N_LED, CRGB::Black);
  fill_solid(rnd1, N_LED, CRGB::Black);
  fill_solid(rnd2, N_LED, CRGB::Black);
}

void fade() {
  byte fadeBy = 64;
  for(int i = 0; i < N_LED; i++) { 
    sq1[i].fadeLightBy(fadeBy); 
    sq2[i].fadeLightBy(fadeBy); 
    rnd1[i].fadeLightBy(fadeBy); 
    rnd2[i].fadeLightBy(fadeBy); 
  }
}

void solid(CHSV color) {
  if( stripOn[0] ) { for(byte i=0; i<N_LED; i+=pixelGap) sq1[i]=color; }
  if( stripOn[1] ) { for(byte i=0; i<N_LED; i+=pixelGap) sq2[i]=color; } 
  if( stripOn[2] ) { for(byte i=0; i<N_LED; i+=pixelGap) rnd1[i]=color; }
  if( stripOn[3] ) { for(byte i=0; i<N_LED; i+=pixelGap) rnd2[i]=color; }
}

void rainbow(CHSV color) {

  byte hue;
  hue = color.hue;
  if( stripOn[0] ) { for(byte i=0; i<N_LED; i+=pixelGap) sq1[i]=CHSV(hue+=10*pixelGap, 255, 255); }
  hue = color.hue;
  if( stripOn[1] ) { for(byte i=0; i<N_LED; i+=pixelGap) sq2[i]=CHSV(hue+=10*pixelGap, 255, 255); }
  hue = color.hue;
  if( stripOn[2] ) { for(byte i=0; i<N_LED; i+=pixelGap) rnd1[i]=CHSV(hue+=10*pixelGap, 255, 255); }
  hue = color.hue;
  if( stripOn[3] ) { for(byte i=0; i<N_LED; i+=pixelGap) rnd2[i]=CHSV(hue+=10*pixelGap, 255, 255); }
}

void cylon(CHSV color) {
  static int pos=0;
  static int dir = 1;
   
  pos = pos + dir*(int)pixelGap;
  if( pos >= N_LED ) {
    dir = -1;
    pos = pos + dir*(int)pixelGap;
  }
  if( pos < 0 ) {
    dir = +1;
    pos = pos + dir*(int)pixelGap;
  }

  if( stripOn[0] ) { sq1[pos]=color; }
  if( stripOn[1] ) { sq2[pos]=color; } 
  if( stripOn[2] ) { rnd1[pos]=color; }
  if( stripOn[3] ) { rnd2[pos]=color; }

}

