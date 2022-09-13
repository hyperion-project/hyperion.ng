// AtmoOrb by Lightning303 & Rick164, Additions by Lord-Grey
//
// ESP8266 Standalone Version
//
//
// You may change the settings that are commented

#define FASTLED_ALLOW_INTERRUPTS 0
// To make sure that all leds get changed 100% of the time, we need to allow FastLED to disabled interrupts for a short while.
// If you experience problems, please set this value to 1.
// This is only needed for 3 wire (1 data line + Vcc and GND) chips (e.g. WS2812B). If you are using WS2801, APA102 or similar chipsets, you can set the value back to 1.

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>

#define NUM_LEDS 24 // Number of leds
#define DATA_PIN D7 // Data pin for leds (the default pin 7 might correspond to pin 13 on some boards)
#define SERIAL_DEBUG 0 // Serial debugging (0=Off, 1=On)

#define ID 1 // Id of this lamp

// Smoothing
#define SMOOTH_STEPS 20 // Steps to take for smoothing colors
#define SMOOTH_DELAY 10 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

// Startup color
#define STARTUP_RED 255 // Color shown directly after power on
#define STARTUP_GREEN 175 // Color shown directly after power on
#define STARTUP_BLUE 100 // Color shown directly after power on

// White adjustment
#define RED_CORRECTION 220 // Color Correction
#define GREEN_CORRECTION 255 // Color Correction
#define BLUE_CORRECTION 180 // Color Correction

// RC Switch
#define RC_SWITCH 0 // RF transmitter to swtich remote controlled power sockets (0=Off, 1=On)
#if RC_SWITCH == 1
  #include <RCSwitch.h>
  #define RC_PIN 2 // Data pin for RF transmitter
  #define RC_SLEEP_DELAY 900000 // Delay until RF transmitter send signals
  char* rcCode0 = "10001"; // First part of the transmission code
  char* rcCode1 = "00010"; // Second part of the transmission code
  RCSwitch mySwitch = RCSwitch();
  boolean remoteControlled = false;
#endif

// Network settings
const char* ssid = "***"; // WiFi SSID
const char* password = "***"; // WiFi password

const IPAddress multicastIP(239,255,255,250); // Multicast IP address
const int multicastPort = 49692; // Multicast port number
IPAddress    ip_null(0,0,0,0);
IPAddress    local_IP(0,0,0,0);
WiFiUDP Udp;

int          timeout    = 20000;      // wait 20 sec for successfull login
boolean      is_connect = false;      // ... not yet connected

CRGB leds[NUM_LEDS];

byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;

void setColor(byte red, byte green, byte blue);
void setSmoothColor(byte red, byte green, byte blue);
void smoothColor();
void clearSmoothColors();

void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  //FastLED.setCorrection(TypicalSMD5050);
  FastLED.setCorrection(CRGB(RED_CORRECTION, GREEN_CORRECTION, BLUE_CORRECTION));
  FastLED.showColor(CRGB(STARTUP_RED, STARTUP_GREEN, STARTUP_BLUE));

  #if RC_SWITCH == 1
    mySwitch.enableTransmit(RC_PIN);
  #endif

  #if SERIAL_DEBUG == 1
    Serial.begin(115200);
  #endif

  #if SERIAL_DEBUG == 1
    Serial.printf("Connecting to %s ", ssid);
  #endif

  // .... wait for WiFi gets valid !!!
  unsigned long tick = millis();      // get start-time for login
  WiFi.begin(ssid, password);
  while ( (!is_connect) &&  ((millis() - tick) < timeout) )
  {
    yield();                          // ... for safety
    is_connect = WiFi.status();       // connected ?
    if (!is_connect)                  // only if not yet connected !
    {
      #if SERIAL_DEBUG == 1
        Serial.print(".");              // print a dot while waiting
      #endif
      delay(50);
    }
  }
  
  if (is_connect)
  {
    #if SERIAL_DEBUG == 1
      Serial.print("after ");
      Serial.print(millis() - tick);
      Serial.println(" ms");
    #endif
    // .... wait for local_IP becomes valid !!!
    is_connect = false;
    tick = millis();      // get start-time for login
    while ( (!is_connect) &&  ((millis() - tick) < timeout) )
    {
      yield();                          // ... for safety
      local_IP = WiFi.localIP(); 
      is_connect = local_IP != ip_null;       // connected ?
      if (!is_connect)                  // only if not yet connected !
      {
        #if SERIAL_DEBUG == 1
          Serial.print(".");              // print a dot while waiting
        #endif
        delay(50);
      }
    }
    if (is_connect)
    {
      #if SERIAL_DEBUG == 1
        Serial.print("local_IP valid after ");
        Serial.print(millis() - tick);
        Serial.println(" ms");
        Serial.println("");
        Serial.print(F("Connected to "));
        Serial.println(ssid);
      #endif

       // ... now start UDP and check the result:
      is_connect = Udp.beginMulticast(local_IP, multicastIP, multicastPort);
      if (is_connect)
      {
        #if SERIAL_DEBUG == 1      
          Serial.print("Listening to Multicast at ");
          Serial.print(multicastIP);
          Serial.println(":" + String(multicastPort));
        #endif
      }
      else
      {
        #if SERIAL_DEBUG == 1      
          Serial.println(" - ERROR beginMulticast !");
        #endif
      }
    }
    else
    {
      #if SERIAL_DEBUG == 1     
        Serial.println("local_IP invalid after timeout !");
      #endif
    }
  }
  else
  {
    #if SERIAL_DEBUG == 1     
      Serial.println("- invalid after timeout !");      
    #endif
  }
}

