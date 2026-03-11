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
  const Event *part1;
  uint16_t part1_len;
  const Event *part2;
  uint16_t part2_len;
  uint16_t ticksPerQuarter;
  uint16_t tempoBPM;
  uint8_t timeBeats;
  uint8_t timeBeatType;
  uint32_t totalTicks;
} Song;

// Standard 88-key piano frequency table (MIDI 21-108, A0 to C8)
// DO NOT MODIFY THIS TABLE - must match converter.js
const uint16_t NOTE_FREQS[] PROGMEM = {
  28, 29, 31, 33, 35, 37, 39, 41, 44, 46,     // MIDI 21-30 (A0-D#1)
  49, 52, 55, 58, 62, 65, 69, 73, 78, 82,     // MIDI 31-40 (E1-E2)
  87, 92, 98, 104, 110, 117, 124, 131, 139, 147, // MIDI 41-50 (F2-D2)
  156, 165, 175, 185, 196, 208, 220, 233, 247, 262, // MIDI 51-60 (C2-C3)
  277, 294, 311, 330, 349, 370, 392, 415, 440, 466, // MIDI 61-70 (C#3-A#3)
  494, 523, 554, 587, 622, 659, 698, 740, 784, 831, // MIDI 71-80 (B3-A#4)
  880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, // MIDI 81-90 (A4-A#5)
  1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637  // MIDI 91-100 (D#6)
};
const uint8_t NOTE_FREQS_MIN_MIDI = 21;
const uint8_t NOTE_FREQS_COUNT = 88;

const Event PART1[] PROGMEM = {
  { 0, 8, 55 }, { 8, 4, 58 }, { 14, 2, 58 },
  { 16, 4, 51 }, { 20, 4, 53 }, { 24, 4, 55 },
  { 28, 4, 51 }, { 32, 2, 53 }, { 36, 1, 53 },
  { 39, 1, 53 }, { 40, 4, 53 }, { 44, 2, 55 },
  { 46, 2, 53 }, { 48, 1, 51 }, { 50, 1, 51 },
  { 51, 1, 53 }, { 52, 2, 55 }, { 54, 2, 51 },
  { 56, 8, 53 }, { 64, 8, 55 }, { 72, 4, 58 },
  { 78, 2, 58 }, { 80, 4, 51 }, { 84, 4, 53 },
  { 88, 4, 55 }, { 92, 4, 51 }, { 96, 2, 53 },
  { 100, 1, 53 }, { 103, 1, 53 }, { 104, 4, 53 },
  { 108, 2, 55 }, { 110, 2, 53 }, { 112, 1, 51 },
  { 114, 1, 51 }, { 115, 1, 53 }, { 116, 2, 55 },
  { 118, 2, 53 }, { 120, 8, 51 }, { 128, 8, 55 },
  { 136, 4, 58 }, { 142, 2, 58 }, { 144, 4, 51 },
  { 148, 4, 53 }, { 152, 4, 55 }, { 156, 4, 51 },
  { 160, 2, 53 }, { 164, 1, 53 }, { 167, 1, 53 },
  { 168, 4, 53 }, { 172, 2, 55 }, { 174, 2, 53 },
  { 176, 1, 51 }, { 178, 1, 51 }, { 179, 1, 53 },
  { 180, 2, 55 }, { 182, 2, 51 }, { 184, 8, 53 },
  { 192, 8, 55 }, { 200, 4, 58 }, { 206, 2, 58 },
  { 208, 4, 51 }, { 212, 4, 53 }, { 216, 4, 55 },
  { 220, 4, 51 }, { 224, 2, 53 }, { 228, 1, 53 },
  { 231, 1, 53 }, { 232, 4, 53 }, { 236, 2, 55 },
  { 238, 2, 53 }, { 240, 1, 51 }, { 242, 1, 51 },
  { 243, 1, 53 }, { 244, 2, 55 }, { 246, 2, 53 },
  { 248, 8, 51 },
};
const uint16_t PART1_LEN = sizeof(PART1) / sizeof(PART1[0]);

