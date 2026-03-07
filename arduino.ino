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
  { 96, 4, 53 }, { 100, 4, 52 }, { 104, 4, 53 },
  { 108, 4, 50 }, { 112, 4, 48 }, { 116, 4, 46 },
  { 120, 8, 48 }, { 128, 4, 45 }, { 132, 8, 41 },
  { 144, 12, 48 }, { 156, 12, 41 }, { 168, 12, 53 },
  { 180, 12, 50 }, { 192, 4, 53 }, { 196, 4, 52 },
  { 200, 4, 53 }, { 204, 4, 50 }, { 208, 4, 48 },
  { 212, 4, 46 }, { 216, 8, 48 }, { 224, 4, 45 },
  { 228, 8, 41 }, { 240, 4, 46 }, { 244, 4, 53 },
  { 248, 4, 46 }, { 252, 4, 45 }, { 256, 4, 41 },
  { 260, 4, 48 }, { 264, 10, 46 }, { 276, 8, 46 },
  { 288, 4, 50 }, { 292, 4, 51 }, { 296, 4, 50 },
  { 300, 4, 48 }, { 304, 4, 46 }, { 308, 4, 45 },
  { 312, 8, 50 }, { 320, 4, 46 }, { 324, 8, 43 },
  { 336, 4, 53 }, { 340, 4, 51 }, { 344, 4, 50 },
  { 348, 8, 48 }, { 356, 4, 49 }, { 360, 10, 50 },
  { 372, 4, 50 }, { 384, 4, 50 }, { 388, 4, 51 },
  { 392, 4, 50 }, { 396, 4, 48 }, { 400, 4, 46 },
  { 404, 4, 45 }, { 408, 8, 50 }, { 416, 4, 46 },
  { 420, 8, 43 }, { 428, 4, 50 }, { 432, 4, 48 },
  { 436, 4, 55 }, { 440, 4, 52 }, { 444, 8, 50 },
  { 452, 4, 48 }, { 456, 10, 53 }, { 468, 8, 53 },
  { 480, 12, 53 }, { 492, 12, 50 }, { 504, 12, 46 },
  { 516, 12, 41 }, { 528, 4, 43 }, { 532, 4, 45 },
  { 536, 4, 46 }, { 540, 8, 43 }, { 548, 4, 46 },
  { 552, 10, 41 }, { 564, 8, 41 }, { 576, 12, 48 },
  { 588, 12, 53 }, { 600, 12, 50 }, { 612, 12, 46 },
  { 624, 4, 43 }, { 628, 4, 45 }, { 632, 4, 46 },
  { 636, 8, 48 }, { 644, 4, 50 }, { 648, 10, 48 },
  { 660, 4, 48 }, { 668, 4, 50 }, { 672, 4, 51 },
  { 676, 4, 50 }, { 680, 4, 48 }, { 684, 8, 53 },
  { 692, 4, 50 }, { 696, 4, 48 }, { 700, 6, 46 },
  { 708, 4, 46 }, { 716, 4, 48 }, { 720, 8, 50 },
  { 730, 2, 46 }, { 732, 8, 43 }, { 742, 2, 46 },
  { 744, 4, 43 }, { 748, 6, 41 }, { 756, 4, 41 },
  { 764, 4, 41 }, { 768, 8, 46 }, { 776, 4, 50 },
  { 780, 3, 48 }, { 792, 8, 46 }, { 800, 4, 50 },
  { 804, 3, 48 }, { 812, 2, 50 }, { 814, 2, 51 },
  { 816, 4, 53 }, { 820, 4, 50 }, { 824, 4, 46 },
  { 828, 8, 48 }, { 836, 4, 41 }, { 840, 10, 46 },
  { 852, 12, 46 },
};
const uint16_t PART1_LEN = sizeof(PART1) / sizeof(PART1[0]);

