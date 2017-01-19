/**
 * This is a demo implemention how to use tpm2 protovol on arduino
 * 
 * code is taken from: https://github.com/JonasVanGool/TPM2-ARDUINO
 */


#include <FastLED.h>

#define START_BYTE  0xC9
#define STOP_BYTE   0x36
#define DATA_FRAME  0xDA
#define COMMAND     0xC0
#define REQ_RESP    0xAA

#define BAUDRATE    115200

#define DATA_PIN    12
#define MAX_NR_LEDS 200

CRGB leds[MAX_NR_LEDS];

enum States {
  ST_START,
  ST_PACKET_TYPE,
  ST_PAYLOAD_SIZE,
  ST_USER_DATA,
  ST_END
};

States activeState = ST_START;
uint8_t byteRead = 0;
uint8_t payloadHighByte = 0;
uint8_t payloadLowByte = 0;
int payloadSize = 0;
int bytesRead = 0;
int nrOfLeds = 0;

boolean flag = false;
void setup() {
  // Init leds
  for (int i = 0; i < MAX_NR_LEDS; i++)
    leds[i] = 0;
  // Start serial device
  Serial.begin(BAUDRATE);
  // Set time out for readBytes function
  Serial.setTimeout(100);
  // init fastLED Library
  LEDS.addLeds<WS2812B, DATA_PIN, RGB>(leds, MAX_NR_LEDS);
  // setting brightness to 50% brightness
  LEDS.setBrightness(128);
  // debug led
  pinMode(13, OUTPUT);


  for(;;) {
    if (Serial.available() > 0) {
      // process TPM2 protocol
      switch (activeState) {
        //------------------------------//
        //         START                //
        //------------------------------//
        case ST_START:
          // read incomming byte
          byteRead = Serial.read();
          if (byteRead == START_BYTE) {
            activeState = ST_PACKET_TYPE;
          }
          break;
        //------------------------------//
        //         PACKET_TYPE          //
        //------------------------------//
        case ST_PACKET_TYPE:
          // read incomming byte
          byteRead = Serial.read();
          if (byteRead == DATA_FRAME) {
            activeState = ST_PAYLOAD_SIZE;
          } else {
            activeState = ST_START;
            Serial.flush();
          }
          break;
        //------------------------------//
        //         PAYLOAD_SIZE         //
        //------------------------------//
        case ST_PAYLOAD_SIZE:
          payloadHighByte = Serial.read();
          while (Serial.available() == 0) {}
          payloadLowByte = Serial.read();
          payloadSize = (payloadHighByte << 8) + payloadLowByte;
          nrOfLeds = payloadSize / 3;
          if (nrOfLeds <= MAX_NR_LEDS) {
            activeState = ST_USER_DATA;
          } else {
            activeState = ST_START;
            Serial.flush();
          }
          break;
        //------------------------------//
        //         USER_DATA            //
        //------------------------------//
        case ST_USER_DATA:
          bytesRead = Serial.readBytes((char *)leds, payloadSize);
          LEDS.show();
          if (bytesRead == payloadSize) {
            activeState = ST_END;
          } else {
            activeState = ST_START;
            Serial.flush();
          }
          break;
        //------------------------------//
        //         END                  //
        //------------------------------//
        case ST_END:
          // read incomming byte
          byteRead = Serial.read();
          if (byteRead == STOP_BYTE) {
            activeState = ST_START;
          } else {
            activeState = ST_START;
            Serial.flush();
          }
          break;
        default: break;
      }
    }

  }
}

void loop() {
}