const Event PART2[] PROGMEM = {
  { 0, 8, 51 }, { 8, 4, 50 }, { 14, 2, 50 },
  { 16, 4, 43 }, { 20, 2, 46 }, { 24, 4, 46 },
  { 28, 4, 43 }, { 32, 2, 46 }, { 36, 1, 46 },
  { 39, 1, 46 }, { 40, 2, 46 }, { 44, 1, 46 },
  { 46, 1, 46 }, { 48, 1, 46 }, { 50, 1, 46 },
  { 51, 1, 46 }, { 52, 1, 46 }, { 54, 2, 46 },
  { 56, 8, 50 }, { 64, 8, 51 }, { 72, 4, 50 },
  { 78, 2, 50 }, { 80, 4, 43 }, { 84, 2, 46 },
  { 88, 4, 46 }, { 92, 4, 43 }, { 96, 2, 46 },
  { 100, 1, 46 }, { 103, 1, 46 }, { 104, 2, 46 },
  { 108, 1, 46 }, { 110, 1, 46 }, { 112, 1, 46 },
  { 114, 1, 46 }, { 115, 1, 46 }, { 116, 1, 46 },
  { 118, 2, 46 }, { 120, 8, 43 }, { 128, 8, 51 },
  { 136, 4, 50 }, { 142, 2, 50 }, { 144, 4, 43 },
  { 148, 2, 46 }, { 152, 4, 46 }, { 156, 4, 43 },
  { 160, 2, 46 }, { 164, 1, 46 }, { 167, 1, 46 },
  { 168, 2, 46 }, { 172, 1, 46 }, { 174, 1, 46 },
  { 176, 1, 46 }, { 178, 1, 46 }, { 179, 1, 46 },
  { 180, 1, 46 }, { 182, 2, 46 }, { 184, 8, 50 },
  { 192, 8, 51 }, { 200, 4, 50 }, { 206, 2, 50 },
  { 208, 4, 43 }, { 212, 2, 46 }, { 216, 4, 46 },
  { 220, 4, 43 }, { 224, 2, 46 }, { 228, 1, 46 },
  { 231, 1, 46 }, { 232, 2, 46 }, { 236, 1, 46 },
  { 238, 1, 46 }, { 240, 1, 46 }, { 242, 1, 46 },
  { 243, 1, 46 }, { 244, 1, 46 }, { 246, 2, 46 },
  { 248, 8, 43 },
};
const uint16_t PART2_LEN = sizeof(PART2) / sizeof(PART2[0]);



// ===== SONG 2 DATA =====

const Event PART3[] PROGMEM = {
  { 0, 2, 51 }, { 2, 2, 48 }, { 4, 2, 46 },
  { 6, 2, 44 }, { 8, 2, 46 }, { 10, 2, 48 },
  { 12, 2, 51 }, { 14, 2, 48 }, { 16, 2, 46 },
  { 18, 2, 44 }, { 20, 1, 46 }, { 21, 1, 48 },
  { 22, 1, 46 }, { 23, 1, 48 }, { 24, 2, 51 },
  { 26, 2, 48 }, { 28, 2, 51 }, { 30, 2, 53 },
  { 32, 2, 48 }, { 34, 2, 53 }, { 36, 2, 51 },
  { 38, 2, 48 }, { 40, 2, 46 }, { 42, 6, 44 },
  { 48, 10, 51 }, { 60, 10, 51 }, { 72, 6, 51 },
  { 78, 6, 48 }, { 84, 12, 52 }, { 96, 2, 55 },
  { 98, 2, 52 }, { 100, 2, 50 }, { 102, 2, 48 },
  { 104, 2, 50 }, { 106, 2, 52 }, { 108, 2, 55 },
  { 110, 2, 52 }, { 112, 2, 50 }, { 114, 2, 48 },
  { 116, 1, 50 }, { 117, 1, 52 }, { 118, 1, 50 },
  { 119, 1, 52 }, { 120, 2, 55 }, { 122, 2, 52 },
  { 124, 2, 55 }, { 126, 2, 56 }, { 128, 2, 53 },
  { 130, 2, 56 }, { 132, 2, 55 }, { 134, 2, 52 },
  { 136, 2, 50 }, { 138, 4, 48 }, { 144, 10, 55 },
  { 156, 10, 55 }, { 168, 6, 55 }, { 174, 6, 53 },
  { 180, 12, 51 }, { 192, 2, 58 }, { 194, 2, 55 },
  { 196, 2, 53 }, { 198, 2, 51 }, { 200, 1, 53 },
  { 201, 1, 55 }, { 202, 1, 53 }, { 203, 1, 55 },
  { 204, 10, 58 }, { 216, 2, 58 }, { 218, 2, 55 },
  { 220, 1, 51 }, { 222, 6, 51 }, { 228, 2, 58 },
  { 230, 2, 55 }, { 232, 1, 51 }, { 234, 4, 51 },
  { 240, 2, 51 }, { 242, 2, 48 }, { 244, 2, 46 },
  { 246, 2, 44 }, { 248, 2, 46 }, { 250, 2, 48 },
  { 252, 2, 51 }, { 254, 2, 48 }, { 256, 2, 46 },
  { 258, 2, 44 }, { 260, 1, 46 }, { 261, 1, 48 },
  { 262, 1, 46 }, { 263, 1, 48 }, { 264, 6, 51 },
  { 270, 6, 53 }, { 276, 6, 51 }, { 282, 6, 53 },
  { 288, 12, 51 }, { 300, 12, 53 }, { 312, 12, 56 },
};
const uint16_t PART3_LEN = sizeof(PART3) / sizeof(PART3[0]);

