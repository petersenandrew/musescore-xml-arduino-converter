// Acknowledgments
// Music player with RFID song selection
// RFID code based on https://github.com/miguelbalboa/rfid

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// SONG_DATA_BEGIN
typedef struct { uint16_t t; uint16_t d; uint8_t n; } Event;

typedef struct {
  const Event* part1;
  uint16_t part1_len;
  const Event* part2;
  uint16_t part2_len;
  uint16_t ticksPerQuarter;
  uint16_t tempoBPM;
  uint8_t timeBeats;
  uint8_t timeBeatType;
  uint32_t totalTicks;
} Song;

const uint16_t NOTE_FREQS[] PROGMEM = {
  // Frequencies will be populated when you add songs
};

// ===== SONG ARRAY =====
const Song SONGS[] = {
  // Add songs using: node converter.js song.musicxml --append arduino.ino
};
const uint8_t NUM_SONGS = sizeof(SONGS) / sizeof(SONGS[0]);

// RFID UID to Song mapping
typedef struct {
  byte uid[4];
  int8_t songIndex;
  const char* title;
  const char* mood;
} RFIDMapping;

// Map your RFID cards to songs - RFID 1 plays Song 1, RFID 2 plays Song 2, etc.
const RFIDMapping RFID_MAPPINGS[] = {

  // Add your RFID mappings here
  // Format: { { 0xXX, 0xXX, 0xXX, 0xXX }, songIndex, "Song Name", "Mood" },
  { { 0xE2, 0x83, 0x50, 0x20 }, 0, "Olymic Fanfare", "Euphoric" },
  { { 0x86, 0x06, 0xCA, 0x30 }, 1, "Morning Mood", "Calm" },
  { { 0x86, 0x8E, 0x5A, 0x30 }, 2, "William Tell Overture", "Energized" },
  { { 0x09, 0x63, 0x15, 0xC1 }, 3, "Home on the Range", "Melancholic" },
  { { 0x1B, 0x79, 0xF3, 0x00 }, 4, "crossing field", "Intense" },

};
const uint8_t NUM_RFID_MAPPINGS = sizeof(RFID_MAPPINGS) / sizeof(RFID_MAPPINGS[0]);

// Playback configuration
const uint8_t SONG_TO_PLAY = 255; // 255 = RFID mode, 0-253 = play specific song once, 254 = play all sequentially
// SONG_DATA_END

// RFID Pins (Arduino Mega)
#define RST_PIN 5  // Moved near SPI pins for cleaner wiring
#define SS_PIN 53   // SPI Slave Select
Servo servo;

// Buzzer Pins
const int BZ1 = 8; // Violin
const int BZ2 = 9; // Piano Composite

// I2C LCD Display (20=SDA, 21=SCL on Arduino Mega)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27, 16 chars, 2 lines

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Global playback state
int8_t currentSongIndex = -1;
bool songActive = false;
uint32_t songStartTime = 0;
uint32_t currentPulseUs = 0;
const char* currentSongTitle = NULL;
const char* currentSongMood = NULL;

struct Player {
  const Event *events;
  uint16_t len;
  uint16_t idx;
  Event next;
  bool hasNext;
  uint32_t endTick;
  uint32_t halfUs;
  uint32_t nextFlip;
  bool level;
  uint8_t pin;
};

Player p1;
Player p2;

static Event readEvent(const Event *events, uint16_t idx) {
  Event e;
  memcpy_P(&e, &events[idx], sizeof(Event));
  return e;
}

static uint16_t readFreq(uint8_t idx) {
  return pgm_read_word(&NOTE_FREQS[idx]);
}

static void printLCDLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  if (text == NULL) {
    return;
  }

  // Keep output on one row of a 16x2 LCD.
  for (uint8_t i = 0; i < 16 && text[i] != '\0'; i++) {
    lcd.print(text[i]);
  }
}

static void initPlayer(Player &p, const Event *events, uint16_t len, uint8_t pin) {
  p.events = events;
  p.len = len;
  p.idx = 0;
  p.hasNext = len > 0;
  if (p.hasNext) {
    p.next = readEvent(events, 0);
  }
  p.endTick = 0;
  p.halfUs = 0;
  p.nextFlip = 0;
  p.level = false;
  p.pin = pin;
}

static void startEvent(Player &p, uint32_t nowUs) {
  uint16_t freq = readFreq(p.next.n);
  p.endTick = (uint32_t)p.next.t + (uint32_t)p.next.d;
  if (freq == 0) {
    p.halfUs = 0;
    digitalWrite(p.pin, LOW);
  } else {
    p.halfUs = 500000UL / freq;
    p.nextFlip = nowUs + p.halfUs;
  }

  p.idx++;
  if (p.idx < p.len) {
    p.next = readEvent(p.events, p.idx);
  } else {
    p.hasNext = false;
  }
}