const Event PART2[] PROGMEM = {
  { 0, 4, 65 }, { 4, 4, 64 }, { 8, 4, 65 },
  { 12, 4, 62 }, { 16, 4, 60 }, { 20, 4, 58 },
  { 24, 8, 60 }, { 32, 4, 57 }, { 36, 8, 53 },
  { 44, 4, 65 }, { 48, 2, 62 }, { 50, 2, 53 },
  { 52, 2, 55 }, { 54, 2, 57 }, { 56, 2, 58 },
  { 58, 2, 60 }, { 60, 2, 61 }, { 62, 2, 62 },
  { 64, 2, 67 }, { 66, 2, 65 }, { 68, 2, 62 },
  { 70, 2, 58 }, { 72, 8, 60 }, { 80, 4, 53 },
  { 84, 12, 58 }, { 96, 4, 34 }, { 100, 4, 41 },
  { 104, 4, 46 }, { 108, 4, 34 }, { 112, 4, 41 },
  { 116, 4, 46 }, { 120, 4, 33 }, { 124, 4, 41 },
  { 128, 4, 48 }, { 132, 4, 29 }, { 136, 4, 41 },
  { 140, 4, 45 }, { 144, 4, 33 }, { 148, 4, 41 },
  { 152, 4, 48 }, { 156, 4, 29 }, { 160, 4, 41 },
  { 164, 4, 45 }, { 168, 4, 34 }, { 172, 4, 41 },
  { 176, 4, 46 }, { 180, 4, 29 }, { 184, 4, 41 },
  { 188, 4, 46 }, { 192, 4, 34 }, { 196, 4, 41 },
  { 200, 4, 46 }, { 204, 4, 34 }, { 208, 4, 41 },
  { 212, 4, 46 }, { 216, 4, 33 }, { 220, 4, 41 },
  { 224, 4, 48 }, { 228, 4, 29 }, { 232, 4, 41 },
  { 236, 4, 45 }, { 240, 4, 34 }, { 244, 4, 41 },
  { 248, 4, 46 }, { 252, 4, 29 }, { 256, 4, 41 },
  { 260, 4, 45 }, { 264, 4, 34 }, { 268, 4, 29 },
  { 272, 4, 26 }, { 276, 8, 22 }, { 288, 4, 31 },
  { 292, 4, 38 }, { 296, 4, 43 }, { 300, 4, 26 },
  { 304, 4, 38 }, { 308, 4, 42 }, { 312, 4, 31 },
  { 316, 4, 38 }, { 320, 4, 43 }, { 324, 4, 31 },
  { 328, 4, 34 }, { 332, 4, 38 }, { 336, 4, 33 },
  { 340, 4, 39 }, { 344, 4, 41 }, { 348, 4, 33 },
  { 352, 4, 39 }, { 356, 4, 41 }, { 360, 4, 34 },
  { 364, 2, 49 }, { 366, 2, 50 }, { 368, 2, 53 },
  { 370, 2, 58 }, { 372, 8, 62 }, { 384, 4, 31 },
  { 388, 4, 38 }, { 392, 4, 43 }, { 396, 4, 26 },
  { 400, 4, 38 }, { 404, 4, 42 }, { 408, 4, 31 },
  { 412, 4, 38 }, { 416, 4, 43 }, { 420, 4, 31 },
  { 424, 4, 38 }, { 428, 4, 43 }, { 432, 4, 24 },
  { 436, 4, 36 }, { 440, 4, 40 }, { 444, 4, 24 },
  { 448, 4, 36 }, { 452, 4, 40 }, { 456, 4, 29 },
  { 460, 4, 45 }, { 464, 4, 48 }, { 468, 8, 41 },
  { 480, 4, 34 }, { 484, 4, 46 }, { 488, 4, 50 },
  { 492, 4, 38 }, { 496, 4, 41 }, { 500, 4, 46 },
  { 504, 4, 26 }, { 508, 4, 34 }, { 512, 4, 41 },
  { 516, 4, 22 }, { 520, 4, 34 }, { 524, 4, 38 },
  { 528, 4, 27 }, { 532, 4, 34 }, { 536, 4, 39 },
  { 540, 4, 27 }, { 544, 4, 34 }, { 548, 4, 39 },
  { 552, 4, 22 }, { 556, 4, 29 }, { 560, 4, 26 },
  { 564, 4, 22 }, { 568, 4, 34 }, { 572, 4, 38 },
  { 576, 4, 33 }, { 580, 4, 39 }, { 584, 4, 41 },
  { 588, 4, 29 }, { 592, 4, 39 }, { 596, 4, 41 },
  { 600, 4, 34 }, { 604, 4, 41 }, { 608, 4, 46 },
  { 612, 4, 31 }, { 616, 4, 38 }, { 620, 4, 43 },
  { 624, 4, 24 }, { 628, 4, 36 }, { 632, 4, 40 },
  { 636, 4, 24 }, { 640, 4, 36 }, { 644, 4, 40 },
  { 648, 4, 29 }, { 652, 4, 36 }, { 656, 4, 41 },
  { 660, 4, 29 }, { 664, 4, 36 }, { 668, 4, 41 },
  { 672, 4, 33 }, { 676, 4, 39 }, { 680, 4, 41 },
  { 684, 4, 29 }, { 688, 4, 39 }, { 692, 4, 41 },
  { 696, 4, 34 }, { 700, 4, 38 }, { 704, 4, 41 },
  { 708, 4, 34 }, { 712, 4, 38 }, { 716, 4, 41 },
  { 720, 4, 31 }, { 724, 4, 34 }, { 730, 2, 38 },
  { 732, 4, 27 }, { 736, 4, 34 }, { 742, 2, 39 },
  { 744, 4, 22 }, { 748, 4, 34 }, { 752, 4, 38 },
  { 756, 4, 21 }, { 760, 4, 36 }, { 764, 4, 39 },
  { 768, 4, 22 }, { 772, 4, 34 }, { 776, 4, 38 },
  { 780, 3, 39 }, { 792, 4, 22 }, { 796, 4, 34 },
  { 800, 4, 38 }, { 804, 3, 39 }, { 816, 4, 22 },
  { 820, 4, 34 }, { 824, 4, 38 }, { 828, 4, 17 },
  { 832, 4, 33 }, { 836, 4, 39 }, { 840, 4, 34 },
  { 844, 4, 29 }, { 848, 4, 26 }, { 852, 12, 22 },
};
const uint16_t PART2_LEN = sizeof(PART2) / sizeof(PART2[0]);