const Event PART4[] PROGMEM = {
  { 0, 10, 39 }, { 12, 10, 39 }, { 24, 6, 39 },
  { 30, 6, 41 }, { 36, 10, 39 }, { 48, 2, 39 },
  { 50, 2, 36 }, { 52, 2, 34 }, { 54, 2, 32 },
  { 56, 2, 34 }, { 58, 2, 36 }, { 60, 2, 39 },
  { 62, 2, 36 }, { 64, 2, 34 }, { 66, 2, 32 },
  { 68, 1, 34 }, { 69, 1, 36 }, { 70, 1, 34 },
  { 71, 1, 36 }, { 72, 2, 39 }, { 74, 2, 36 },
  { 76, 2, 39 }, { 78, 2, 41 }, { 80, 2, 36 },
  { 82, 2, 41 }, { 84, 2, 43 }, { 86, 2, 40 },
  { 88, 2, 38 }, { 90, 4, 36 }, { 96, 10, 43 },
  { 108, 10, 43 }, { 120, 6, 43 }, { 126, 6, 44 },
  { 132, 10, 43 }, { 144, 2, 43 }, { 146, 2, 40 },
  { 148, 2, 38 }, { 150, 2, 36 }, { 152, 2, 38 },
  { 154, 2, 40 }, { 156, 2, 43 }, { 158, 2, 40 },
  { 160, 2, 38 }, { 162, 2, 36 }, { 164, 1, 38 },
  { 165, 1, 40 }, { 166, 1, 38 }, { 167, 1, 40 },
  { 168, 2, 43 }, { 170, 2, 40 }, { 172, 2, 43 },
  { 174, 2, 44 }, { 176, 2, 41 }, { 178, 2, 44 },
  { 180, 2, 46 }, { 182, 2, 43 }, { 184, 2, 41 },
  { 186, 4, 39 }, { 192, 10, 46 }, { 204, 2, 46 },
  { 206, 2, 43 }, { 208, 2, 41 }, { 210, 2, 39 },
  { 212, 1, 41 }, { 213, 1, 43 }, { 214, 1, 41 },
  { 215, 1, 43 }, { 216, 4, 46 }, { 222, 2, 46 },
  { 224, 2, 43 }, { 226, 2, 39 }, { 228, 6, 46 },
  { 234, 2, 34 }, { 236, 2, 31 }, { 238, 2, 27 },
  { 240, 4, 36 }, { 246, 4, 36 }, { 252, 4, 36 },
  { 258, 6, 36 }, { 264, 2, 39 }, { 266, 2, 36 },
  { 268, 2, 34 }, { 270, 2, 32 }, { 272, 2, 34 },
  { 274, 2, 36 }, { 276, 2, 39 }, { 278, 2, 36 },
  { 280, 2, 34 }, { 282, 2, 32 }, { 284, 1, 34 },
  { 285, 1, 36 }, { 286, 1, 34 }, { 287, 1, 36 },
  { 288, 12, 32 }, { 300, 12, 27 }, { 312, 12, 32 },
};
const uint16_t PART4_LEN = sizeof(PART4) / sizeof(PART4[0]);

// ===== SONG 3 DATA =====

