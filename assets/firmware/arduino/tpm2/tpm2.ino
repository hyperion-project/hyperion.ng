#include <pt.h>
#include "FastLED.h"

#define ANALOG_MODE_AVERAGE  0
#define ANALOG_MODE_LAST_LED 1


/*==============================================================================*/
/* LED und Arduino Variablen */
/*==============================================================================*/
#define INITIAL_LED_TEST_ENABLED true
#define INITIAL_LED_TEST_BRIGHTNESS 32  // 0..255
#define INITIAL_LED_TEST_TIME_MS 500  // 0..255

#define MAX_LEDS             400                // Number of LEDs
#define MAX_ARGS             10                 // Max Number of command arguments
#define BAUDRATE             460800             // Baudrate
#define SERIAL               Serial            // Serial port for communication
#define NUM_LEDS_PER_STRIP 20

// type of your led controller, possible values, see below
#define LED_TYPE WS2812B 

// 3 wire (pwm): NEOPIXEL BTM1829 TM1812 TM1809 TM1804 TM1803 UCS1903 UCS1903B UCS1904 UCS2903 WS2812 WS2852
//               S2812B SK6812 SK6822 APA106 PL9823 WS2811 WS2813 APA104 WS2811_40 GW6205 GW6205_40 LPD1886 LPD1886_8BIT 
// 4 wire (spi): LPD8806 WS2801 WS2803 SM16716 P9813 APA102 SK9822 DOTSTAR

// For 3 wire led stripes line Neopixel/Ws2812, which have a data line, ground, and power, you just need to define DATA_PIN.
// For led chipsets that are SPI based (four wires - data, clock, ground, and power), both defines DATA_PIN and CLOCK_PIN are needed

// DATA_PIN, or DATA_PIN, CLOCK_PIN
#define LED_PINS 6        // 3 wire leds
//#define LED_PINS 6, 13  // 4 wire leds

#define COLOR_ORDER GRB  // colororder of the stripe, set RGB in hyperion

#define OFF_TIMEOUT 15000    // ms to switch off after no data was received, set 0 to deactivate

// analog rgb uni color led stripe - using of hyperion smoothing is recommended
// ATTENTION  this pin config is default for atmega328 based arduinos, others might work to
//            if you have flickering analog leds this might be caused by unsynced pwm signals
//            try other pins is more or less the only thing that helps
#define ANALOG_OUTPUT_ENABLED false
#define ANALOG_MODE           ANALOG_MODE_LAST_LED  // use ANALOG_MODE_AVERAGE or ANALOG_MODE_LAST_LED
#define ANALOG_GROUND_PIN     8                     // additional ground pin to make wiring a bit easier
#define ANALOG_RED_PIN        9
#define ANALOG_GREEN_PIN      10
#define ANALOG_BLUE_PIN       11

// overall color adjustments
#define ANALOG_BRIGHTNESS_RED   255              // maximum brightness for analog 0-255
#define ANALOG_BRIGHTNESS_GREEN 255              // maximum brightness for analog 0-255
#define ANALOG_BRIGHTNESS_BLUE  255              // maximum brightness for analog 0-255

#define BRIGHTNESS 255                      // maximum brightness 0-255
#define DITHER_MODE BINARY_DITHER           // BINARY_DITHER or DISABLE_DITHER
#define COLOR_TEMPERATURE CRGB(255,255,255) // RGB value describing the color temperature
#define COLOR_CORRECTION  TypicalLEDStrip   // predefined fastled color correction
//#define COLOR_CORRECTION  CRGB(255,255,255) // or RGB value describing the color correction

// Baudrate, higher rate allows faster refresh rate and more LEDs
//#define serialRate 460800      // use 115200 for ftdi based boards
#define serialRate 115200      // use 115200 for ftdi based boards
//#define serialRate 500000         // use 115200 for ftdi based boards



/*==============================================================================*/
/* TPM2 Variablen */
/*==============================================================================*/

enum Protocol
{
   // positions
   posStart      = 0,
   posType       = 1,
   posFsHigh     = 2,
   posFsLow      = 3,
   posData       = 4,

   // bytes 
   tpm2Start     = 0xc9,
   tpm2DataFrame = 0xda,
   tpm2Command   = 0xc0,
   tpm2Answer    = 0xaa,
   tpm2End       = 0x36,
   tpm2Ack       = 0xAC
};

enum Mode
{
   mNone,
   mCommunication,
   mProgram
};

