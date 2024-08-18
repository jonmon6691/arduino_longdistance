// rf95_server.pde
// -*- mode: C++ -*-

#include <SPI.h>
#include <RH_RF95.h>
#include <Adafruit_Soundboard.h>

#define RINGER_PWR 26
#define AFX_PWR 25
#define AFX_ACT 32
#define AFX_RST 12
#define SW_OFF_HOOK 14
#define SW_DIAL 22

// ESP32 feather w/wing (Jon's dumb wiring)
#define RFM95_CS 33   // "B"
#define RFM95_INT 15  // "C"
#define RFM95_RST 27  // "A"

// Not used since they are implicit in the Arduino-provided "Serial1" object
#define AFX_TX 16
#define AFX_RX 17

#define MAX_AFX_RETRIES 1
#define AFX_STATE_OFF 0
#define AFX_STATE_INIT 1
#define AFX_STATE_UP 2
#define AFX_STATE_MISSING 3
uint8_t afx_state;

RH_RF95 rf95(RFM95_CS, RFM95_INT);
Adafruit_Soundboard afx = Adafruit_Soundboard(&Serial1, NULL, AFX_RST);

#define ACTIVATION_MS 20  // Minimum on-time possible (theoretically 8ms, 10ms didn't work, 20 is plenty responsive and seems to work fine)
unsigned long t_ring_until;
unsigned long t_awake_until;

// Stores the number of files, and incidentally, the length of *playlist
uint8_t num_files;

// Stores the order to play the song's in, by index
uint8_t *playlist;

// Stores the track to be played
uint8_t track_i;

void init_phone() {
  digitalWrite(RINGER_PWR, LOW); pinMode(RINGER_PWR, OUTPUT);
  pinMode(SW_OFF_HOOK, INPUT_PULLUP);
  pinMode(SW_DIAL, INPUT_PULLUP);
  t_ring_until = 0;
}

void init_LoRa() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  if (!rf95.init()) Serial.println("init failed");
  if (!rf95.setFrequency(915.0)) Serial.println("setFrequency failed");
  if (!rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128)) Serial.println("setModemConfig failed");

  // The default transmitter power is 13dBm, using PA_BOOST.
  rf95.setTxPower(20, false);
}

void init_audio() {
  afx_state = AFX_STATE_OFF;
  pinMode(AFX_PWR, OUTPUT); digitalWrite(AFX_PWR, HIGH);
  afx_state = AFX_STATE_INIT;
  pinMode(AFX_ACT, INPUT_PULLUP);

  // Sound board talks on Serial1
  Serial1.begin(9600);
  for (uint8_t i = 0; i < MAX_AFX_RETRIES; i++) {
    if (afx.reset()) {
      afx_state = AFX_STATE_UP;
      break;
    } else {
      afx_state = AFX_STATE_MISSING;
    }
  }

  num_files = afx.listFiles();
  playlist = (uint8_t *) malloc(sizeof(uint8_t) * num_files);
  track_i = 0;

  // Initialize playlist
  for (int i = 0; i < num_files; i++) playlist[i] = i;
  shuffle_playlist();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(9600);
  Serial.println("Server up");

  init_phone();
  init_LoRa();
  init_audio();

  t_awake_until = millis() + 2000;

  Serial.println("Setup finished.");
  digitalWrite(LED_BUILTIN, LOW);
}

void shuffle_playlist() {
  // Fisher-yates and Durstenfeld with the good shuffle
  // for i from n−1 down to 1 do
  for (int i = num_files - 1; i > 0; i--) {
    // j gets random integer such that 0 <= j <= i
    int j = random(i+1); 
    // exchange [j] and [i]
    uint8_t tmp = playlist[i];
    playlist[i] = playlist[j];
    playlist[j] = tmp;
  }
}

// returns the new state
uint8_t handle_audio(uint8_t in_state) {
  uint8_t power = digitalRead(AFX_PWR) == HIGH;
  switch (in_state) {
  case AFX_STATE_OFF:
    if (power) return AFX_STATE_INIT;
    break;

  default:
  case AFX_STATE_INIT:
    if (!power) return AFX_STATE_OFF;
    if (!afx.reset()) return AFX_STATE_INIT;
    return AFX_STATE_UP;
    break;

  case AFX_STATE_UP:
    if (!power) return AFX_STATE_OFF;
    return AFX_STATE_UP;
    break;

  case AFX_STATE_MISSING:
    return AFX_STATE_MISSING;
    break;
  }
}

void loop() {
  if (rf95.available()) {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      t_ring_until = millis() + ACTIVATION_MS;
      t_awake_until = millis() + 2000;
    }
  }

  // Ring the bell
  if (millis() < t_ring_until) {
    digitalWrite(RINGER_PWR, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(RINGER_PWR, LOW);
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Power up the audio module while the ringer is off the hook
  if (digitalRead(SW_OFF_HOOK) == LOW) {
    digitalWrite(AFX_PWR, HIGH);
  } else {
    digitalWrite(AFX_PWR, LOW);
  }

  // Audio module state machine
  afx_state = handle_audio(afx_state);

  // If the ringer module is up and running, not playing already, and there's files on the board
  if (afx_state == AFX_STATE_UP && digitalRead(AFX_ACT) == HIGH && num_files > 0) {
    // Play one from the shuffled list
    afx.playTrack(playlist[track_i++]);
    // Reshuffle the playlist once we get to the end
    if (track_i >= num_files) {
      track_i = 0;
      shuffle_playlist();
    }
  }

  // Go to sleep
  if (millis() > t_awake_until) {
    esp_sleep_enable_ext0_wakeup(gpio_num_t::GPIO_NUM_15, 1);
    esp_light_sleep_start();
  }
}
