#include <LowPower.h>
#include <RH_RF95.h>

// ButtonSizedNodeV3
#define BUTTON_PIN     3
#define BUTTON_GND_PIN     5
#define LED_PIN     6
#define RFM95_CS    10
#define RFM95_INT   2
#define RFM95_RST   16


RH_RF95 rf95(RFM95_CS, RFM95_INT);

void init_LoRa() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  // Set up radio:
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  if (!rf95.init()) Serial.println("init failed");
  if (!rf95.setFrequency(915.0)) Serial.println("setFrequency failed");
  if (!rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128)) Serial.println("setModemConfig failed");
  // The default transmitter power is 13dBm, using PA_BOOST.
  rf95.setTxPower(20, false);

}

unsigned long t_sleep_stamp; // Holds the millis() timestamp after which the remote should sleep
#define SLEEP_TIME_MS 1000 // Sleep after the last time the button was down

void wakeup_handler() {}
void button_sleep() {
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), wakeup_handler, CHANGE);
  
  //rf95.setModeIdle();
  rf95.sleep();

  // Enter power down state with ADC and BOD module disabled.
  // Wake up when wake up pin is low.
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
  // Disable external pin interrupt on wake up pin.
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN)); 
  
  rf95.setModeTx();
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUTTON_GND_PIN, LOW);
  pinMode(BUTTON_GND_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);

  init_LoRa();

  t_sleep_stamp = millis() + SLEEP_TIME_MS;
}

void loop() {
  uint8_t data[] = "";
  if (!digitalRead(BUTTON_PIN)) {
  // if (millis() / 500 % 2) {
    rf95.send(data, 0);
    rf95.waitPacketSent();
    digitalWrite(LED_PIN, HIGH);
    t_sleep_stamp = millis() + SLEEP_TIME_MS;
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  if (millis() > t_sleep_stamp) {
    button_sleep();
    // Waiting for interrupt
    t_sleep_stamp = millis() + SLEEP_TIME_MS;
  }
}