struct Data
{
   int pos;
   uint8_t type;
   uint16_t fs;
   uint16_t maxSize;
   uint8_t command;
   CRGB rgb[MAX_LEDS];
} data;

byte args[MAX_ARGS];
unsigned long lastDataAt = 0;
int program = 0;
int mode = mNone;
int effectDelay = 100;
static struct pt pt1;


// int freeRam() 
// {
//    extern int __heap_start, *__brkval; 
//    int v; 
//    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
// }

void setup()
{
  SERIAL.begin(BAUDRATE);

  delay(1000);  
  int ledCount = MAX_LEDS;

  if (ANALOG_MODE == ANALOG_MODE_LAST_LED) {
    ledCount--;
  }

  memset(data.rgb, 0, sizeof(struct CRGB) * ledCount); 

  FastLED.addLeds<LED_TYPE, LED_PINS, COLOR_ORDER>(data.rgb, ledCount);

  // color adjustments
  FastLED.setBrightness ( BRIGHTNESS );
  FastLED.setTemperature( COLOR_TEMPERATURE );
  FastLED.setCorrection ( COLOR_CORRECTION );
  FastLED.setDither     ( DITHER_MODE );

#ifdef INITIAL_LED_TEST_ENABLED
  for (int v=0;v<INITIAL_LED_TEST_BRIGHTNESS;v++)
  {
    oneColorAll(v,v,v);
    delay(INITIAL_LED_TEST_TIME_MS/2/INITIAL_LED_TEST_BRIGHTNESS);
  }
 
  for (int v=0;v<INITIAL_LED_TEST_BRIGHTNESS;v++)
  {
    oneColorAll(INITIAL_LED_TEST_BRIGHTNESS-v,INITIAL_LED_TEST_BRIGHTNESS-v,INITIAL_LED_TEST_BRIGHTNESS-v);
    delay(INITIAL_LED_TEST_TIME_MS/2/INITIAL_LED_TEST_BRIGHTNESS);
  }
  
  
  #define BRIGHT    127     //max led intensity (1-500)
#define INHALE    1250    //Inhalation time in milliseconds.
#define PULSE     INHALE*1000/BRIGHT
#define REST      1000    //Rest Between Inhalations.

while(true)
{
	  //ramp increasing intensity, Inhalation: 
  for (int i=1;i<BRIGHT;i++){
	   oneColorAll(i,i,i);
    delayMicroseconds(PULSE-i*10);   // wait
    delay(0);                        //to prevent watchdog firing.
  }
  //ramp decreasing intensity, Exhalation (half time):
  for (int i=BRIGHT-1;i>0;i--){
	   oneColorAll(i,i,i);
    delayMicroseconds(PULSE-i*10);   // wait
    i--;
    delay(0);                        //to prevent watchdog firing.
  }
  delay(REST);                       //take a rest...
}

#endif

  oneColorAll(0,0,0);

  memset(args, 0, MAX_ARGS);
  resetVars();
  PT_INIT(&pt1);
  while (!SERIAL) {
    ; // wait for serial port to connect. Needed for native USB
  }
}

/*==============================================================================*/
/* Thread für Programm/Effekte
/*==============================================================================*/

static int playProgramThread(struct pt *pt) 
{
   static unsigned long timestamp = 0;
   PT_BEGIN(pt);
   while(1) 
   {
      PT_WAIT_UNTIL(pt, millis() - timestamp > effectDelay);
      playProgram();
      timestamp = millis();
   }
   PT_END(pt);
}

/*==============================================================================*/
/* loop
/*==============================================================================*/

void loop()
{
  while (1)
  {
   // if data available switch to communication mode
   if (SERIAL.available() > 0)
   {
      if (mode != mCommunication)
      {
         mode = mCommunication;
         resetVars();
      }
      doCommunication();
   }
   else
   // communication timeout after 0.5 seconds
   while (SERIAL.available() == 0 && millis()-lastDataAt > 1000)
   {
      if (mode != mProgram)
      {
         mode = mProgram;
         resetVars();
      }
      else
        playProgramThread(&pt1);
   }
  }
}

/*==============================================================================*/
/* do communication
/*==============================================================================*/