// ===== SONG 2 DATA =====

const Event PART3[] PROGMEM = {
  { 0, 1, 48 }, { 1, 1, 48 }, { 2, 1, 48 },
  { 3, 1, 52 }, { 4, 1, 60 }, { 5, 1, 52 },
  { 6, 1, 50 }, { 7, 1, 50 }, { 8, 1, 50 },
  { 9, 1, 52 }, { 10, 1, 60 }, { 11, 1, 60 },
  { 12, 1, 48 }, { 13, 1, 48 }, { 14, 1, 48 },
  { 15, 1, 52 }, { 16, 1, 60 }, { 17, 1, 52 },
  { 18, 1, 50 }, { 19, 1, 60 }, { 20, 1, 60 },
  { 21, 1, 57 }, { 22, 1, 55 }, { 23, 1, 52 },
  { 24, 1, 48 }, { 25, 1, 48 }, { 26, 1, 48 },
  { 27, 1, 52 }, { 28, 1, 60 }, { 29, 1, 52 },
  { 30, 1, 50 }, { 31, 1, 50 }, { 32, 1, 50 },
  { 33, 1, 52 }, { 34, 1, 60 }, { 35, 1, 60 },
  { 36, 2, 60 }, { 38, 1, 55 }, { 39, 1, 57 },
  { 40, 1, 55 }, { 41, 1, 52 }, { 42, 1, 50 },
  { 43, 1, 50 }, { 44, 1, 50 }, { 45, 1, 52 },
  { 46, 1, 60 }, { 47, 1, 60 }, { 48, 1, 48 },
  { 49, 1, 48 }, { 50, 1, 48 }, { 51, 1, 52 },
  { 52, 1, 60 }, { 53, 1, 52 }, { 54, 1, 50 },
  { 55, 1, 50 }, { 56, 1, 50 }, { 57, 1, 52 },
  { 58, 1, 60 }, { 59, 1, 60 }, { 60, 1, 48 },
  { 61, 1, 48 }, { 62, 1, 48 }, { 63, 1, 52 },
  { 64, 1, 60 }, { 65, 1, 52 }, { 66, 1, 50 },
  { 67, 1, 60 }, { 68, 1, 60 }, { 69, 1, 57 },
  { 70, 1, 55 }, { 71, 1, 52 }, { 72, 1, 48 },
  { 73, 1, 48 }, { 74, 1, 48 }, { 75, 1, 52 },
  { 76, 1, 60 }, { 77, 1, 52 }, { 78, 1, 50 },
  { 79, 1, 50 }, { 80, 1, 50 }, { 81, 1, 52 },
  { 82, 1, 60 }, { 83, 1, 60 }, { 84, 2, 60 },
  { 86, 1, 55 }, { 87, 1, 57 }, { 88, 1, 55 },
  { 89, 1, 52 }, { 90, 1, 50 }, { 91, 1, 50 },
  { 92, 1, 50 }, { 93, 1, 52 }, { 94, 1, 60 },
  { 95, 1, 60 }, { 96, 1, 57 }, { 97, 1, 60 },
  { 98, 1, 60 }, { 99, 1, 55 }, { 100, 1, 60 },
  { 101, 1, 60 }, { 102, 1, 52 }, { 103, 1, 60 },
  { 104, 1, 60 }, { 105, 1, 48 }, { 106, 1, 60 },
  { 107, 1, 60 }, { 108, 1, 57 }, { 109, 1, 60 },
  { 110, 1, 60 }, { 111, 1, 55 }, { 112, 1, 60 },
  { 113, 1, 60 }, { 114, 1, 52 }, { 115, 1, 60 },
  { 116, 1, 60 }, { 117, 1, 50 }, { 118, 1, 60 },
  { 119, 1, 60 }, { 120, 1, 57 }, { 121, 1, 60 },
  { 122, 1, 60 }, { 123, 1, 55 }, { 124, 1, 60 },
  { 125, 1, 60 }, { 126, 1, 52 }, { 127, 1, 60 },
  { 128, 1, 60 }, { 129, 1, 48 }, { 130, 1, 60 },
  { 131, 1, 60 }, { 132, 2, 60 }, { 134, 1, 55 },
  { 135, 1, 57 }, { 136, 1, 55 }, { 137, 1, 52 },
  { 138, 1, 50 }, { 139, 1, 50 }, { 140, 1, 50 },
  { 141, 1, 52 }, { 142, 1, 60 }, { 143, 1, 60 },
  { 144, 1, 57 }, { 145, 1, 60 }, { 146, 1, 60 },
  { 147, 1, 55 }, { 148, 1, 60 }, { 149, 1, 60 },
  { 150, 1, 52 }, { 151, 1, 60 }, { 152, 1, 60 },
  { 153, 1, 48 }, { 154, 1, 60 }, { 155, 1, 60 },
  { 156, 1, 57 }, { 157, 1, 60 }, { 158, 1, 60 },
  { 159, 1, 55 }, { 160, 1, 60 }, { 161, 1, 60 },
  { 162, 1, 52 }, { 163, 1, 60 }, { 164, 1, 60 },
  { 165, 1, 55 }, { 166, 1, 60 }, { 167, 1, 60 },
  { 168, 1, 57 }, { 169, 1, 60 }, { 170, 1, 60 },
  { 171, 1, 55 }, { 172, 1, 60 }, { 173, 1, 60 },
  { 174, 1, 52 }, { 175, 1, 60 }, { 176, 1, 60 },
  { 177, 1, 48 }, { 178, 1, 60 }, { 179, 1, 60 },
  { 180, 2, 60 }, { 182, 1, 55 }, { 183, 1, 57 },
  { 184, 1, 55 }, { 185, 1, 52 }, { 186, 1, 50 },
  { 187, 1, 50 }, { 188, 1, 50 }, { 189, 1, 52 },
  { 190, 1, 60 }, { 191, 1, 60 }, { 192, 1, 48 },
  { 193, 1, 48 }, { 194, 1, 48 }, { 195, 1, 52 },
  { 196, 1, 60 }, { 197, 1, 52 }, { 198, 1, 50 },
  { 199, 1, 50 }, { 200, 1, 50 }, { 201, 1, 52 },
  { 202, 1, 60 }, { 203, 1, 60 }, { 204, 1, 48 },
  { 205, 1, 48 }, { 206, 1, 48 }, { 207, 1, 52 },
  { 208, 1, 60 }, { 209, 1, 52 }, { 210, 1, 50 },
  { 211, 1, 60 }, { 212, 1, 60 }, { 213, 1, 57 },
  { 214, 1, 55 }, { 215, 1, 52 }, { 216, 1, 48 },
  { 217, 1, 48 }, { 218, 1, 48 }, { 219, 1, 52 },
  { 220, 1, 60 }, { 221, 1, 52 }, { 222, 1, 50 },
  { 223, 1, 50 }, { 224, 1, 50 }, { 225, 1, 52 },
  { 226, 1, 60 }, { 227, 1, 60 }, { 228, 2, 60 },
  { 230, 1, 55 }, { 231, 1, 57 }, { 232, 1, 55 },
  { 233, 1, 52 }, { 234, 1, 50 }, { 235, 1, 50 },
  { 236, 1, 50 }, { 237, 1, 52 }, { 238, 1, 60 },
  { 239, 1, 60 }, { 240, 1, 48 }, { 241, 1, 48 },
  { 242, 1, 48 }, { 243, 1, 52 }, { 244, 1, 60 },
  { 245, 1, 52 }, { 246, 1, 50 }, { 247, 1, 50 },
  { 248, 1, 50 }, { 249, 1, 52 }, { 250, 1, 60 },
  { 251, 1, 60 }, { 252, 1, 48 }, { 253, 1, 48 },
  { 254, 1, 48 }, { 255, 1, 52 }, { 256, 1, 60 },
  { 257, 1, 52 }, { 258, 1, 50 }, { 259, 1, 60 },
  { 260, 1, 60 }, { 261, 1, 57 }, { 262, 1, 55 },
  { 263, 1, 52 }, { 264, 1, 48 }, { 265, 1, 48 },
  { 266, 1, 48 }, { 267, 1, 52 }, { 268, 1, 60 },
  { 269, 1, 52 }, { 270, 1, 50 }, { 271, 1, 50 },
  { 272, 1, 50 }, { 273, 1, 52 }, { 274, 1, 60 },
  { 275, 1, 60 }, { 276, 2, 60 }, { 278, 1, 55 },
  { 279, 1, 57 }, { 280, 1, 55 }, { 281, 1, 52 },
  { 282, 1, 50 }, { 283, 1, 50 }, { 284, 1, 50 },
  { 285, 1, 52 }, { 286, 1, 60 }, { 287, 1, 60 },
  { 288, 1, 57 }, { 289, 1, 60 }, { 290, 1, 60 },
  { 291, 1, 55 }, { 292, 1, 60 }, { 293, 1, 60 },
  { 294, 1, 52 }, { 295, 1, 60 }, { 296, 1, 60 },
  { 297, 1, 48 }, { 298, 1, 60 }, { 299, 1, 60 },
  { 300, 1, 57 }, { 301, 1, 60 }, { 302, 1, 60 },
  { 303, 1, 55 }, { 304, 1, 60 }, { 305, 1, 60 },
  { 306, 1, 52 }, { 307, 1, 60 }, { 308, 1, 60 },
  { 309, 1, 50 }, { 310, 1, 60 }, { 311, 1, 60 },
  { 312, 1, 57 }, { 313, 1, 60 }, { 314, 1, 60 },
  { 315, 1, 55 }, { 316, 1, 60 }, { 317, 1, 60 },
  { 318, 1, 52 }, { 319, 1, 60 }, { 320, 1, 60 },
  { 321, 1, 48 }, { 322, 1, 60 }, { 323, 1, 60 },
  { 324, 2, 60 }, { 326, 1, 55 }, { 327, 1, 57 },
  { 328, 1, 55 }, { 329, 1, 52 }, { 330, 1, 50 },
  { 331, 1, 50 }, { 332, 1, 50 }, { 333, 1, 52 },
  { 334, 1, 60 }, { 335, 1, 60 }, { 336, 1, 57 },
  { 337, 1, 60 }, { 338, 1, 60 }, { 339, 1, 55 },
  { 340, 1, 60 }, { 341, 1, 60 }, { 342, 1, 52 },
  { 343, 1, 60 }, { 344, 1, 60 }, { 345, 1, 48 },
  { 346, 1, 60 }, { 347, 1, 60 }, { 348, 1, 57 },
  { 349, 1, 60 }, { 350, 1, 60 }, { 351, 1, 55 },
  { 352, 1, 60 }, { 353, 1, 60 }, { 354, 1, 52 },
  { 355, 1, 60 }, { 356, 1, 60 }, { 357, 1, 55 },
  { 358, 1, 60 }, { 359, 1, 60 }, { 360, 1, 57 },
  { 361, 1, 60 }, { 362, 1, 60 }, { 363, 1, 55 },
  { 364, 1, 60 }, { 365, 1, 60 }, { 366, 1, 52 },
  { 367, 1, 60 }, { 368, 1, 60 }, { 369, 1, 48 },
  { 370, 1, 60 }, { 371, 1, 60 }, { 372, 2, 60 },
  { 374, 1, 55 }, { 375, 1, 57 }, { 376, 1, 55 },
  { 377, 1, 52 }, { 378, 1, 50 }, { 379, 1, 50 },
  { 380, 1, 50 }, { 381, 1, 52 }, { 382, 1, 60 },
  { 383, 1, 60 }, { 384, 6, 48 },
};
const uint16_t PART3_LEN = sizeof(PART3) / sizeof(PART3[0]);

