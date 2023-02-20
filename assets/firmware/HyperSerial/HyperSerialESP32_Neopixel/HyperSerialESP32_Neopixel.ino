#include <NeoPixelBus.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////          CONFIG SECTION STARTS               /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define THIS_IS_RGBW                  // RGBW SK6812, otherwise comment it
#define COLD_WHITE                    // for RGBW (THIS_IS_RGBW enabled) select COLD version, comment it if NEUTRAL

const bool skipFirstLed = false;      // if set the first led in the strip will be set to black (for level shifters using sacrifice LED)
const int serialSpeed = 2000000;      // serial port speed
#define DATA_PIN 2		                // PIN: data output for LED strip

const bool reportStats = false;       // Send back processing statistics
const int  reportStatInterval_s = 10; // Send back processing every interval in seconds

/* Statistics breakdown:
    FPS: Updates to the LEDs per second
    F-FPS: Frames identified per second
    S:  Shown (Done) updates to the LEDs per given interval
    F:  Frames identified per interval (garbled grames cannot be counted)
    G:  Good frames identified per interval
    B:  Total bad frames of all types identified per interval
    BF: Bad frames identified per interval
    BS: Skipped  incomplete frames
    BC: Frames failing CRC check per interval
    BFL Frames failing Fletcher content validation per interval
*/ 

//Developer configs
#define ENABLE_STRIP
#define ENABLE_CHECK_FLETCHER

const int SERIAL_SIZE_RX = 4096;

#ifndef ENABLE_STRIP
const int serial2Speed = 460800;
const bool reportInput = false;
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////            CONFIG SECTION ENDS               /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const String version = "8.0";

#ifdef THIS_IS_RGBW
float whiteLimit = 1.0f;
#ifdef COLD_WHITE
uint8_t rCorrection = 0xA0; // adjust red   -> white in 0-0xFF range
uint8_t gCorrection = 0xA0; // adjust green -> white in 0-0xFF range
uint8_t bCorrection = 0xA0; // adjust blue  -> white in 0-0xFF range
#else
uint8_t rCorrection = 0xB0; // adjust red   -> white in 0-0xFF range
uint8_t gCorrection = 0xB0; // adjust green -> white in 0-0xFF range
uint8_t bCorrection = 0x70; // adjust blue  -> white in 0-0xFF range
#endif
#endif

int ledCount = 0;   // This is dynamic, don't change it
int pixelCount = 0; // This is dynamic, don't change it

#ifdef THIS_IS_RGBW
#define LED_TYPE NeoGrbwFeature
  #if defined(ARDUINO_LOLIN_S2_MINI)
    #define LED_METHOD NeoEsp32I2s0Sk6812Method
  #else
    #define LED_METHOD NeoEsp32I2s1Sk6812Method
  #endif
#else
#define LED_TYPE NeoGrbFeature
  #if defined(ARDUINO_LOLIN_S2_MINI)
    #define LED_METHOD NeoEsp32I2s0Ws2812xMethod
  #else
    #define LED_METHOD NeoEsp32I2s1Ws2812xMethod
  #endif
#endif

#define LED_DRIVER NeoPixelBus<LED_TYPE, LED_METHOD>

uint8_t* ledBuffer;
int ledBufferSize;

#ifdef ENABLE_STRIP
LED_DRIVER* strip = NULL;
#endif

enum class AwaProtocol
{
  HEADER_A,
  HEADER_w,
  HEADER_a,
  HEADER_HI,
  HEADER_LO,
  HEADER_CRC,
  CHANNELCALIB_GAIN,
  CHANNELCALIB_RED,
  CHANNELCALIB_GREEN,
  CHANNELCALIB_BLUE,
  PIXEL,
  FLETCHER1,
  FLETCHER2,
  FLETCHER_EXT
};

AwaProtocol state = AwaProtocol::HEADER_A;

const int headerSize = 6;
const int trailerSize = 3;
const int calibInfoSize = 4;
int bytesRead = 0;

bool isVersion2 = false;
bool isChannelCalib = false;
uint8_t CRC = 0;
int count = 0;
int currentPixel = 0;
uint16_t fletcher1 = 0;
uint16_t fletcher2 = 0;
uint16_t fletcherExt = 0;

#ifdef THIS_IS_RGBW
RgbwColor inputColor;
uint8_t wChannel[256];
uint8_t rChannel[256];
uint8_t gChannel[256];
uint8_t bChannel[256];
#else
RgbColor inputColor;
#endif

bool ledsComplete = false;

// statistics
const int reportStatInterval_ms = reportStatInterval_s * 1000;
unsigned long curTime;
unsigned long stat_start = 0;
uint16_t stat_shown = 0;
uint16_t stat_frames = 0;
uint16_t stat_good = 0;
uint16_t stat_bad = 0;

