#include "FastLED.h"

/**************************************
   S E T U P

   set following values to your needs
 **************************************/

// Number of leds in your strip
#define NUM_LEDS 100

// For 3 wire led stripes line Neopixel/Ws2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.
// For led chipsets that are SPI based (four wires - data, clock, ground, and power),
// like the APA102, WS2801, LPD8806 both defines DATA_PIN and CLOCK_PIN are needed
#define DATA_PIN 6
#define CLOCK_PIN 13

#define COLOR_ORDER RGB  // colororder of the stripe.
#define GROUND_PIN 5     // additional ground pin to make wiring a bit easier

#define OFF_TIMEOUT 1000    // ms to switch off after no data was received, set 0 to deactivate

// analog rgb uni color led
#define ANALOG_OUTPUT_ENABLED true
#define ANALOG_RED_PIN   11
#define ANALOG_GREEN_PIN 10
#define ANALOG_BLUE_PIN   9

// overall color adjustments
#define BRIGHTNESS 96                      // maximum brightness 0-255
#define DITHER_MODE BINARY_DITHER          // BINARY_DITHER or DISABLE_DITHER
#define COLOR_TEMPERATURE CRGB(255,255,255) // RGB value describing the color temperature
#define COLOR_CORRECTION  CRGB(255,255,255) // RGB value describing the color correction

// Baudrate, higher rate allows faster refresh rate and more LEDs (defined in /etc/boblight.conf)
//#define serialRate 460800
#define serialRate 115200



/**************************************
   A D A L I G H T   C O D E

   no user changes needed
 **************************************/

// Adalight sends a "Magic Word" (defined in /etc/boblight.conf) before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

unsigned long endTime;

// Define the array of leds
CRGB leds[NUM_LEDS];


void showAnalogRGB(byte r, byte g, byte b) {
  if (ANALOG_OUTPUT_ENABLED) {
    analogWrite(ANALOG_RED_PIN   , r);
    analogWrite(ANALOG_GREEN_PIN , g);
    analogWrite(ANALOG_BLUE_PIN  , b);
  }
}

void switchOff() {
  memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
  FastLED.show();
  showAnalogRGB(0, 0, 0);
}

bool checkIncommingData() {
  boolean dataAvailable = true;
  while (!Serial.available()) {
    if ( OFF_TIMEOUT > 0 && endTime < millis()) {
      switchOff();
      dataAvailable = false;
      endTime = millis() + OFF_TIMEOUT;
    }
  }

  return dataAvailable;
}

void setup() {

  // additional ground pin to make wiring a bit easier
  pinMode(GROUND_PIN, OUTPUT);
  digitalWrite(GROUND_PIN, LOW);

  // analog output
  if (ANALOG_OUTPUT_ENABLED) {
    pinMode(ANALOG_RED_PIN  , OUTPUT);
    pinMode(ANALOG_GREEN_PIN, OUTPUT);
    pinMode(ANALOG_BLUE_PIN , OUTPUT);
    showAnalogRGB(0, 0, 0);
  }

  // Uncomment/edit one of the following lines for your leds arrangement.
  // FastLED.addLeds<NEOPIXEL    , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<BTM1829     , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1812      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1809      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1804      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1803      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903     , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903B    , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1904     , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS2903     , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2812      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2852      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B     , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<SK6812      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<SK6822      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<APA106      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<PL9823      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2811      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2813      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<APA104      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2811_40   , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205      , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205_40   , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD1886     , DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD1886_8BIT, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2801 , DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2803 , DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<SM16716, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<P9813  , DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<APA102 , DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<SK9822 , DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // color adjustments
  FastLED.setBrightness ( BRIGHTNESS );
  FastLED.setTemperature( COLOR_TEMPERATURE );
  FastLED.setCorrection ( COLOR_CORRECTION );
  FastLED.setDither     ( DITHER_MODE );

  // initial RGB flash
  LEDS.showColor(CRGB(255, 0, 0));
  delay(400);
  LEDS.showColor(CRGB(0, 255, 0));
  delay(400);
  LEDS.showColor(CRGB(0, 0, 255));
  delay(400);
  LEDS.showColor(CRGB(0, 0, 0));

  Serial.begin(serialRate);
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
      sum_r += r;
      sum_g += g;
      sum_b += b;
    }

    // shows new values
    if (transmissionSuccess) {
      endTime = millis() + OFF_TIMEOUT;
      FastLED.show();
      showAnalogRGB(sum_r / NUM_LEDS, sum_g / NUM_LEDS, sum_b / NUM_LEDS);
    }
  }
} // end of setup


void loop() {
  // Not used. See note in setup() function.
}