const Event PART4[] PROGMEM = {};
const uint16_t PART4_LEN = 0;

// ===== SONG 3 DATA =====

const Event PART5[] PROGMEM = {
  { 0, 16, 48 }, { 1, 1, 53 }, { 2, 6, 57 },
  { 8, 8, 55 }, { 16, 16, 57 }, { 20, 4, 60 },
  { 24, 8, 53 }, { 32, 16, 48 }, { 33, 1, 53 },
  { 34, 6, 57 }, { 40, 8, 55 }, { 48, 16, 57 },
  { 52, 4, 60 }, { 56, 8, 53 }, { 64, 16, 58 },
  { 68, 4, 57 }, { 72, 4, 55 }, { 76, 4, 53 },
  { 80, 16, 52 }, { 84, 4, 53 }, { 88, 4, 55 },
  { 92, 4, 52 }, { 96, 32, 53 }, { 100, 4, 55 },
  { 104, 4, 57 }, { 108, 4, 58 }, { 112, 8, 60 },
  { 120, 6, 48 }, { 128, 16, 48 }, { 129, 1, 53 },
  { 130, 6, 57 }, { 136, 8, 55 }, { 144, 16, 57 },
  { 148, 4, 60 }, { 152, 8, 53 }, { 160, 16, 48 },
  { 161, 1, 53 }, { 162, 6, 57 }, { 168, 8, 55 },
  { 176, 16, 57 }, { 180, 4, 60 }, { 184, 8, 53 },
  { 192, 16, 58 }, { 196, 4, 57 }, { 200, 4, 55 },
  { 204, 4, 53 }, { 208, 16, 52 }, { 212, 4, 53 },
  { 216, 4, 55 }, { 220, 4, 52 }, { 224, 32, 53 },
  { 228, 4, 48 }, { 232, 4, 45 }, { 236, 4, 48 },
  { 240, 16, 41 }, { 256, 16, 48 }, { 257, 1, 53 },
  { 258, 6, 57 }, { 264, 8, 55 }, { 272, 16, 57 },
  { 276, 4, 60 }, { 280, 8, 53 }, { 288, 16, 48 },
  { 289, 1, 53 }, { 290, 6, 57 }, { 296, 8, 55 },
  { 304, 16, 57 }, { 308, 4, 60 }, { 312, 8, 53 },
  { 320, 16, 58 }, { 324, 4, 57 }, { 328, 4, 55 },
  { 332, 4, 53 }, { 336, 16, 52 }, { 340, 4, 53 },
  { 344, 4, 55 }, { 348, 4, 52 }, { 352, 32, 53 },
  { 356, 4, 55 }, { 360, 4, 57 }, { 364, 4, 58 },
  { 368, 8, 60 }, { 376, 6, 48 }, { 384, 16, 48 },
  { 385, 1, 53 }, { 386, 6, 57 }, { 392, 8, 55 },
  { 400, 16, 57 }, { 404, 4, 60 }, { 408, 8, 53 },
  { 416, 16, 48 }, { 417, 1, 53 }, { 418, 6, 57 },
  { 424, 8, 55 }, { 432, 16, 57 }, { 436, 4, 60 },
  { 440, 8, 53 }, { 448, 16, 58 }, { 452, 4, 57 },
  { 456, 4, 55 }, { 460, 4, 53 }, { 464, 16, 52 },
  { 468, 4, 53 }, { 472, 4, 55 }, { 476, 4, 52 },
  { 480, 32, 53 }, { 484, 4, 48 }, { 488, 4, 45 },
  { 492, 4, 48 }, { 496, 16, 41 },
};
const uint16_t PART5_LEN = sizeof(PART5) / sizeof(PART5[0]);