const Event PART5[] PROGMEM = {
  { 0, 1, 41 }, { 1, 1, 41 }, { 2, 1, 41 },
  { 4, 1, 41 }, { 5, 1, 41 }, { 6, 1, 41 },
  { 8, 1, 41 }, { 9, 1, 41 }, { 10, 2, 46 },
  { 12, 2, 48 }, { 14, 2, 50 }, { 16, 1, 41 },
  { 17, 1, 41 }, { 18, 1, 41 }, { 20, 1, 41 },
  { 21, 1, 41 }, { 22, 2, 46 }, { 24, 1, 50 },
  { 25, 1, 50 }, { 26, 2, 48 }, { 28, 2, 45 },
  { 30, 1, 41 }, { 32, 1, 41 }, { 33, 1, 41 },
  { 34, 1, 41 }, { 36, 1, 41 }, { 37, 1, 41 },
  { 38, 1, 41 }, { 40, 1, 41 }, { 41, 1, 41 },
  { 42, 2, 46 }, { 44, 2, 48 }, { 46, 2, 50 },
  { 48, 1, 46 }, { 49, 1, 50 }, { 50, 2, 53 },
  { 54, 1, 53 }, { 55, 1, 51 }, { 56, 1, 50 },
  { 57, 1, 48 }, { 58, 2, 46 }, { 60, 2, 50 },
  { 62, 2, 46 }, { 64, 1, 41 }, { 65, 1, 41 },
  { 66, 1, 41 }, { 68, 1, 41 }, { 69, 1, 41 },
  { 70, 1, 41 }, { 72, 1, 41 }, { 73, 1, 41 },
  { 74, 2, 46 }, { 76, 2, 48 }, { 78, 2, 50 },
  { 80, 1, 41 }, { 81, 1, 41 }, { 82, 1, 41 },
  { 84, 1, 41 }, { 85, 1, 41 }, { 86, 2, 46 },
  { 88, 1, 50 }, { 89, 1, 50 }, { 90, 2, 48 },
  { 92, 2, 45 }, { 94, 1, 41 }, { 96, 1, 41 },
  { 97, 1, 41 }, { 98, 1, 41 }, { 100, 1, 41 },
  { 101, 1, 41 }, { 102, 1, 41 }, { 104, 1, 41 },
  { 105, 1, 41 }, { 106, 2, 46 }, { 108, 2, 48 },
  { 110, 2, 50 }, { 112, 1, 46 }, { 113, 1, 50 },
  { 114, 2, 53 }, { 118, 1, 53 }, { 119, 1, 51 },
  { 120, 1, 50 }, { 121, 1, 48 }, { 122, 2, 46 },
  { 124, 2, 50 }, { 126, 2, 46 }, { 128, 1, 50 },
  { 129, 1, 50 }, { 130, 1, 50 }, { 132, 1, 50 },
  { 133, 1, 50 }, { 134, 1, 50 }, { 136, 1, 50 },
  { 137, 1, 50 }, { 138, 2, 50 }, { 140, 2, 55 },
  { 142, 2, 50 }, { 144, 2, 55 }, { 146, 2, 50 },
  { 148, 2, 55 }, { 150, 2, 50 }, { 152, 2, 48 },
  { 154, 2, 46 }, { 156, 2, 45 }, { 158, 2, 43 },
  { 160, 1, 50 }, { 161, 1, 50 }, { 162, 1, 50 },
  { 164, 1, 50 }, { 165, 1, 50 }, { 166, 1, 50 },
  { 168, 1, 50 }, { 169, 1, 50 }, { 170, 2, 50 },
  { 172, 2, 55 }, { 174, 2, 50 }, { 176, 2, 55 },
  { 178, 2, 50 }, { 180, 2, 55 }, { 182, 2, 53 },
  { 184, 2, 52 }, { 186, 2, 53 }, { 188, 2, 52 },
  { 190, 2, 53 }, { 192, 1, 41 }, { 193, 1, 41 },
  { 194, 1, 41 }, { 196, 1, 41 }, { 197, 1, 41 },
  { 198, 1, 41 }, { 200, 1, 41 }, { 201, 1, 41 },
  { 202, 2, 46 }, { 204, 2, 48 }, { 206, 2, 50 },
  { 208, 1, 41 }, { 209, 1, 41 }, { 210, 1, 41 },
  { 212, 1, 41 }, { 213, 1, 41 }, { 214, 2, 46 },
  { 216, 1, 50 }, { 217, 1, 50 }, { 218, 2, 48 },
  { 220, 2, 45 }, { 222, 1, 41 }, { 224, 1, 41 },
  { 225, 1, 41 }, { 226, 1, 41 }, { 228, 1, 41 },
  { 229, 1, 41 }, { 230, 1, 41 }, { 232, 1, 41 },
  { 233, 1, 41 }, { 234, 2, 46 }, { 236, 2, 48 },
  { 238, 2, 50 }, { 240, 1, 46 }, { 241, 1, 50 },
  { 242, 2, 53 }, { 246, 1, 53 }, { 247, 1, 51 },
  { 248, 1, 50 }, { 249, 1, 48 }, { 250, 2, 46 },
  { 252, 2, 50 }, { 254, 4, 46 },
};
const uint16_t PART5_LEN = sizeof(PART5) / sizeof(PART5[0]);