void doCommunication()
{
   // read ...
   while (SERIAL.available() > 0)
   {
     
      byte val = SERIAL.read();
      lastDataAt = millis();

      if (data.pos == posStart && val == tpm2Start)                                    // Startbyte
         resetVars();

      else if (data.pos == posType && (val == tpm2DataFrame || val == tpm2Command))    // Block-Art
         data.type = val;

      else if (data.pos == posFsHigh)                                                  // Framesize (High Bit)
      {
         data.fs = (val << 8) & 0xFF00;
		 data.maxSize = data.fs;
      }
      else if (data.pos == posFsLow)                                                   // Framesize (Low byte)
      {
         data.fs += val & 0x00FF;
		 data.maxSize = data.fs;
         if (data.fs > MAX_LEDS*3) // framsize too high
         {
			 data.fs = MAX_LEDS*3;
         } 
         
      }
      else if (data.pos == posData + data.maxSize && val == tpm2End)                        // End Byte
         parsePacket();

      else if (data.pos >= posData && data.pos < posData+data.fs)                      // Bytes zwischen Header und Ende lesen
         evaluateData(val);

      else if (data.pos >= posData && data.pos < posData+data.maxSize)                 // Bytes zwischen Header und Ende lesen
         continue;

      else                                                                             // protocol violation ...
      {
         resetVars();
         continue;
      }
      data.pos++;
   }

}

/*==============================================================================*/
/* evaluate data
/*==============================================================================*/

void evaluateData(byte val)
{
   if (data.type == tpm2DataFrame)        // frame data
   {
      uint8_t* rgb = (uint8_t*)data.rgb;
      rgb[data.pos-posData] = val;
   }

   else                                  // command data
   {
      if (data.pos == posData)
      {
         data.command = val;
         memset(args, 0, sizeof(args));  
      }
      else
         args[data.pos-posData-1] = val;
   }
}

/*==============================================================================*/
/* reset variables
/*==============================================================================*/

void resetVars()
{
   memset(&data, 0, sizeof(Data));
   //data.rgb = (struct CRGB*)FastSPI_LED.getRGBData();
   memset(data.rgb, 0,  MAX_LEDS * sizeof(struct CRGB));
}

/*==============================================================================*/
/* parse packet
/*==============================================================================*/

void parsePacket()
{
   switch (data.type)
   {
      case tpm2DataFrame:
      {
         showLeds();
         break;
      }
      case tpm2Command: 
      {
         setProgram();
         break;
      }
      default: 
         break;
   }

   SERIAL.write(tpm2Ack);
   resetVars();
   data.pos = -1;
}

/*==============================================================================*/
/* set LED color
/*==============================================================================*/

void setLedColor(int led, uint8_t r, uint8_t g, uint8_t b)
{
   data.rgb[led].r = r; 
   data.rgb[led].g = g; 
   data.rgb[led].b = b; 
}

/*==============================================================================*/
/* one Color All
/*==============================================================================*/

void oneColorAll(uint8_t r, uint8_t g, uint8_t b)
{
   memset(data.rgb, 0, MAX_LEDS * sizeof(struct CRGB));

   for (int led = 0; led < MAX_LEDS; led++)
      setLedColor(led, r, g, b);

   showLeds();
   effectDelay = 1000;
}

/*==============================================================================*/
/* Output Leds
/*==============================================================================*/

void showLeds()
{
    FastLED.show();
}


void setProgram()
{
   program = data.command;
}

void playProgram()
{
   switch (program)
   {
      case  0: oneColorAll(args[0],args[1],args[2]);   break;
      case  1: rainbow_loop(20);                       break;
      default: oneColorAll(0,0,0);        break;
   }
}

/* Set LED Color of given LED */

void oneColorAllNOSHOW(int r, int g, int b)
{
   resetVars();
   for (int led = 0; led < MAX_LEDS; led++)
   {
      setLedColor(led, r, g, b);
   }

}

/*==============================================================================*/
/* Effect 0: Fixed color - Arguments RR GG BB
/*==============================================================================*/

/*==============================================================================*/
/* Effect 1: Loops rainbow colors around the stripe
/*==============================================================================*/

void rainbow_loop(int idelay) { //-LOOP HSV RAINBOW
   static double idex = 0;
   static double ihue = 0;
   double steps = (double)255/MAX_LEDS; 
   
   for(int led = 0 ; led < MAX_LEDS; led++ ) {
      ihue = led * steps + idex;
      if (ihue >= 255) 
         ihue -= 255;

      data.rgb[led] = CHSV((int)ihue, 255, 255);

      if (led == 0)
         idex += steps;
      if (idex >= 255) 
         idex = 0;
   }  
   showLeds();
   effectDelay = idelay;
}