const Event PART6[] PROGMEM = {};
const uint16_t PART6_LEN = 0;

// ===== SONG 4 DATA =====

const Event PART7[] PROGMEM = {
  { 2, 2, 55 }, { 4, 1, 60 }, { 6, 2, 60 },
  { 8, 1, 64 }, { 10, 2, 64 }, { 12, 4, 60 },
  { 16, 1, 55 }, { 18, 1, 55 }, { 20, 1, 55 },
  { 23, 1, 55 }, { 24, 1, 62 }, { 25, 1, 60 },
  { 26, 1, 59 }, { 27, 1, 57 }, { 28, 4, 55 },
  { 34, 2, 55 }, { 36, 1, 60 }, { 38, 2, 60 },
  { 40, 1, 64 }, { 42, 2, 64 }, { 44, 4, 60 },
  { 48, 2, 55 }, { 50, 2, 60 }, { 52, 2, 59 },
  { 54, 1, 57 }, { 55, 1, 59 }, { 56, 2, 60 },
  { 58, 2, 54 }, { 60, 4, 55 }, { 66, 2, 55 },
  { 68, 1, 59 }, { 70, 2, 59 }, { 72, 1, 60 },
  { 73, 1, 59 }, { 74, 1, 57 }, { 75, 1, 59 },
  { 76, 4, 60 }, { 80, 2, 55 }, { 82, 2, 60 },
  { 84, 1, 59 }, { 86, 1, 59 }, { 88, 1, 59 },
  { 89, 1, 65 }, { 90, 1, 62 }, { 91, 1, 59 },
  { 92, 4, 60 }, { 98, 2, 60 }, { 100, 1, 57 },
  { 102, 1, 57 }, { 104, 2, 57 }, { 106, 1, 60 },
  { 108, 4, 60 }, { 112, 1, 55 }, { 114, 1, 55 },
  { 116, 1, 55 }, { 119, 1, 55 }, { 120, 2, 62 },
  { 122, 2, 59 }, { 124, 4, 60 }, { 130, 2, 60 },
  { 132, 1, 59 }, { 133, 1, 57 }, { 134, 1, 57 },
  { 136, 1, 57 }, { 137, 1, 57 }, { 138, 1, 59 },
  { 139, 1, 62 }, { 140, 4, 60 }, { 144, 1, 55 },
  { 146, 1, 55 }, { 148, 1, 55 }, { 151, 1, 55 },
  { 152, 2, 62 }, { 154, 2, 59 }, { 156, 6, 60 },
  { 164, 4, 64 }, { 168, 2, 67 }, { 170, 2, 62 },
  { 172, 2, 64 }, { 174, 2, 60 }, { 176, 4, 45 },
};
const uint16_t PART7_LEN = sizeof(PART7) / sizeof(PART7[0]);