const Event PART6[] PROGMEM = {
  { 2, 2, 38 }, { 6, 2, 38 }, { 10, 2, 41 },
  { 12, 2, 45 }, { 14, 2, 46 }, { 18, 2, 38 },
  { 22, 2, 38 }, { 26, 2, 45 }, { 28, 1, 41 },
  { 30, 2, 41 }, { 34, 2, 38 }, { 38, 2, 38 },
  { 42, 2, 41 }, { 44, 2, 45 }, { 46, 1, 46 },
  { 48, 1, 46 }, { 49, 1, 46 }, { 50, 2, 45 },
  { 54, 1, 45 }, { 56, 1, 45 }, { 57, 1, 45 },
  { 58, 1, 46 }, { 60, 2, 46 }, { 62, 2, 41 },
  { 66, 2, 38 }, { 70, 2, 38 }, { 74, 2, 41 },
  { 76, 2, 45 }, { 78, 2, 46 }, { 82, 2, 38 },
  { 86, 2, 38 }, { 90, 2, 45 }, { 92, 1, 41 },
  { 94, 2, 41 }, { 98, 2, 38 }, { 102, 2, 38 },
  { 106, 2, 41 }, { 108, 2, 45 }, { 110, 1, 46 },
  { 112, 1, 46 }, { 113, 1, 46 }, { 114, 2, 45 },
  { 118, 1, 45 }, { 120, 1, 45 }, { 121, 1, 45 },
  { 122, 1, 46 }, { 124, 2, 46 }, { 126, 2, 41 },
  { 128, 1, 46 }, { 129, 1, 46 }, { 130, 1, 46 },
  { 132, 1, 46 }, { 133, 1, 46 }, { 134, 1, 46 },
  { 136, 1, 46 }, { 137, 1, 46 }, { 138, 2, 46 },
  { 140, 2, 50 }, { 142, 2, 46 }, { 144, 2, 50 },
  { 146, 2, 46 }, { 148, 2, 50 }, { 150, 2, 46 },
  { 152, 2, 45 }, { 154, 2, 43 }, { 156, 2, 45 },
  { 158, 2, 43 }, { 160, 1, 46 }, { 161, 1, 46 },
  { 162, 1, 46 }, { 164, 1, 46 }, { 165, 1, 46 },
  { 166, 1, 46 }, { 168, 1, 46 }, { 169, 1, 46 },
  { 170, 2, 46 }, { 172, 2, 50 }, { 174, 2, 46 },
  { 176, 2, 50 }, { 178, 2, 46 }, { 180, 2, 50 },
  { 182, 1, 48 }, { 184, 1, 48 }, { 186, 1, 48 },
  { 188, 1, 48 }, { 190, 2, 48 }, { 194, 2, 38 },
  { 198, 2, 38 }, { 202, 2, 41 }, { 204, 2, 45 },
  { 206, 2, 46 }, { 210, 2, 38 }, { 214, 2, 38 },
  { 218, 2, 45 }, { 220, 1, 41 }, { 222, 2, 41 },
  { 226, 2, 38 }, { 230, 2, 38 }, { 234, 2, 41 },
  { 236, 2, 45 }, { 238, 1, 46 }, { 240, 1, 46 },
  { 241, 1, 46 }, { 242, 2, 45 }, { 246, 1, 45 },
  { 248, 1, 45 }, { 249, 1, 45 }, { 250, 1, 46 },
  { 252, 2, 46 }, { 254, 4, 41 },
};
const uint16_t PART6_LEN = sizeof(PART6) / sizeof(PART6[0]);

// ===== SONG 4 DATA =====

const Event PART7[] PROGMEM = {
  { 4, 2, 39 }, { 6, 2, 41 }, { 8, 4, 43 },
  { 16, 1, 44 }, { 18, 1, 44 }, { 20, 4, 44 },
  { 24, 1, 43 }, { 25, 1, 44 }, { 26, 4, 46 },
  { 38, 4, 41 }, { 44, 4, 41 }, { 52, 2, 39 },
  { 54, 2, 41 }, { 56, 4, 43 }, { 64, 1, 44 },
  { 66, 1, 44 }, { 68, 2, 44 }, { 72, 1, 44 },
  { 73, 1, 44 }, { 74, 3, 43 }, { 77, 1, 41 },
  { 78, 2, 39 }, { 80, 2, 38 }, { 82, 2, 39 },
  { 84, 2, 41 }, { 86, 4, 39 }, { 92, 6, 39 },
  { 98, 6, 46 }, { 104, 2, 44 }, { 106, 3, 43 },
  { 109, 1, 41 }, { 110, 4, 43 }, { 116, 4, 43 },
  { 122, 2, 39 }, { 126, 1, 39 }, { 127, 1, 39 },
  { 128, 2, 39 }, { 132, 2, 39 }, { 134, 4, 41 },
  { 140, 4, 41 }, { 148, 2, 39 }, { 150, 2, 41 },
  { 152, 4, 43 }, { 160, 1, 44 }, { 162, 1, 44 },
  { 164, 2, 44 }, { 168, 1, 44 }, { 169, 1, 44 },
  { 170, 3, 43 }, { 173, 1, 41 }, { 174, 2, 39 },
  { 176, 2, 38 }, { 178, 2, 39 }, { 180, 2, 41 },
  { 182, 4, 39 }, { 188, 6, 39 },
};
const uint16_t PART7_LEN = sizeof(PART7) / sizeof(PART7[0]);

