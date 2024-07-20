// rf95_client.pde
// -*- mode: C++ -*-

#include <SPI.h>
#include <RH_RF95.h>
#include <LowPower.h>


#if 1
// Feather M0 w/Radio
#define RFM95_CS    8
#define RFM95_INT   3
#define RFM95_RST   4
#else
// ESP32 feather w/wing (Jon's dumb wiring)
#define RFM95_CS   33  // "B"
#define RFM95_INT  15  // "C"
#define RFM95_RST  27  // "A" 
#endif

RH_RF95 rf95(RFM95_CS, RFM95_INT);

#define BUTTON 15 // Active low
unsigned long t_last_pressed; // Holds the millis() timestamp of the last time the button was down
#define SLEEP_TIME_MS 10000

void setup() 
{
  pinMode(BUTTON, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
 
  Serial.begin(9600);
  Serial.println("Remote up!");

  // Set up radio:
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  if (!rf95.init()) Serial.println("init failed");
  if (!rf95.setFrequency(915.0)) Serial.println("setFrequency failed");
  if (!rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128)) Serial.println("setModemConfig failed");
  // The default transmitter power is 13dBm, using PA_BOOST.
  rf95.setTxPower(20, false);

  t_last_pressed = millis();
  attachInterrupt(digitalPinToInterrupt(BUTTON), nop_handler, LOW);
  Serial.println("Setup finished.");
}
 
void loop()
{
  // Send a message to rf95_server
  uint8_t data[] = "";
  if(digitalRead(BUTTON) == 0) {
    rf95.send(data, 0);
    rf95.waitPacketSent();
    digitalWrite(LED_BUILTIN, HIGH);
    t_last_pressed = millis();
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  return;
  if (millis() > t_last_pressed + SLEEP_TIME_MS) {
    rf95.setModeIdle();
    LowPower.standby();
    // Waiting for interrupt
    rf95.setModeTx();
  }

}

void nop_handler(void) {}