const Event PART8[] PROGMEM = {};
const uint16_t PART8_LEN = 0;

// ===== SONG 5 DATA =====

const Event PART9[] PROGMEM = {
  { 6, 1, 48 }, { 7, 1, 48 }, { 8, 1, 53 },
  { 10, 1, 53 }, { 11, 1, 55 }, { 12, 1, 57 },
  { 13, 1, 53 }, { 14, 1, 57 }, { 15, 1, 58 },
  { 16, 1, 60 }, { 18, 1, 60 }, { 19, 1, 58 },
  { 20, 2, 57 }, { 22, 1, 55 }, { 23, 1, 53 },
  { 24, 2, 55 }, { 26, 2, 48 }, { 28, 2, 55 },
  { 30, 2, 48 }, { 32, 1, 55 }, { 33, 1, 57 },
  { 34, 1, 55 }, { 35, 1, 55 }, { 36, 1, 48 },
  { 38, 1, 48 }, { 39, 1, 48 }, { 40, 1, 53 },
  { 42, 1, 53 }, { 43, 1, 55 }, { 44, 1, 57 },
  { 45, 1, 53 }, { 46, 1, 57 }, { 47, 1, 58 },
  { 48, 1, 60 }, { 50, 1, 60 }, { 51, 1, 58 },
  { 52, 2, 57 }, { 54, 1, 55 }, { 55, 1, 53 },
  { 56, 1, 55 }, { 58, 1, 55 }, { 60, 2, 55 },
  { 62, 1, 57 }, { 63, 1, 55 }, { 64, 1, 53 },
  { 66, 1, 53 }, { 68, 4, 53 }, { 72, 4, 48 },
  { 76, 2, 45 }, { 78, 2, 43 }, { 80, 1, 45 },
  { 81, 1, 46 }, { 82, 1, 45 }, { 83, 1, 43 },
  { 84, 2, 41 }, { 86, 2, 36 }, { 88, 1, 41 },
  { 90, 1, 41 }, { 91, 1, 43 }, { 92, 2, 45 },
  { 94, 1, 43 }, { 95, 1, 41 }, { 96, 1, 43 },
  { 98, 1, 43 }, { 100, 2, 43 }, { 104, 4, 60 },
  { 108, 2, 57 }, { 110, 2, 55 }, { 112, 1, 57 },
  { 113, 1, 58 }, { 114, 1, 57 }, { 115, 1, 55 },
  { 116, 2, 53 }, { 118, 2, 48 }, { 120, 1, 55 },
  { 122, 1, 55 }, { 124, 2, 55 }, { 126, 1, 57 },
  { 127, 1, 55 }, { 128, 2, 53 }, { 132, 2, 53 },
  { 136, 4, 53 }, { 142, 1, 48 }, { 143, 1, 48 },
  { 144, 1, 53 }, { 146, 1, 53 }, { 147, 1, 55 },
  { 148, 1, 57 }, { 149, 1, 53 }, { 150, 1, 57 },
  { 151, 1, 58 }, { 152, 1, 60 }, { 154, 1, 60 },
  { 155, 1, 58 }, { 156, 2, 57 }, { 158, 1, 55 },
  { 159, 1, 53 }, { 160, 1, 55 }, { 162, 1, 55 },
  { 164, 2, 55 }, { 166, 2, 53 }, { 168, 1, 55 },
  { 169, 1, 57 }, { 170, 1, 55 }, { 171, 1, 55 },
  { 172, 2, 48 }, { 174, 1, 36 }, { 175, 1, 36 },
  { 176, 1, 41 }, { 178, 1, 41 }, { 179, 1, 43 },
  { 180, 1, 45 }, { 181, 1, 41 }, { 182, 1, 45 },
  { 183, 1, 46 }, { 184, 1, 48 }, { 186, 1, 48 },
  { 187, 1, 46 }, { 188, 2, 45 }, { 190, 1, 43 },
  { 191, 1, 40 }, { 192, 1, 43 }, { 194, 1, 43 },
  { 196, 2, 43 }, { 198, 1, 45 }, { 199, 1, 43 },
  { 200, 2, 41 }, { 204, 2, 41 }, { 208, 8, 41 },
};
const uint16_t PART9_LEN = sizeof(PART9) / sizeof(PART9[0]);