const Event PART8[] PROGMEM = {
  { 0, 1, 34 }, { 2, 2, 34 }, { 8, 4, 34 },
  { 12, 1, 39 }, { 13, 1, 38 }, { 14, 2, 36 },
  { 20, 4, 36 }, { 26, 2, 39 }, { 30, 1, 39 },
  { 31, 1, 39 }, { 32, 2, 39 }, { 34, 2, 38 },
  { 36, 2, 39 }, { 40, 2, 36 }, { 42, 2, 39 },
  { 44, 4, 38 }, { 48, 1, 34 }, { 50, 2, 34 },
  { 56, 4, 34 }, { 60, 1, 39 }, { 61, 1, 38 },
  { 62, 2, 36 }, { 68, 4, 36 }, { 80, 2, 34 },
  { 82, 2, 36 }, { 84, 2, 38 }, { 88, 2, 34 },
  { 90, 2, 32 }, { 92, 6, 31 }, { 98, 2, 27 },
  { 100, 1, 34 }, { 102, 2, 34 }, { 104, 2, 29 },
  { 106, 4, 34 }, { 110, 2, 27 }, { 112, 1, 34 },
  { 114, 1, 34 }, { 116, 2, 34 }, { 120, 1, 34 },
  { 121, 1, 34 }, { 130, 2, 38 }, { 136, 2, 36 },
  { 138, 2, 39 }, { 140, 4, 38 }, { 144, 1, 34 },
  { 146, 2, 34 }, { 152, 4, 34 }, { 156, 1, 39 },
  { 157, 1, 38 }, { 158, 2, 36 }, { 164, 4, 35 },
  { 176, 2, 34 }, { 178, 2, 36 }, { 180, 2, 38 },
  { 184, 2, 34 }, { 186, 2, 32 }, { 188, 6, 31 },
};
const uint16_t PART8_LEN = sizeof(PART8) / sizeof(PART8[0]);

// ===== SONG 5 DATA =====

