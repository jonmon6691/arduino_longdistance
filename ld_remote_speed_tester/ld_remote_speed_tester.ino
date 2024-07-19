// Special server code to explore how fast the client is sending packets.

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

#define N_SAMPLES 100
int n_packets;
unsigned long t_first_packet_ms;

void setup() {
  n_packets = 0;
  t_first_packet_ms = millis();

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
      digitalWrite(LED_BUILTIN, HIGH);

      n_packets += 1;
      if (n_packets == 1) t_first_packet_ms = millis();
      else if (n_packets > N_SAMPLES) {
        n_packets = 0;
        Serial.println(millis() - t_first_packet_ms);
      }
    } else {
      Serial.println("recv failed");
      digitalWrite(LED_BUILTIN, LOW);
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
