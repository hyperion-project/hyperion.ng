 
#include "FastLED.h"

#define ANALOG_MODE_AVERAGE  0
#define ANALOG_MODE_LAST_LED 1

/**************************************
   S E T U P

   set following values to your needs
 **************************************/

// Number of leds in your strip. set to 1 and ANALOG_OUTPUT_ENABLED to true to activate analog only
#define NUM_LEDS 100

#define LED_TYPE WS2812B  // type of your led controller, possible values, see below

// 3 wire (pwm): NEOPIXEL BTM1829 TM1812 TM1809 TM1804 TM1803 UCS1903 UCS1903B UCS1904 UCS2903 WS2812 WS2852
//               S2812B SK6812 SK6822 APA106 PL9823 WS2811 WS2813 APA104 WS2811_40 GW6205 GW6205_40 LPD1886 LPD1886_8BIT 
// 4 wire (spi): LPD8806 WS2801 WS2803 SM16716 P9813 APA102 SK9822 DOTSTAR

// For 3 wire led stripes line Neopixel/Ws2812, which have a data line, ground, and power, you just need to define DATA_PIN.
// For led chipsets that are SPI based (four wires - data, clock, ground, and power), both defines DATA_PIN and CLOCK_PIN are needed

// DATA_PIN, or DATA_PIN, CLOCK_PIN
#define LED_PINS MOSI        // 3 wire leds
//#define LED_PINS MOSI, SCK // 4 wire leds

#define COLOR_ORDER GRB  // colororder of the stripe, set RGB in hyperion

#define OFF_TIMEOUT 15000    // ms to switch off after no data was received, set 0 to deactivate

// analog rgb uni color led stripe - using of hyperion smoothing is recommended
#define ANALOG_MODE           ANALOG_MODE_LAST_LED  // use ANALOG_MODE_AVERAGE or ANALOG_MODE_LAST_LED


// overall color adjustments
#define ANALOG_BRIGHTNESS_RED   255              // maximum brightness for analog 0-255
#define ANALOG_BRIGHTNESS_GREEN 255              // maximum brightness for analog 0-255
#define ANALOG_BRIGHTNESS_BLUE  255              // maximum brightness for analog 0-255

#define BRIGHTNESS 128                      // maximum brightness 0-255
#define DITHER_MODE BINARY_DITHER           // BINARY_DITHER or DISABLE_DITHER
#define COLOR_TEMPERATURE CRGB(255,255,255) // RGB value describing the color temperature
#define COLOR_CORRECTION  TypicalLEDStrip   // predefined fastled color correction
//#define COLOR_CORRECTION  CRGB(255,255,255) // or RGB value describing the color correction


// Baudrate, higher rate allows faster refresh rate and more LEDs (defined in /etc/boblight.conf)
 #define serialRate 115200      // use 115200 for ftdi based boards
//#define serialRate 460800      // use 115200 for ftdi based boards

// ATTENTION  this pin config is default for atmega328 based arduinos, others might work to
//            if you have flickering analog leds this might be caused by unsynced pwm signals
//            try other pins is more or less the only thing that helps

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  // 2  3  4  5  6  7  8  9 10 11 12 13 44 45 46
  // R  B  G  R  B  G  R  B  G  R  B  G  R  B  G
  #define ANALOG_PINS_MAX 15
  #define ANALOG_RGB_STRIPES 4
  const byte ANALOG_PINS[ANALOG_PINS_MAX] = {2,4,3,5,7,6,8,10,9,11,13,12,44,46,45}; 
#else
// 9  10  11
// R  B  G
  #define ANALOG_PINS_MAX 3
  #define ANALOG_RGB_STRIPES 1
  const byte ANALOG_PINS[ANALOG_PINS_MAX] = {9,11,10}; 
#endif

/**************************************
   A D A L I G H T   C O D E

   no user changes needed
 **************************************/

// Adalight sends a "Magic Word" (defined in /etc/boblight.conf) before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

unsigned long endTime;

// Define the array of leds
CRGB leds[NUM_LEDS];

// set rgb to analog led stripe
void showAnalogRGB(const CRGB& led, const short stripeId=-1) {
  if (ANALOG_RGB_STRIPES > 0) {
    byte r = map(led.r, 0,255,0,ANALOG_BRIGHTNESS_RED);
    byte g = map(led.g, 0,255,0,ANALOG_BRIGHTNESS_GREEN);
    byte b = map(led.b, 0,255,0,ANALOG_BRIGHTNESS_BLUE);
    if (stripeId<0) {
      for (byte i=0;i<ANALOG_RGB_STRIPES;i++) {
         analogWrite(ANALOG_PINS[3*i+1], r);
         analogWrite(ANALOG_PINS[3*i]  , g);
         analogWrite(ANALOG_PINS[3*i+2], b);
      }
    } else {
         analogWrite(ANALOG_PINS[stripeId*3]  , g);
         analogWrite(ANALOG_PINS[stripeId*3+1], r);
         analogWrite(ANALOG_PINS[stripeId*3+2], b);
    }
  }
}