const Event PART9[] PROGMEM = {
  { 0, 1, 51 }, { 2, 1, 51 }, { 3, 1, 51 },
  { 4, 1, 51 }, { 6, 1, 51 }, { 8, 8, 51 },
  { 16, 1, 55 }, { 18, 1, 55 }, { 19, 1, 55 },
  { 20, 1, 55 }, { 22, 1, 55 }, { 24, 8, 55 },
  { 32, 6, 58 }, { 40, 6, 58 }, { 48, 2, 58 },
  { 52, 2, 58 }, { 56, 4, 58 }, { 60, 4, 46 },
  { 64, 2, 51 }, { 68, 1, 51 }, { 71, 1, 51 },
  { 72, 4, 51 }, { 76, 3, 55 }, { 79, 1, 53 },
  { 80, 4, 51 }, { 84, 3, 50 }, { 87, 1, 48 },
  { 88, 4, 46 }, { 94, 2, 46 }, { 96, 2, 48 },
  { 100, 3, 48 }, { 103, 1, 50 }, { 104, 4, 51 },
  { 108, 3, 53 }, { 111, 1, 55 }, { 112, 4, 56 },
  { 116, 3, 55 }, { 119, 1, 53 }, { 120, 2, 55 },
  { 124, 3, 55 }, { 127, 1, 53 }, { 128, 4, 51 },
  { 132, 3, 43 }, { 135, 1, 44 }, { 136, 4, 46 },
  { 140, 3, 55 }, { 143, 1, 53 }, { 144, 4, 51 },
  { 148, 3, 43 }, { 151, 1, 44 }, { 152, 4, 46 },
  { 158, 2, 53 }, { 160, 4, 55 }, { 164, 3, 53 },
  { 167, 1, 51 }, { 168, 4, 58 }, { 172, 3, 50 },
  { 175, 1, 51 }, { 176, 2, 53 }, { 180, 1, 53 },
  { 183, 1, 53 }, { 184, 4, 46 }, { 188, 1, 53 },
  { 191, 1, 53 }, { 192, 4, 53 }, { 196, 3, 50 },
  { 199, 1, 51 }, { 200, 4, 53 }, { 204, 1, 55 },
  { 207, 1, 55 }, { 208, 4, 55 }, { 212, 3, 51 },
  { 215, 1, 53 }, { 216, 4, 55 }, { 222, 2, 55 },
  { 224, 2, 53 }, { 226, 2, 51 }, { 228, 2, 50 },
  { 230, 2, 55 }, { 232, 2, 51 }, { 234, 2, 48 },
  { 240, 8, 51 }, { 248, 8, 46 }, { 256, 2, 43 },
  { 258, 6, 39 }, { 268, 4, 46 }, { 272, 3, 48 },
  { 275, 1, 50 }, { 276, 3, 51 }, { 279, 1, 53 },
  { 280, 4, 55 }, { 284, 3, 53 }, { 287, 1, 51 },
  { 288, 4, 58 }, { 292, 1, 50 }, { 295, 1, 50 },
  { 296, 4, 51 }, { 300, 1, 53 }, { 303, 1, 53 },
  { 304, 4, 53 }, { 308, 3, 50 }, { 311, 1, 51 },
  { 312, 4, 53 }, { 316, 1, 55 }, { 319, 1, 55 },
  { 320, 4, 55 }, { 324, 3, 51 }, { 327, 1, 53 },
  { 328, 4, 55 }, { 334, 2, 55 }, { 336, 2, 53 },
  { 338, 2, 51 }, { 340, 2, 50 }, { 342, 2, 55 },
  { 344, 2, 51 }, { 346, 2, 48 }, { 352, 8, 51 },
  { 360, 8, 46 }, { 368, 2, 43 }, { 370, 6, 39 },
  { 380, 4, 46 }, { 384, 3, 48 }, { 387, 1, 50 },
  { 388, 3, 51 }, { 391, 1, 53 }, { 392, 4, 55 },
  { 396, 3, 53 }, { 399, 1, 51 }, { 400, 12, 58 },
  { 412, 1, 50 }, { 415, 1, 50 }, { 416, 12, 51 },
};
const uint16_t PART9_LEN = sizeof(PART9) / sizeof(PART9[0]);

