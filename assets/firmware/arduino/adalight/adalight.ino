#include "FastLED.h"

#define ANALOG_MODE_AVERAGE  0
#define ANALOG_MODE_LAST_LED 1

/**************************************
   S E T U P

   set following values to your needs
 **************************************/

// Number of leds in your strip
#define NUM_LEDS 100

#define SPI_LEDS false    // connection type. Set "true" for 4 wire and "false" for 3 Wire stripes.
#define LED_TYPE WS2812B  // type of your led controller, possible values, see below

// 3 wire (pwm): NEOPIXEL BTM1829 TM1812 TM1809 TM1804 TM1803 UCS1903 UCS1903B UCS1904 UCS2903 WS2812 WS2852
//               S2812B SK6812 SK6822 APA106 PL9823 WS2811 WS2813 APA104 WS2811_40 GW6205 GW6205_40 LPD1886 LPD1886_8BIT 
// 4 wire (spi): LPD8806 WS2801 WS2803 SM16716 P9813 APA102 SK9822 DOTSTAR

// For 3 wire led stripes line Neopixel/Ws2812, which have a data line, ground, and power, you just need to define DATA_PIN.
// For led chipsets that are SPI based (four wires - data, clock, ground, and power), both defines DATA_PIN and CLOCK_PIN are needed
#define DATA_PIN  11
#define CLOCK_PIN 13

#define COLOR_ORDER RGB  // colororder of the stripe, set RGB in hyperion
#define GROUND_PIN 4     // additional ground pin to make wiring a bit easier

#define OFF_TIMEOUT 10000    // ms to switch off after no data was received, set 0 to deactivate

// analog rgb uni color led
#define ANALOG_OUTPUT_ENABLED true
#define ANALOG_MODE           ANALOG_MODE_AVERAGE  // use ANALOG_MODE_AVERAGE or ANALOG_MODE_LAST_LED
#define ANALOG_RED_PIN        6
#define ANALOG_GREEN_PIN      5
#define ANALOG_BLUE_PIN       3

// overall color adjustments
#define ANALOG_BRIGHTNESS_RED   255              // maximum brightness for analog 0-255
#define ANALOG_BRIGHTNESS_GREEN 255              // maximum brightness for analog 0-255
#define ANALOG_BRIGHTNESS_BLUE  255               // maximum brightness for analog 0-255

#define BRIGHTNESS 255                      // maximum brightness 0-255
#define DITHER_MODE BINARY_DITHER          // BINARY_DITHER or DISABLE_DITHER
#define COLOR_TEMPERATURE CRGB(255,255,255) // RGB value describing the color temperature
#define COLOR_CORRECTION  CRGB(255,255,255) // RGB value describing the color correction

// Baudrate, higher rate allows faster refresh rate and more LEDs (defined in /etc/boblight.conf)
#define serialRate 460800
//#define serialRate 115200    // for older ftdi based boards



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
void showAnalogRGB(byte r, byte g, byte b) {
  if (ANALOG_OUTPUT_ENABLED) {
    analogWrite(ANALOG_RED_PIN   , map(r, 0,255,0,ANALOG_BRIGHTNESS_RED));
    analogWrite(ANALOG_GREEN_PIN , map(g, 0,255,0,ANALOG_BRIGHTNESS_GREEN));
    analogWrite(ANALOG_BLUE_PIN  , map(b, 0,255,0,ANALOG_BRIGHTNESS_BLUE));
  }
}

// switch of digital and analog leds
void switchOff() {
  memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
  FastLED.show();
  showAnalogRGB(0, 0, 0);
}

// function to check if serial data is available
// if timeout occured leds switch of, if configured
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

// main function that setups and runs the code
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
  int ledCount = NUM_LEDS;
  if (ANALOG_MODE == ANALOG_MODE_LAST_LED) {
    ledCount--;
  }
  
  #if SPI_LEDS == true
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, ledCount);
  #else
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, ledCount);
  #endif
 
  // color adjustments
  FastLED.setBrightness ( BRIGHTNESS );
  FastLED.setTemperature( COLOR_TEMPERATURE );
  FastLED.setCorrection ( COLOR_CORRECTION );
  FastLED.setDither     ( DITHER_MODE );

  // initial RGB flash
  LEDS.showColor(CRGB(255, 0, 0));
  showAnalogRGB(255, 0, 0);
  delay(400);
  LEDS.showColor(CRGB(0, 255, 0));
  showAnalogRGB(0, 255, 0);
  delay(400);
  LEDS.showColor(CRGB(0, 0, 255));
  showAnalogRGB(0, 0, 255);
  delay(400);
  LEDS.showColor(CRGB(0, 0, 0));
  showAnalogRGB(0, 0, 0);
  
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
      if ( ANALOG_OUTPUT_ENABLED && ANALOG_MODE == ANALOG_MODE_AVERAGE )
      {
        sum_r += r;
        sum_g += g;
        sum_b += b;
      }
    }

    // shows new values
    if (transmissionSuccess) {
      endTime = millis() + OFF_TIMEOUT;
      FastLED.show();

      if ( ANALOG_OUTPUT_ENABLED)
      {
        switch (ANALOG_MODE)
        {
          case ANALOG_MODE_LAST_LED:
            showAnalogRGB(leds[NUM_LEDS-1].r, leds[NUM_LEDS-1].g, leds[NUM_LEDS-1].b);
            break;

          default:
            showAnalogRGB(sum_r / NUM_LEDS, sum_g / NUM_LEDS, sum_b / NUM_LEDS);
            break;
        }
      }
    }
  }
} // end of setup


void loop() {
  // Not used. See note in setup() function.
}
