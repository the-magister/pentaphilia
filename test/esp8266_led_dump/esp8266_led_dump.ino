// compilation settings
// Board: Adafruit HUZZAH ESP8266, 80 Mhz, 4M (3M SPIFFS).
// See here for installation of the IDE: https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide

#include <Streaming.h>
#include <FastLED.h>
extern "C" {
#include "user_interface.h"
}

// See: https://github.com/FastLED/FastLED/wiki/ESP8266-notes

FASTLED_USING_NAMESPACE

// hardware SPI on ESP8266
#define SPI_SCK 14 // clock line; CLK
#define SPI_MOSI 13 // master in, slave out; NC
#define SPI_MISO 12 // master out, slave in; DATA

// 45mm 12V modules
#define DATA_PIN SPI_MISO
#define CLOCK_PIN SPI_SCK
#define LED_TYPE WS2801
#define COLOR_ORDER RGB

// pentaphilia dimensions
#define N_LED_STRAND 20 // number of LEDs in a strand of lights
#define N_GAP (125-1) // number of double pentagons gaps we'll light
//#define N_LED N_GAP*N_LED_STRAND // number of LEDs on a single data/clock pair
#define N_LED 20 // number of LEDs on a single data/clock pair

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

// general controls
#define BRIGHTNESS          255

// rotating "base color" used by many of the patterns
uint8_t gHue = 0; 


/****Variables needed for sending to Processing. */
uint16_t sendDelay = 10;    // [Milliseconds] To slow stuff down if needed.
boolean testing = true;  // Default is false. [Ok to change for testing.]
  // Can temporarily change testing to true to check output in serial monitor.
  // Must set back to false to allow Processing to connect and receive data.

boolean linkedUp = false;  // Initially set linkup status false. [Do Not change.]
/****End of variables needed for sending Processing. */


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
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, N_LED).setCorrection(TypicalLEDStrip);
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

  /****Stuff needed in setup() section for sending to Processing. */
  Serial.begin(115200);  // Allows serial output.
  while (!Serial){ ; }  // Wait for serial connection. Only needed for Leonardo board.
  firstContact();  // Connect with Processing. Hello, is anyone out there?
  /****End of stuff to put in your setup() section for Processing. */


}

void loop() {

  // send the 'leds' array out to the actual LED strip
  showLEDs();  

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
  showLEDs();  
}

void showLEDs() {
  FastLED.show();  
  if( linkedUp ) SendToProcessing();
}

/****The below two functions are needed for sending to Processing. */
// Copy from here to the bottom.
//--------------------
// Waits for Processing to respond and then sends the number of pixels.
void firstContact() {
  uint16_t nLEDS = N_LED;  // Number to send to Processing.  (Allows up to 65,535 pixels)
  if (testing == true){
    linkedUp = true;  // If testing, pretend we are already connected to Processing.
    Serial.print("N_LED: "); Serial.println(nLEDS);  // Number of pixels in our LED strip. 
    Serial.print("  High Byte = "); Serial.print(highByte(nLEDS));  // The high byte.
    Serial.print(" x 256 = "); Serial.println(highByte(nLEDS) * 256);
    Serial.print("  Low Byte  = "); Serial.println(lowByte(nLEDS));  // The low byte.
    delay(3000);
  }
  else {
    while (Serial.available() <= 0) {  // Repeats until Processing responds. Hello?
      Serial.print('A');  // send an ASCII A (byte of value 65)
      delay(100);
    }
    // Once Processing responds send the number of pixels as two bytes.
    Serial.write(highByte(nLEDS));  // Send the high byte to Processing.
    Serial.write(lowByte(nLEDS));  // Send the low byte to Processing.
    Serial.print('#');  // send an ASCII # (byte of value 35) as a flag for Processing.
    linkedUp = true;  // Now that Processing knows number of pixels set linkedUp to true.
    delay(500);
  }
}

//--------------------
// This function sends ALL the pixel data to Processing.
void SendToProcessing() {
  if (testing == true){  // Print pixel data. If N_LED is large this will be a lot of data!
    Serial.println("-------------------------------------------------------");
    for (uint16_t d=0; d < N_LED; d++){
      Serial.print("p: "); Serial.print(d);
      Serial.print("\tr: "); Serial.print(leds[d].r);
      Serial.print("\tg: "); Serial.print(leds[d].g);
      Serial.print("\tb: "); Serial.println(leds[d].b);
    }
    Serial.println(" ");
    delay(500);  // Add some extra delay while testing.
  }
  else {  // Send ALL the pixel data to Processing!
    for (uint16_t d=0; d < N_LED; d++){
      Serial.write(d);          // Pixel number
      Serial.write(leds[d].r);  // Red channel data
      Serial.write(leds[d].g);  // Green channel data
      Serial.write(leds[d].b);  // Blue channel data
    }
    delay(sendDelay);  // Delay to slow things down if needed.
  }
}

//--------------------
/****End of the functions needed for sending to Processing.*/