const Event PART10[] PROGMEM = {
  { 6, 1, 48 }, { 7, 1, 48 }, { 8, 1, 53 },
  { 10, 1, 53 }, { 11, 1, 55 }, { 12, 1, 57 },
  { 13, 1, 53 }, { 14, 1, 57 }, { 15, 1, 58 },
  { 16, 1, 60 }, { 18, 1, 60 }, { 19, 1, 58 },
  { 20, 2, 57 }, { 22, 1, 55 }, { 23, 1, 53 },
  { 24, 2, 55 }, { 26, 2, 48 }, { 28, 2, 55 },
  { 30, 2, 48 }, { 32, 1, 55 }, { 33, 1, 57 },
  { 34, 1, 55 }, { 35, 1, 52 }, { 36, 1, 48 },
  { 38, 1, 48 }, { 39, 1, 48 }, { 40, 2, 48 },
  { 44, 2, 48 }, { 46, 1, 53 }, { 48, 2, 53 },
  { 52, 2, 53 }, { 54, 1, 48 }, { 56, 1, 48 },
  { 58, 1, 48 }, { 60, 4, 48 }, { 64, 1, 46 },
  { 66, 2, 46 }, { 68, 4, 45 }, { 72, 2, 48 },
  { 76, 1, 48 }, { 78, 1, 48 }, { 80, 1, 48 },
  { 82, 1, 48 }, { 84, 1, 48 }, { 86, 1, 48 },
  { 88, 2, 48 }, { 92, 2, 48 }, { 96, 1, 48 },
  { 98, 1, 48 }, { 100, 2, 48 }, { 104, 4, 57 },
  { 108, 2, 53 }, { 110, 2, 48 }, { 112, 1, 53 },
  { 113, 1, 55 }, { 114, 1, 53 }, { 115, 1, 48 },
  { 116, 2, 45 }, { 118, 2, 41 }, { 120, 1, 48 },
  { 122, 2, 48 }, { 124, 2, 52 }, { 126, 2, 48 },
  { 128, 4, 50 }, { 132, 4, 46 }, { 136, 4, 45 },
  { 142, 1, 48 }, { 143, 1, 48 }, { 144, 2, 48 },
  { 146, 1, 45 }, { 147, 1, 48 }, { 148, 1, 53 },
  { 149, 1, 48 }, { 150, 1, 53 }, { 151, 1, 55 },
  { 152, 1, 57 }, { 154, 1, 57 }, { 155, 1, 53 },
  { 156, 2, 53 }, { 158, 1, 48 }, { 160, 1, 48 },
  { 162, 1, 48 }, { 164, 1, 48 }, { 166, 1, 48 },
  { 168, 2, 48 }, { 172, 2, 48 }, { 174, 1, 36 },
  { 175, 1, 36 }, { 176, 4, 36 }, { 180, 4, 38 },
  { 184, 1, 41 }, { 186, 1, 41 }, { 188, 2, 41 },
  { 190, 2, 36 }, { 192, 1, 41 }, { 194, 1, 41 },
  { 196, 2, 41 }, { 198, 2, 36 }, { 200, 4, 38 },
  { 204, 4, 34 }, { 208, 8, 36 },
};
const uint16_t PART10_LEN = sizeof(PART10) / sizeof(PART10[0]);