int8_t findSongByRFID(byte *uid) {
  for (uint8_t i = 0; i < NUM_RFID_MAPPINGS; i++) {
    bool match = true;
    for (uint8_t j = 0; j < 4; j++) {
      if (RFID_MAPPINGS[i].uid[j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      currentSongTitle = RFID_MAPPINGS[i].title;
      currentSongMood = RFID_MAPPINGS[i].mood;
      return RFID_MAPPINGS[i].songIndex;
    }
  }
  currentSongTitle = NULL;
  currentSongMood = NULL;
  return -1;
}

void startSong(uint8_t songIndex) {
  if (songIndex >= NUM_SONGS) return;
  
  // Stop any currently playing song
  digitalWrite(BZ1, LOW);
  digitalWrite(BZ2, LOW);
  
  const Song &song = SONGS[songIndex];
  currentSongIndex = songIndex;
  songActive = true;
  songStartTime = micros();
  currentPulseUs = 60000000UL / (song.tempoBPM * song.ticksPerQuarter);
  
  // Initialize players for this song
  initPlayer(p1, song.part1, song.part1_len, BZ1);
  initPlayer(p2, song.part2, song.part2_len, BZ2);
  
  Serial.print(F("Playing song "));
  Serial.println(songIndex);
  
  // Update LCD display
  lcd.clear();
  if (currentSongTitle != NULL) {
    printLCDLine(0, currentSongTitle);
  } else {
    printLCDLine(0, "Unknown Title");
  }

  if (currentSongMood != NULL) {
    printLCDLine(1, currentSongMood);
  } else {
    printLCDLine(1, "Mood Unknown");
  }
}

void stopSong() {
  songActive = false;
  currentSongIndex = -1;
  digitalWrite(BZ1, LOW);
  digitalWrite(BZ2, LOW);
  p1.halfUs = 0;
  p2.halfUs = 0;
  
  // Update LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Song Finished"));
  lcd.setCursor(0, 1);
  lcd.print(F("Ready for next"));
}

void updatePlayback() {
  if (!songActive || currentSongIndex < 0) return;
  
  const Song &song = SONGS[currentSongIndex];
  uint32_t nowUs = micros() - songStartTime;
  uint32_t currTick = nowUs / currentPulseUs;

  // Check if song is finished
  if (currTick > song.totalTicks && !p1.hasNext && !p2.hasNext && p1.halfUs == 0 && p2.halfUs == 0) {
    stopSong();
    Serial.println(F("Song finished"));
    servo.write(170);
    delay(15000);
    servo.write(90);
    return;
  }

  // Start new events
  if (p1.hasNext && currTick >= p1.next.t) {
    startEvent(p1, nowUs);
  }
  if (p2.hasNext && currTick >= p2.next.t) {
    startEvent(p2, nowUs);
  }

  // Update buzzer 1
  if (p1.halfUs > 0) {
    if (currTick >= p1.endTick) {
      p1.halfUs = 0;
      digitalWrite(p1.pin, LOW);
    } else if (nowUs >= p1.nextFlip) {
      p1.level = !p1.level;
      digitalWrite(p1.pin, p1.level);
      p1.nextFlip += p1.halfUs;
    }
  }

  // Update buzzer 2
  if (p2.halfUs > 0) {
    if (currTick >= p2.endTick) {
      p2.halfUs = 0;
      digitalWrite(p2.pin, LOW);
    } else if (nowUs >= p2.nextFlip) {
      p2.level = !p2.level;
      digitalWrite(p2.pin, p2.level);
      p2.nextFlip += p2.halfUs;
    }
  }
}

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Print UID for debugging
  Serial.print(F("Card UID:"));
  for (byte i = 0; i < 4; i++) {
    Serial.print(F(" 0x"));
    if (mfrc522.uid.uidByte[i] < 0x10) Serial.print(F("0"));
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Find matching song
  int8_t songIndex = findSongByRFID(mfrc522.uid.uidByte);
  
  if (songIndex >= 0 && songIndex < NUM_SONGS) {
    // Switch to this song (even if currently playing another)
    startSong(songIndex);
  } else {
    Serial.println(F("Unknown card - no song mapped"));
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void playSongOnce(uint8_t songIndex) {
  if (songIndex >= NUM_SONGS) return;
  
  const Song &song = SONGS[songIndex];
  const uint32_t PULSE_US = 60000000UL / (song.tempoBPM * song.ticksPerQuarter);
  
  Player p1_local;
  Player p2_local;
  initPlayer(p1_local, song.part1, song.part1_len, BZ1);
  initPlayer(p2_local, song.part2, song.part2_len, BZ2);

  uint32_t start = micros();

  while (true) {
    uint32_t nowUs = micros() - start;
    uint32_t currTick = nowUs / PULSE_US;

    if (currTick > song.totalTicks && !p1_local.hasNext && !p2_local.hasNext && p1_local.halfUs == 0 && p2_local.halfUs == 0) {
      break;
    }

    if (p1_local.hasNext && currTick >= p1_local.next.t) {
      startEvent(p1_local, nowUs);
    }
    if (p2_local.hasNext && currTick >= p2_local.next.t) {
      startEvent(p2_local, nowUs);
    }

    if (p1_local.halfUs > 0) {
      if (currTick >= p1_local.endTick) {
        p1_local.halfUs = 0;
        digitalWrite(p1_local.pin, LOW);
      } else if (nowUs >= p1_local.nextFlip) {
        p1_local.level = !p1_local.level;
        digitalWrite(p1_local.pin, p1_local.level);
        p1_local.nextFlip += p1_local.halfUs;
      }
    }

    if (p2_local.halfUs > 0) {
      if (currTick >= p2_local.endTick) {
        p2_local.halfUs = 0;
        digitalWrite(p2_local.pin, LOW);
      } else if (nowUs >= p2_local.nextFlip) {
        p2_local.level = !p2_local.level;
        digitalWrite(p2_local.pin, p2_local.level);
        p2_local.nextFlip += p2_local.halfUs;
      }
    }
  }

  digitalWrite(BZ1, LOW);
  digitalWrite(BZ2, LOW);
}

void setup() {
  pinMode(BZ1, OUTPUT);
  pinMode(BZ2, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  pinMode(SS_PIN, OUTPUT);
  pinMode(51, OUTPUT); // MOSI (Mega)
  pinMode(50, INPUT);  // MISO (Mega)
  pinMode(52, OUTPUT); // SCK (Mega)
  servo.attach(12);
  servo.write(100);
  
  // Initialize I2C LCD (SDA=20, SCL=21 on Arduino Mega)
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("Music Player"));
  lcd.setCursor(0, 1);
  lcd.print(F("Initializing..."));
  delay(1000);
  
  Serial.begin(9600);
  
  if (SONG_TO_PLAY == 255) {
    // RFID mode - continuous monitoring and playback
    while (!Serial);
    
    // Initialize SPI
    SPI.begin();
    
    // Initialize MFRC522
    mfrc522.PCD_Init();
    
    // Initialize key
    for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
    }
    
    mfrc522.PCD_AntennaOn();
    
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    Serial.print(F("MFRC522 Version: 0x"));
    Serial.println(v, HEX);
    
    if (v == 0x00 || v == 0xFF) {
      Serial.println(F("ERROR: Communication failure!"));
      Serial.println(F("Check wiring (Arduino Mega):"));
      Serial.println(F("  MISO -> D50"));
      Serial.println(F("  MOSI -> D51"));
      Serial.println(F("  SCK  -> D52"));
      Serial.println(F("  SS   -> D53"));
      Serial.println(F("  RST  -> D5"));
      Serial.println(F("  VCC  -> 3.3V (NOT 5V!)"));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("RFID ERROR!"));
      lcd.setCursor(0, 1);
      lcd.print(F("Check wiring"));
      while(1); // Halt
    }
    
    Serial.println(F("RFID Music Player Ready"));
    Serial.println(F("Scan a card to play a song..."));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Scan RFID Card"));
    lcd.setCursor(0, 1);
    lcd.print(F("to play music"));
    
  } else if (SONG_TO_PLAY == 254) {
    // Play all songs sequentially
    for (uint8_t i = 0; i < NUM_SONGS; i++) {
      playSongOnce(i);
      delay(1000);
    }
  } else if (SONG_TO_PLAY < NUM_SONGS) {
    // Play specific song
    playSongOnce(SONG_TO_PLAY);
    Serial.println("test");
  }
}

void loop() {
  if (SONG_TO_PLAY == 255) {
    // RFID mode - check for interrupts ONLY when not actively playing to keep audio clean
    if (!songActive) {
      checkRFID();
    }
    updatePlayback();
  }
  // Otherwise do nothing (blocking modes finish in setup())
}