uint16_t stat_bad_frame = 0;
uint16_t stat_bad_skip = 0;
uint16_t stat_bad_crc = 0;
uint16_t stat_bad_fletcher = 0;

uint16_t stat_final_shown = 0;
uint16_t stat_final_frames = 0;
uint16_t stat_final_good = 0;
uint16_t stat_final_bad = 0;

uint16_t stat_final_bad_frame = 0;
uint16_t stat_final_bad_skip = 0;
uint16_t stat_final_bad_crc = 0;
uint16_t stat_final_bad_fletcher = 0;

//Debugging
String inputString;
String inputErrorString;
String debugString;

void printStringHex(String string)
{
#ifndef ENABLE_STRIP
  Serial2.println(string.length());
  for (int i = 0; i < string.length(); ++i)
  {
    if (i % 36 == 0)
    {
      Serial2.println();
      Serial2.print("[");
      Serial2.print(i);
      Serial2.print("] ");
    }

    if (string[i] < 16)
      Serial2.print("0");
    Serial2.print(string[i], HEX);
    Serial2.print(":");
  }
#endif
}

inline void showMe()
{
#ifdef ENABLE_STRIP
  if (strip != NULL && strip->CanShow())
  {
    stat_shown++;
    strip->Show();
  }
#endif
}

// statistics
inline void showStats()
{
  if (reportStats)
  {
    if (stat_frames > 0)
    {
      stat_final_shown = stat_shown;
      stat_final_frames = stat_frames;
      stat_final_good = stat_good;
      stat_final_bad = stat_bad;

      stat_final_bad_frame = stat_bad_frame;
      stat_final_bad_skip = stat_bad_skip;
      stat_final_bad_crc = stat_bad_crc;
      stat_final_bad_fletcher = stat_bad_fletcher;
    }

    stat_start = curTime;
    stat_shown = 0;
    stat_frames = 0;
    stat_good = 0;
    stat_bad = 0;

    stat_bad_frame = 0;
    stat_bad_skip = 0;
    stat_bad_crc = 0;
    stat_bad_fletcher = 0;

    String summary = String("FPS: ") + (stat_final_shown / reportStatInterval_s) +
      " F-FPS: " + (stat_final_frames / reportStatInterval_s) +
      " S: " + stat_final_shown +
      " F: " + stat_final_frames +
      " G: " + stat_final_good +
      " B: " + stat_final_bad +
      " (BF: " + stat_final_bad_frame +
      " BS: " + stat_final_bad_skip +
      " BC: " + stat_final_bad_crc +
      " BFL: " + stat_final_bad_fletcher +
      ")";
#ifdef ENABLE_STRIP
    Serial.println(summary);
#else
    Serial2.println(summary);
#endif
  }
}

void InitLeds(uint16_t ledCount, int pixelCount, bool channelCalibration = false)
{
  if (ledBuffer != NULL)
    delete ledBuffer;

  ledBufferSize = pixelCount + (channelCalibration ? calibInfoSize : 0);
  ledBuffer = new uint8_t[ledBufferSize];

#ifdef ENABLE_STRIP
  if (strip != NULL)
    delete strip;

  strip = new LED_DRIVER(ledCount, DATA_PIN);
  strip->Begin();
#endif
}