// set color to all leds
void showColor(const CRGB& led) {
 #if NUM_LEDS > ANALOG_RGB_STRIPES
  LEDS.showColor(led);
  #endif
  showAnalogRGB(led);
}


// function to check if serial data is available
// if timeout occured leds switch of, if configured
bool checkIncommingData() {
  boolean dataAvailable = true;
  while (!Serial.available()) {
    if ( OFF_TIMEOUT > 0 && endTime < millis()) {
      showColor(CRGB(0,0,0)); // leds off
      dataAvailable = false;
      endTime = millis() + OFF_TIMEOUT;
    }
  }

  return dataAvailable;
}

// main function that setups and runs the code
void setup() {

  // analog output
  if (ANALOG_RGB_STRIPES > 0) {
    for (byte i=0;i<ANALOG_PINS_MAX;i++) {
      pinMode(ANALOG_PINS[i], OUTPUT);
    }
  }

  // Uncomment/edit one of the following lines for your leds arrangement.
  int ledCount = NUM_LEDS;
  if (ANALOG_MODE == ANALOG_MODE_LAST_LED) {
    ledCount -= ANALOG_RGB_STRIPES;
  }

  #if NUM_LEDS > ANALOG_RGB_STRIPES
    FastLED.addLeds<LED_TYPE, LED_PINS, COLOR_ORDER>(leds, ledCount);
  #endif
 
  // color adjustments
  FastLED.setBrightness ( BRIGHTNESS );
  FastLED.setTemperature( COLOR_TEMPERATURE );
  FastLED.setCorrection ( COLOR_CORRECTION );
  FastLED.setDither     ( DITHER_MODE );

  // initial RGB flash
  showColor(CRGB(255, 0, 0));  delay(400);
  showColor(CRGB(0, 255, 0));  delay(400);
  showColor(CRGB(0, 0, 255));  delay(400);
  showColor(CRGB(0, 0, 0));
  
  Serial.begin(serialRate);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.print("Ada\n"); // Send "Magic Word" string to host

  boolean transmissionSuccess;
  unsigned long sum_r, sum_g, sum_b;

  // loop() is avoided as even that small bit of function overhead
  // has a measurable impact on this code's overall throughput.
  while (true) {
    // wait for first byte of Magic Word
    for (i = 0; i < sizeof prefix; ++i) {
      // If next byte is not in Magic Word, the start over
      if (!checkIncommingData() || prefix[i] != Serial.read()) {
        i = 0;
      }
    }
    // Hi, Lo, Checksum
    if (!checkIncommingData()) continue;
    hi = Serial.read();
    if (!checkIncommingData()) continue;
    lo = Serial.read();
    if (!checkIncommingData()) continue;
    chk = Serial.read();

    // if checksum does not match go back to wait
    if (chk != (hi ^ lo ^ 0x55)) continue;

    memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
    transmissionSuccess = true;
    sum_r = 0;
    sum_g = 0;
    sum_b = 0;

    // read the transmission data and set LED values
    for (uint8_t idx = 0; idx < NUM_LEDS; idx++) {
      byte r, g, b;
      if (!checkIncommingData()) {
        transmissionSuccess = false;
        break;
      }
      r = Serial.read();
      if (!checkIncommingData()) {
        transmissionSuccess = false;
        break;
      }
      g = Serial.read();
      if (!checkIncommingData()) {
        transmissionSuccess = false;
        break;
      }
      b = Serial.read();
      leds[idx].r = r;
      leds[idx].g = g;
      leds[idx].b = b;
      #if ANALOG_OUTPUT_ENABLED == true && ANALOG_MODE == ANALOG_MODE_AVERAGE
          sum_r += r;
          sum_g += g;
          sum_b += b;
      #endif
    }

    // shows new values
    if (transmissionSuccess) {
      endTime = millis() + OFF_TIMEOUT;
     #if NUM_LEDS > ANALOG_RGB_STRIPES
      FastLED.show();
      #endif

      #if ANALOG_RGB_STRIPES > 0
        #if ANALOG_MODE == ANALOG_MODE_LAST_LED
          for ( byte x=1; x<=ANALOG_RGB_STRIPES; x++) {
            showAnalogRGB(leds[NUM_LEDS-x], x-1);
          }
        #else
          showAnalogRGB(CRGB(sum_r/NUM_LEDS, sum_g/NUM_LEDS, sum_b/NUM_LEDS));
         #endif
      #endif
    }
  }
} // end of setup


void loop() {
  // Not used. See note in setup() function.
}