const Event PART10[] PROGMEM = {
  { 10, 2, 22 }, { 12, 2, 27 }, { 14, 2, 31 },
  { 26, 2, 27 }, { 28, 2, 31 }, { 30, 2, 34 },
  { 32, 2, 20 }, { 34, 2, 22 }, { 36, 2, 24 },
  { 38, 2, 25 }, { 40, 2, 27 }, { 42, 2, 29 },
  { 44, 2, 31 }, { 46, 2, 32 }, { 48, 2, 34 },
  { 50, 2, 29 }, { 52, 2, 26 }, { 54, 2, 24 },
  { 56, 4, 22 }, { 60, 4, 34 }, { 64, 4, 27 },
  { 68, 3, 31 }, { 71, 1, 34 }, { 72, 4, 39 },
  { 76, 3, 34 }, { 79, 1, 38 }, { 80, 4, 39 },
  { 84, 1, 32 }, { 87, 1, 32 }, { 88, 4, 27 },
  { 94, 2, 27 }, { 96, 2, 32 }, { 100, 1, 32 },
  { 103, 1, 32 }, { 104, 4, 32 }, { 108, 1, 31 },
  { 111, 1, 31 }, { 112, 4, 29 }, { 116, 1, 34 },
  { 119, 1, 34 }, { 120, 4, 27 }, { 124, 4, 34 },
  { 128, 4, 39 }, { 132, 3, 31 }, { 135, 1, 32 },
  { 136, 2, 34 }, { 140, 1, 34 }, { 143, 1, 34 },
  { 144, 4, 39 }, { 148, 3, 31 }, { 151, 1, 32 },
  { 152, 4, 34 }, { 158, 2, 39 }, { 160, 2, 36 },
  { 164, 1, 36 }, { 167, 1, 36 }, { 168, 4, 38 },
  { 172, 3, 34 }, { 175, 1, 31 }, { 176, 2, 29 },
  { 180, 1, 29 }, { 183, 1, 29 }, { 184, 2, 34 },
  { 188, 1, 34 }, { 191, 1, 34 }, { 192, 2, 34 },
  { 196, 1, 34 }, { 199, 1, 34 }, { 200, 4, 34 },
  { 204, 1, 27 }, { 207, 1, 27 }, { 208, 4, 27 },
  { 212, 3, 31 }, { 215, 1, 34 }, { 216, 4, 39 },
  { 222, 2, 39 }, { 224, 4, 27 }, { 228, 1, 31 },
  { 230, 2, 31 }, { 232, 2, 36 }, { 234, 2, 39 },
  { 240, 8, 39 }, { 248, 8, 34 }, { 256, 2, 31 },
  { 258, 6, 27 }, { 268, 4, 27 }, { 272, 4, 32 },
  { 276, 3, 31 }, { 279, 1, 29 }, { 280, 4, 27 },
  { 284, 1, 36 }, { 287, 1, 36 }, { 288, 2, 34 },
  { 292, 1, 34 }, { 295, 1, 34 }, { 296, 4, 27 },
  { 300, 1, 34 }, { 303, 1, 34 }, { 304, 2, 34 },
  { 308, 1, 34 }, { 311, 1, 34 }, { 312, 4, 34 },
  { 316, 1, 27 }, { 319, 1, 27 }, { 320, 4, 27 },
  { 324, 3, 31 }, { 327, 1, 34 }, { 328, 4, 39 },
  { 334, 2, 39 }, { 336, 4, 27 }, { 340, 1, 31 },
  { 342, 2, 31 }, { 344, 2, 36 }, { 346, 2, 39 },
  { 352, 8, 39 }, { 360, 8, 34 }, { 368, 2, 31 },
  { 370, 6, 27 }, { 380, 4, 27 }, { 384, 4, 32 },
  { 388, 3, 31 }, { 391, 1, 29 }, { 392, 4, 27 },
  { 396, 1, 36 }, { 399, 1, 36 }, { 400, 10, 34 },
  { 412, 1, 34 }, { 415, 1, 34 }, { 416, 12, 27 },
};
const uint16_t PART10_LEN = sizeof(PART10) / sizeof(PART10[0]);
// ===== SONG ARRAY =====
const Song SONGS[] = {
  // Song 1
  {
    PART1, PART1_LEN,
    PART2, PART2_LEN,
    4,    // ticksPerQuarter
    96,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    256   // totalTicks
  },
  // Song 2
  {
    PART3, PART3_LEN,
    PART4, PART4_LEN,
    4,    // ticksPerQuarter
    60,  // tempoBPM
    6,    // timeBeats
    8,    // timeBeatType
    324   // totalTicks
  },
  // Song 3
  {
    PART5, PART5_LEN,
    PART6, PART6_LEN,
    2,    // ticksPerQuarter
    165,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    258   // totalTicks
  },
  // Song 4
  {
    PART7, PART7_LEN,
    PART8, PART8_LEN,
    2,    // ticksPerQuarter
    88,  // tempoBPM
    3,    // timeBeats
    4,    // timeBeatType
    194   // totalTicks
  },
  // Song 5
  {
    PART9, PART9_LEN,
    PART10, PART10_LEN,
    4,    // ticksPerQuarter
    92,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    428   // totalTicks
  },
};
const uint8_t NUM_SONGS = sizeof(SONGS) / sizeof(SONGS[0]);

typedef struct {
  byte uid[4];
  int8_t songIndex;
  const char* title;
  const char* mood;
} RFIDMapping;

const RFIDMapping RFID_MAPPINGS[] = {



  // Add your RFID mappings here
  // Format: { { 0xXX, 0xXX, 0xXX, 0xXX }, songIndex, "Song Name", "Mood" },
  { { 0xE2, 0x83, 0x50, 0x20 }, 0, "Olymic Fanfare", "Euphoric" },
  { { 0x86, 0x06, 0xCA, 0x30 }, 1, "Morning Mood", "Calm" },
  { { 0x86, 0x8E, 0x5A, 0x30 }, 2, "William Tell Overture", "Energized" },
  { { 0x09, 0x63, 0x15, 0xC1 }, 3, "Home on the Range", "Melancholic" },
  { { 0x1B, 0x79, 0xF3, 0x00 }, 4, "Heart of Oak", "Intense" },



};
const uint8_t NUM_RFID_MAPPINGS = sizeof(RFID_MAPPINGS) / sizeof(RFID_MAPPINGS[0]);

const uint8_t SONG_TO_PLAY = 255;
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