inline void processSerialData()
{
  while (Serial.available()) {

    char input = Serial.read();
    ++bytesRead;

#ifndef ENABLE_STRIP
    if (reportInput)
      inputString += input;
#endif

    switch (state)
    {
    case AwaProtocol::HEADER_A:
      if (input == 'A')
      {
        state = AwaProtocol::HEADER_w;
      }
      break;

    case AwaProtocol::HEADER_w:
      if (input == 'w')
        state = AwaProtocol::HEADER_a;
      else
      {
        state = AwaProtocol::HEADER_A;
      }
      break;

    case AwaProtocol::HEADER_a:
      if (input == 'a')
      {
        isVersion2 = false;
        state = AwaProtocol::HEADER_HI;
      }
      else if (input == 'A')
      {
        state = AwaProtocol::HEADER_HI;
        isVersion2 = true;
      }
      else
      {
        state = AwaProtocol::HEADER_A;
      }
      break;

    case AwaProtocol::HEADER_HI:

      stat_frames++;

      count = input << 8;

      CRC = input;
      fletcher1 = 0;
      fletcher2 = 0;
      fletcherExt = 0;
      state = AwaProtocol::HEADER_LO;
      break;

    case AwaProtocol::HEADER_LO:
      count += input + 1;

      if (ledCount != count || isChannelCalib != isVersion2)
      {
        ledCount = count;
        isChannelCalib = isVersion2;
        pixelCount = ledCount * 3;

        if (isChannelCalib)
          prepareCalibration();

        InitLeds(ledCount, pixelCount, isChannelCalib);
      }

      CRC = CRC ^ input ^ 0x55;

      state = AwaProtocol::HEADER_CRC;

      break;

    case AwaProtocol::HEADER_CRC:

      // Check, if incomplete package information was skipped, set bytesread to headersize and skip wrong input
      if (bytesRead != headerSize)
      {
        stat_bad_skip++;
        bytesRead = headerSize;
      }

      currentPixel = 0;
      if (CRC == input)
      {
        state = AwaProtocol::PIXEL;
      }
      else
      {
        // CRC failure
        stat_bad++;
        stat_bad_crc++;

        state = AwaProtocol::HEADER_A;
      }
      break;

    case AwaProtocol::PIXEL:
      ledBuffer[currentPixel++] = input;
      if (currentPixel == pixelCount)
      {
        if (isChannelCalib)
          state = AwaProtocol::CHANNELCALIB_GAIN;
        else
          state = AwaProtocol::FLETCHER1;
      }
      break;

    case AwaProtocol::CHANNELCALIB_GAIN:
      ledBuffer[currentPixel++] = input;
      state = AwaProtocol::CHANNELCALIB_RED;
      break;

    case AwaProtocol::CHANNELCALIB_RED:
      ledBuffer[currentPixel++] = input;

      state = AwaProtocol::CHANNELCALIB_GREEN;
      break;

    case AwaProtocol::CHANNELCALIB_GREEN:
      ledBuffer[currentPixel++] = input;

      state = AwaProtocol::CHANNELCALIB_BLUE;
      break;

    case AwaProtocol::CHANNELCALIB_BLUE:
      ledBuffer[currentPixel++] = input;

      state = AwaProtocol::FLETCHER1;
      break;

    case AwaProtocol::FLETCHER1:
      fletcher1 = input;

      state = AwaProtocol::FLETCHER2;
      break;

    case AwaProtocol::FLETCHER2:
      fletcher2 = input;

      state = AwaProtocol::FLETCHER_EXT;
      break;

    case AwaProtocol::FLETCHER_EXT:
      fletcherExt = input;
      ledsComplete = true;

      state = AwaProtocol::HEADER_A;
      break;
    }
  }
}

