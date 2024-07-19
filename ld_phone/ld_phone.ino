// rf95_server.pde
// -*- mode: C++ -*-

#include <SPI.h>
#include <RH_RF95.h>

#if 0
// Feather M0 w/Radio
#define RFM95_CS 8
#define RFM95_INT 3
#define RFM95_RST 4
#else
// ESP32 feather w/wing (Jon's dumb wiring)
#define RFM95_CS 33   // "B"
#define RFM95_INT 15  // "C"
#define RFM95_RST 27  // "A"
#endif

RH_RF95 rf95(RFM95_CS, RFM95_INT);

#define ACTIVATION_MS 20  // Minimum on-time possible (theoretically 8ms, 10ms didn't work, 20 is plenty responsive and seems to work fine)
unsigned long t_activation_until;

void setup() {
  t_activation_until = 0;

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(9600);
  Serial.println("Server up");

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  if (!rf95.init()) Serial.println("init failed");
  if (!rf95.setFrequency(915.0)) Serial.println("setFrequency failed");
  if (!rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128)) Serial.println("setModemConfig failed");

  // The default transmitter power is 13dBm, using PA_BOOST.
  rf95.setTxPower(20, false);
  Serial.println("Setup finished.");
}


void loop() {
  if (rf95.available()) {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      t_activation_until = millis() + ACTIVATION_MS;
    }
  }

  if (millis() < t_activation_until) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