// ===== SONG 6 DATA =====

const Event PART11[] PROGMEM = {
  { 0, 1, 41 }, { 1, 3, 46 }, { 4, 1, 50 },
  { 5, 3, 53 }, { 8, 1, 46 }, { 9, 1, 51 },
  { 10, 1, 55 }, { 11, 1, 58 }, { 12, 1, 55 },
  { 13, 3, 53 }, { 16, 1, 50 }, { 17, 1, 51 },
  { 18, 1, 48 }, { 19, 1, 41 }, { 20, 1, 51 },
  { 21, 1, 50 }, { 22, 1, 46 }, { 23, 1, 41 },
  { 24, 1, 50 }, { 25, 2, 45 }, { 27, 2, 43 },
  { 29, 2, 41 }, { 32, 1, 41 }, { 33, 3, 46 },
  { 36, 1, 50 }, { 37, 3, 53 }, { 40, 1, 46 },
  { 41, 1, 51 }, { 42, 1, 55 }, { 43, 1, 58 },
  { 44, 1, 55 }, { 45, 3, 53 }, { 48, 1, 50 },
  { 49, 1, 51 }, { 50, 1, 48 }, { 51, 1, 41 },
  { 52, 1, 51 }, { 53, 1, 50 }, { 54, 1, 46 },
  { 55, 1, 41 }, { 56, 1, 50 }, { 57, 2, 45 },
  { 59, 2, 43 }, { 61, 2, 41 }, { 64, 1, 41 },
  { 65, 3, 51 }, { 68, 1, 50 }, { 69, 3, 48 },
  { 72, 1, 41 }, { 73, 1, 51 }, { 74, 1, 50 },
  { 75, 1, 48 }, { 76, 1, 46 }, { 77, 3, 48 },
  { 80, 1, 41 }, { 81, 3, 46 }, { 84, 1, 50 },
  { 85, 3, 53 }, { 88, 1, 46 }, { 89, 1, 51 },
  { 90, 1, 55 }, { 91, 1, 58 }, { 92, 1, 55 },
  { 93, 3, 53 }, { 96, 1, 50 }, { 97, 1, 51 },
  { 98, 1, 48 }, { 99, 1, 41 }, { 100, 1, 51 },
  { 101, 1, 50 }, { 102, 1, 46 }, { 103, 1, 41 },
  { 104, 1, 50 }, { 105, 2, 48 }, { 107, 2, 45 },
  { 109, 2, 46 }, { 112, 1, 41 }, { 113, 3, 51 },
  { 116, 1, 50 }, { 117, 3, 48 }, { 120, 1, 41 },
  { 121, 1, 51 }, { 122, 1, 50 }, { 123, 1, 48 },
  { 124, 1, 46 }, { 125, 3, 48 }, { 128, 1, 41 },
  { 129, 3, 46 }, { 132, 1, 50 }, { 133, 3, 53 },
  { 136, 1, 46 }, { 137, 1, 51 }, { 138, 1, 55 },
  { 139, 1, 58 }, { 140, 1, 55 }, { 141, 3, 53 },
  { 144, 1, 50 }, { 145, 1, 51 }, { 146, 1, 48 },
  { 147, 1, 41 }, { 148, 1, 51 }, { 149, 1, 50 },
  { 150, 1, 46 }, { 151, 1, 41 }, { 152, 1, 50 },
  { 153, 2, 48 }, { 155, 2, 45 }, { 157, 2, 46 },
};
const uint16_t PART11_LEN = sizeof(PART11) / sizeof(PART11[0]);

const Event PART12[] PROGMEM = {};
const uint16_t PART12_LEN = 0;
// ===== SONG ARRAY =====
const Song SONGS[] = {
  // Song 1
  {
    PART1, PART1_LEN,
    PART2, PART2_LEN,
    4,    // ticksPerQuarter
    180,  // tempoBPM
    3,    // timeBeats
    4,    // timeBeatType
    864   // totalTicks
  },
  // Song 2
  {
    PART3, PART3_LEN,
    PART4, PART4_LEN,
    2,    // ticksPerQuarter
    170,  // tempoBPM
    6,    // timeBeats
    8,    // timeBeatType
    390   // totalTicks
  },
  // Song 3
  {
    PART5, PART5_LEN,
    PART6, PART6_LEN,
    8,    // ticksPerQuarter
    150,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    512   // totalTicks
  },
  // Song 4
  {
    PART7, PART7_LEN,
    PART8, PART8_LEN,
    4,    // ticksPerQuarter
    100,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    180   // totalTicks
  },
  // Song 5
  {
    PART9, PART9_LEN,
    PART10, PART10_LEN,
    2,    // ticksPerQuarter
    92,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    216   // totalTicks
  },
  // Song 6
  {
    PART11, PART11_LEN,
    PART12, PART12_LEN,
    2,    // ticksPerQuarter
    120,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    159   // totalTicks
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
  { { 0x1B, 0x79, 0xF3, 0x00 }, 4, "crossing field", "Intense" },
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