void loop()
{
  #if SERIAL_DEBUG == 1
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(F("Lost connection to "));
      Serial.print(ssid);
      Serial.println(F("."));
      Serial.println(F("Trying to reconnect."));
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        Serial.print(F("."));
      }
      Serial.println("");
      Serial.println(F("Reconnected."));
    }
  #endif
  if (Udp.parsePacket())
  {
    byte len = Udp.available();
    byte rcvd[len];
    Udp.read(rcvd, len);

    #if SERIAL_DEBUG == 1
      Serial.print(F("UDP Packet from "));
      Serial.print(Udp.remoteIP());
      Serial.print(F(":"));
      Serial.print(Udp.remotePort());
      Serial.print(F(" to "));
      Serial.println(Udp.destinationIP());
      for (byte i = 0; i < len; i++)
      {
        Serial.print(rcvd[i]);
        Serial.print(F(" "));
      }
      Serial.println("");
    #endif
    if (len >= 8 && rcvd[0] == 0xC0 && rcvd[1] == 0xFF && rcvd[2] == 0xEE && (rcvd[4] == ID || rcvd[4] == 0))
    {
      switch (rcvd[3])
      {
        case 1:
          smoothStep = SMOOTH_STEPS;
          forceLedsOFF();
          break;
        case 2:
        default:
          setSmoothColor(rcvd[5], rcvd[6], rcvd[7]);
          break;
        case 4:
          setColor(rcvd[5], rcvd[6], rcvd[7]);
          smoothStep = SMOOTH_STEPS;
          break;
        case 8:
          #if SERIAL_DEBUG == 1
            Serial.print(F("Announce myself. OrbID: "));
            Serial.println(ID);
          #endif
          Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
          Udp.write(ID);
          Udp.endPacket();
          break;
        case 9:
          #if SERIAL_DEBUG == 1
            Serial.print(F("Identify myself. OrbID: "));
            Serial.println(ID);
          #endif
          identify();
          break;
      }
    }
  }
  if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
  {
    smoothColor();
  }
  #if RC_SWITCH == 1
    if (remoteControlled && currentColor[0] == 0 && currentColor[1] == 0 && currentColor[2] == 0 && millis() >= smoothMillis + RC_SLEEP_DELAY)
    {
      // Send this signal only once every seconds
      smoothMillis += 1000;
      mySwitch.switchOff(rcCode0, rcCode1);
    }
  #endif
}

// Display color on leds
void setColor(byte red, byte green, byte blue)
{
  // Is the new color already active?
  if (currentColor[0] == red && currentColor[1] == green && currentColor[2] == blue)
  {
    return;
  }
  currentColor[0] = red;
  currentColor[1] = green;
  currentColor[2] = blue;

  FastLED.showColor(CRGB(red, green, blue));
}

// Set a new color to smooth to
void setSmoothColor(byte red, byte green, byte blue)
{
  if (smoothStep == SMOOTH_STEPS || SMOOTH_BLOCK == 0)
  {
    // Is the new color the same as the one we already are smoothing towards?
    // If so dont do anything.
    if (nextColor[0] == red && nextColor[1] == green && nextColor[2] == blue)
    {
      return;
    }
    // Is the new color the same as we have right now?
    // If so stop smoothing and keep the current color.
    else if (currentColor[0] == red && currentColor[1] == green && currentColor[2] == blue)
    {
      smoothStep = SMOOTH_STEPS;
      return;
    }

    prevColor[0] = currentColor[0];
    prevColor[1] = currentColor[1];
    prevColor[2] = currentColor[2];

    nextColor[0] = red;
    nextColor[1] = green;
    nextColor[2] = blue;

    smoothMillis = millis();
    smoothStep = 0;

    #if RC_SWITCH == 1
      if (!remoteControlled)
      {
        remoteControlled = true;
      }
    #endif
  }
}

// Display one step to the next color
void smoothColor()
{
  smoothStep++;

  byte red = prevColor[0] + (((nextColor[0] - prevColor[0]) * smoothStep) / SMOOTH_STEPS);
  byte green = prevColor[1] + (((nextColor[1] - prevColor[1]) * smoothStep) / SMOOTH_STEPS);
  byte blue = prevColor[2] + (((nextColor[2] - prevColor[2]) * smoothStep) / SMOOTH_STEPS);   

  setColor(red, green, blue);
}

// Force all leds OFF
void forceLedsOFF()
{
    setColor(0,0,0);
    clearSmoothColors();
}
 
// Clear smooth color byte arrays
void clearSmoothColors()
{
    memset(prevColor, 0, sizeof(prevColor));
    memset(currentColor, 0, sizeof(nextColor));
    memset(nextColor, 0, sizeof(nextColor));
}

void identify()
{
  for (byte i = 0; i < 3; i++)
  {
    FastLED.showColor(CRGB::LemonChiffon);
    delay(500);
    FastLED.showColor(CRGB::Black);
    delay(500);
  }
}