void setup()
{
  // Init serial port
  int bufSize = Serial.setRxBufferSize(SERIAL_SIZE_RX);  
  Serial.begin(serialSpeed);
  Serial.setTimeout(50);

#ifndef ENABLE_STRIP
  Serial2.begin(serial2Speed);
  
  Serial2.println();
  Serial2.println("Welcome!");
  Serial2.println("Hyperion Awa driver " + version);
  Serial2.println("!!! Debug Output !!!");
#endif

  // Display config
  Serial.println();
  Serial.println("Welcome!");
  Serial.println("Hyperion Awa driver " + version);
  Serial.print("(Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.print(__TIME__);
  Serial.println(")");

  // first LED info
  if (skipFirstLed)
    Serial.println("First LED: disabled");
  else
    Serial.println("First LED: enabled");

  // RGBW claibration info
#ifdef THIS_IS_RGBW
#ifdef COLD_WHITE
  Serial.println("Default color mode: RGBW cold");
#else
  Serial.println("Default color mode: RGBW neutral");
#endif
  prepareCalibration();
#else
  Serial.println("Color mode: RGB");
#endif

  InitLeds(ledCount, pixelCount);
}

void prepareCalibration()
{
#ifdef THIS_IS_RGBW
  // prepare LUT calibration table, cold white is much better than "neutral" white
  for (uint32_t i = 0; i < 256; i++)
  {
    // color calibration
    float red = rCorrection * i;   // adjust red
    float green = gCorrection * i; // adjust green
    float blue = bCorrection * i;  // adjust blue

    wChannel[i] = (uint8_t)round(min(whiteLimit * i, 255.0f));
    rChannel[i] = (uint8_t)round(min(red / 0xFF, 255.0f));
    gChannel[i] = (uint8_t)round(min(green / 0xFF, 255.0f));
    bChannel[i] = (uint8_t)round(min(blue / 0xFF, 255.0f));
  }

  Serial.write("RGBW calibration. White limit(%): ");
  Serial.print(whiteLimit * 100.0f);
  Serial.write(" %, red: ");
  Serial.print(rCorrection);
  Serial.write(" , green: ");
  Serial.print(gCorrection);
  Serial.write(" , blue: ");
  Serial.print(bCorrection);
  Serial.println();
#endif
}

void loop()
{
  curTime = millis();

#ifdef __AVR__
  // nothing , USART Interrupt is implemented
  ESPserialEvent();
#else
  // ESP8266  polling
  ESPserialEvent();
#endif

  if (ledsComplete)
  {
#ifndef ENABLE_STRIP
    if (reportInput)
    {
      Serial2.println();
      Serial2.print("<input> L: ");
      printStringHex(inputString);
      Serial2.println("<\input>");
      inputString = "";

      Serial2.print("bytesRead: ");
      Serial2.print(bytesRead);
      Serial2.print(" , currentPixel: ");
      Serial2.print(currentPixel);
      Serial2.print(" ,pixelCount: ");
      Serial2.print(pixelCount);
      Serial2.println();
    }
#endif

    int frameSize = headerSize + ledBufferSize + trailerSize;

    if (bytesRead > frameSize)
    {
      //Add number of frames ignored on top of frame
      int frames = bytesRead / frameSize;
      stat_frames += frames;

      //Count frame plus frames ignored as bad frames
      int badFrames = frames + 1;
      stat_bad += badFrames;
      stat_bad_frame += badFrames;
    }
    else
    {

#ifdef ENABLE_CHECK_FLETCHER
      //Test if content is valid
      uint16_t item = 0;
      uint16_t fletch1 = 0;
      uint16_t fletch2 = 0;
      uint16_t fletchExt = 0;

      while (item < ledBufferSize)
      {
        fletch1 = (fletch1 + (uint16_t)ledBuffer[item]) % 255;
        fletch2 = (fletch2 + fletch1) % 255;
        fletcherExt = (fletcherExt + ((uint16_t)ledBuffer[item] ^ (item))) % 255;
        ++item;
      }
      if ((fletch1 == fletcher1) && (fletch2 == fletcher2) && (ledBuffer[item-1] == (fletcherExt != 0x41) ? fletcherExt : 0xaa))
      {
#endif
        stat_good++;

        uint16_t startLed = 0;
        if (skipFirstLed)
        {
#ifdef ENABLE_STRIP
          #ifdef THIS_IS_RGBW
          strip->SetPixelColor(startLed, RgbwColor(0, 0, 0, 0));
          #else
          strip->SetPixelColor(startLed, RgbColor(0, 0, 0));
          #endif
#endif
          startLed = 1;
        }

        for (uint16_t led = startLed; led < ledCount; ++led)
        {
          inputColor.R = ledBuffer[led * 3];
          inputColor.G = ledBuffer[led * 3 + 1];
          inputColor.B = ledBuffer[led * 3 + 2];

          #ifdef THIS_IS_RGBW
          inputColor.W = min(rChannel[inputColor.R],
            min(gChannel[inputColor.G],
              bChannel[inputColor.B]));
          inputColor.R -= rChannel[inputColor.W];
          inputColor.G -= gChannel[inputColor.W];
          inputColor.B -= bChannel[inputColor.W];
          inputColor.W = wChannel[inputColor.W];
          #endif
#ifdef ENABLE_STRIP
          strip->SetPixelColor(led, inputColor);
#endif
        }

        showMe();
        yield();

        #ifdef THIS_IS_RGBW
        if (isChannelCalib)
        {
          uint8_t incoming_gain = ledBuffer[pixelCount];
          uint8_t incoming_red = ledBuffer[pixelCount + 1];
          uint8_t incoming_green = ledBuffer[pixelCount + 2];
          uint8_t incoming_blue = ledBuffer[pixelCount + 3];

          float final_limit = (incoming_gain != 255) ? incoming_gain / 255.0f : 1.0f;
          if (rCorrection != incoming_red || gCorrection != incoming_green || bCorrection != incoming_blue || whiteLimit != final_limit)
          {
            rCorrection = incoming_red;
            gCorrection = incoming_green;
            bCorrection = incoming_blue;
            whiteLimit = final_limit;
            prepareCalibration();
          }
        }
        #endif

#ifdef ENABLE_CHECK_FLETCHER
      }
      else
      {
        stat_bad++;
        stat_bad_fletcher++;
      }
#endif
    }

    bytesRead = 0;
    state = AwaProtocol::HEADER_A;

    ledsComplete = false;
  }

  if ((curTime - stat_start > reportStatInterval_ms))
  {
    if (stat_frames > 0)
    {
      showStats();
    }
  }
}

#ifdef __AVR__
void serialEvent()
{
  processSerialData();
}
#elif defined(ARDUINO_ARCH_ESP8266) ||  defined(ARDUINO_ARCH_ESP32)
void ESPserialEvent()
{
  processSerialData();
}
#endif
