#include <FastLED.h>
FASTLED_USING_NAMESPACE;

SYSTEM_THREAD(ENABLED);

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

// WiFi
#define timeout 30000
#define reconnect_delay 5000

// UDP
#define SERVER_PORT 49692
#define DISCOVERY_PORT 49692
UDP client;
IPAddress multicastIP(239, 15, 18, 2);

// ORB
unsigned int orbID = 1;

#define SERIAL_DEBUG 0 // Serial debugging (0=Off, 1=On)

// LED
#define DATA_PIN    6
#define NUM_LEDS    24
CRGB leds[NUM_LEDS];

// UDP
#define BUFFER_SIZE  8 // 5 + 3 channels for 1 LED
#define BUFFER_SIZE_DISCOVERY 5
#define TIMEOUT_MS   500
uint8_t buffer[BUFFER_SIZE];
uint8_t bufferDiscovery[BUFFER_SIZE_DISCOVERY];
unsigned long lastWiFiCheck = 0;

// SMOOTHING
#define SMOOTH_STEPS 50 // Steps to take for smoothing colors
#define SMOOTH_DELAY 4 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;
unsigned long smoothMillis;

// CUSTOM COLOR CORRECTIONS
#define RED_CORRECTION 255
#define GREEN_CORRECTION 255
#define BLUE_CORRECTION 255
#include "Particle.h"

void setup()
{
     // Leds - choose one correction method
     // 1 - Custom color correction
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS).setCorrection(CRGB(RED_CORRECTION, GREEN_CORRECTION, BLUE_CORRECTION));
    
    // Set color
    //setColor(40, 21, 0);
    
    // Uncomment the below lines to dim the single built-in led to 2%
    ::RGB.control(true);
    ::RGB.brightness(2);
    ::RGB.control(false);
    
    // WiFi
    lastWiFiCheck = millis();
    initWiFi();

    // 2 - FastLED predefined color correction
    //FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
}

void initWiFi()
{
    // Delays added UDP client creation, required for WiFi reconnects as takes a bit for resources to be full available
    
    // Wait for WiFi connection
    delay(500);
    waitFor(WiFi.ready, timeout);
    delay(reconnect_delay);
    
    // Multicast UDP
    if(WiFi.ready())
    {   
        #if SERIAL_DEBUG == 1
            Serial.println("");
            Serial.print(F("Connected to "));
            Serial.println(WiFi.SSID());
            Serial.print(F("IP address: "));
            Serial.println(WiFi.localIP());
        #endif

        client.begin(SERVER_PORT);
        delay(reconnect_delay);
        client.joinMulticast(multicastIP);

        #if SERIAL_DEBUG == 1
            Serial.print(F("Listening to Multicast at "));
            Serial.print(multicastIP);
            Serial.print(F(":"));
            Serial.println(SERVER_PORT);
        #endif
    }
}

void loop(){
    if (Network.listening())
    {
        // If we are in listening mode (blinking dark blue), don't
        // output by USB serial, because it can conflict with
        // serial commands.
        return;
    }
    
    if(WiFi.ready() == false)    {
        #if SERIAL_DEBUG == 1
            Serial.print(F("Lost connection to "));
            Serial.print(WiFi.SSID());
            Serial.println(F("."));
            Serial.println(F("Trying to reconnect."));
        #endif

        initWiFi();
    }
    
    int packetSize = client.parsePacket();
    
    if(packetSize == BUFFER_SIZE){

        #if SERIAL_DEBUG == 1
            Serial.print(F("Packet size: "));
            Serial.println(packetSize);
        #endif
        client.read(buffer, BUFFER_SIZE);
        //client.flush();

        #if SERIAL_DEBUG == 1
            Serial.print(F("UDP Packet from "));
            Serial.println(client.remoteIP());
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                Serial.print(buffer[i]);
                Serial.print(F(" "));
            }
            Serial.println("");
        #endif
        unsigned int i = 0;
        
        // Look for 0xC0FFEE
        if(buffer[i++] == 0xC0 && buffer[i++] == 0xFF && buffer[i++] == 0xEE)
        {
            byte commandOptions = buffer[i++];
            byte rcvOrbID = buffer[i++];
            
            byte red =  buffer[i++];
            byte green =  buffer[i++];
            byte blue =  buffer[i++];
            
            // Command options
            // 1 = force off
            // 2 = use lamp smoothing and validate by Orb ID
            // 4 = validate by Orb ID
            // 8 = discovery
            // 9 = light-up Orb to identify by Orb ID
            if(commandOptions == 1)
            {
                // Orb ID 0 = turn off all lights
                // Otherwise turn off selectively
                if(rcvOrbID == 0 || rcvOrbID == orbID)
                {
                    smoothStep = SMOOTH_STEPS;
                    forceLedsOFF();
                }
				
                return;
            }
            else if(commandOptions == 2)
            {
                if(rcvOrbID != orbID)
                {
                    return;
                }
                
                setSmoothColor(red, green, blue);
            }
            else if(commandOptions == 4)
            {
                if(rcvOrbID != orbID)
                {
                    return;
                }
                
                smoothStep = SMOOTH_STEPS;
                setColor(red, green, blue);
				        setSmoothColor(red, green, blue);
				
				        return;
            }
            else if(commandOptions == 8)
            {
                #if SERIAL_DEBUG == 1
                  Serial.print(F("Announce myself. OrbID: "));
                  Serial.println(orbID);
                #endif
                // Respond to remote IP address with Orb ID
                IPAddress remoteIP = client.remoteIP();
                bufferDiscovery[0] = orbID;
                
                client.sendPacket(bufferDiscovery, BUFFER_SIZE_DISCOVERY, remoteIP, DISCOVERY_PORT);
                
                // Clear buffer
                memset(bufferDiscovery, 0, sizeof(bufferDiscovery));
                return;
            }
            else if(commandOptions == 9)
            {
                if(rcvOrbID == 0 || rcvOrbID == orbID)
                {
                    #if SERIAL_DEBUG == 1
                      Serial.print(F("Identify myself. OrbID: "));
                      Serial.println(orbID);
                    #endif
                    identify();
                }
                return;
            }            
        }
        
    }else if(packetSize > 0){
        // Got malformed packet
    }
    
    if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1))))
    { 
        smoothColor();
    }
}

// Set color
void setColor(byte red, byte green, byte blue)
{
    for (byte i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = CRGB(red, green, blue); 
    }
    
    FastLED.show();
}


// Set a new color to smooth to
void setSmoothColor(byte red, byte green, byte blue)
{
    if (smoothStep == SMOOTH_STEPS || SMOOTH_BLOCK == 0)
    {
        if (nextColor[0] == red && nextColor[1] == green && nextColor[2] == blue)
        {
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
    }
}

// Display one step to the next color
void smoothColor()
{
    smoothStep++;
    currentColor[0] = prevColor[0] + (((nextColor[0] - prevColor[0]) * smoothStep) / SMOOTH_STEPS);
    currentColor[1] = prevColor[1] + (((nextColor[1] - prevColor[1]) * smoothStep) / SMOOTH_STEPS);
    currentColor[2] = prevColor[2] + (((nextColor[2] - prevColor[2]) * smoothStep) / SMOOTH_STEPS);
    
    setColor(currentColor[0], currentColor[1], currentColor[2]);
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
