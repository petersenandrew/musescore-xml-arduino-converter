// Acknowledgments
// Music player with RFID song selection
// RFID code based on https://github.com/miguelbalboa/rfid

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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

const uint16_t NOTE_FREQS[] PROGMEM = {
  0, 73, 92, 98, 110, 123, 131, 147, 165, 185,
  196, 220, 247, 262, 277, 294, 311, 330, 370, 392,
  440, 466, 494, 523, 554, 587, 622, 659, 740, 784,
  831, 880, 932, 988, 1109, 1175, 1319,
};

const Event PART1[] PROGMEM = {
  { 96, 4, 25 }, { 100, 4, 24 }, { 104, 4, 25 },
  { 108, 4, 22 }, { 112, 4, 20 }, { 116, 4, 19 },
  { 120, 8, 20 }, { 128, 4, 18 }, { 132, 8, 15 },
  { 144, 12, 20 }, { 156, 12, 15 }, { 168, 12, 25 },
  { 180, 12, 22 }, { 192, 4, 25 }, { 196, 4, 24 },
  { 200, 4, 25 }, { 204, 4, 22 }, { 208, 4, 20 },
  { 212, 4, 19 }, { 216, 8, 20 }, { 224, 4, 18 },
  { 228, 8, 15 }, { 240, 4, 19 }, { 244, 4, 25 },
  { 248, 4, 19 }, { 252, 4, 18 }, { 256, 4, 15 },
  { 260, 4, 20 }, { 264, 10, 19 }, { 276, 8, 19 },
  { 288, 4, 22 }, { 292, 4, 23 }, { 296, 4, 22 },
  { 300, 4, 20 }, { 304, 4, 19 }, { 308, 4, 18 },
  { 312, 8, 22 }, { 320, 4, 19 }, { 324, 8, 17 },
  { 336, 4, 25 }, { 340, 4, 23 }, { 344, 4, 22 },
  { 348, 8, 20 }, { 356, 4, 21 }, { 360, 10, 22 },
  { 372, 4, 22 }, { 384, 4, 22 }, { 388, 4, 23 },
  { 392, 4, 22 }, { 396, 4, 20 }, { 400, 4, 19 },
  { 404, 4, 18 }, { 408, 8, 22 }, { 416, 4, 19 },
  { 420, 8, 17 }, { 428, 4, 22 }, { 432, 4, 20 },
  { 436, 4, 26 }, { 440, 4, 24 }, { 444, 8, 22 },
  { 452, 4, 20 }, { 456, 10, 25 }, { 468, 8, 25 },
  { 480, 12, 25 }, { 492, 12, 22 }, { 504, 12, 19 },
  { 516, 12, 15 }, { 528, 4, 17 }, { 532, 4, 18 },
  { 536, 4, 19 }, { 540, 8, 17 }, { 548, 4, 19 },
  { 552, 10, 15 }, { 564, 8, 15 }, { 576, 12, 20 },
  { 588, 12, 25 }, { 600, 12, 22 }, { 612, 12, 19 },
  { 624, 4, 17 }, { 628, 4, 18 }, { 632, 4, 19 },
  { 636, 8, 20 }, { 644, 4, 22 }, { 648, 10, 20 },
  { 660, 4, 20 }, { 668, 4, 22 }, { 672, 4, 23 },
  { 676, 4, 22 }, { 680, 4, 20 }, { 684, 8, 25 },
  { 692, 4, 22 }, { 696, 4, 20 }, { 700, 6, 19 },
  { 708, 4, 19 }, { 716, 4, 20 }, { 720, 8, 22 },
  { 730, 2, 19 }, { 732, 8, 17 }, { 742, 2, 19 },
  { 744, 4, 17 }, { 748, 6, 15 }, { 756, 4, 15 },
  { 764, 4, 15 }, { 768, 8, 19 }, { 776, 4, 22 },
  { 780, 3, 20 }, { 792, 8, 19 }, { 800, 4, 22 },
  { 804, 3, 20 }, { 812, 2, 22 }, { 814, 2, 23 },
  { 816, 4, 25 }, { 820, 4, 22 }, { 824, 4, 19 },
  { 828, 8, 20 }, { 836, 4, 15 }, { 840, 10, 19 },
  { 852, 12, 19 },
};
const uint16_t PART1_LEN = sizeof(PART1) / sizeof(PART1[0]);

const Event PART2[] PROGMEM = {
  { 0, 4, 33 }, { 4, 4, 32 }, { 8, 4, 33 },
  { 12, 4, 31 }, { 16, 4, 29 }, { 20, 4, 28 },
  { 24, 8, 29 }, { 32, 4, 27 }, { 36, 8, 25 },
  { 44, 4, 33 }, { 48, 2, 31 }, { 50, 2, 25 },
  { 52, 2, 26 }, { 54, 2, 27 }, { 56, 2, 28 },
  { 58, 2, 29 }, { 60, 2, 30 }, { 62, 2, 31 },
  { 64, 2, 34 }, { 66, 2, 33 }, { 68, 2, 31 },
  { 70, 2, 28 }, { 72, 8, 29 }, { 80, 4, 25 },
  { 84, 12, 28 }, { 96, 4, 10 }, { 100, 4, 15 },
  { 104, 4, 19 }, { 108, 4, 10 }, { 112, 4, 15 },
  { 116, 4, 19 }, { 120, 4, 9 }, { 124, 4, 15 },
  { 128, 4, 20 }, { 132, 4, 7 }, { 136, 4, 15 },
  { 140, 4, 18 }, { 144, 4, 9 }, { 148, 4, 15 },
  { 152, 4, 20 }, { 156, 4, 7 }, { 160, 4, 15 },
  { 164, 4, 18 }, { 168, 4, 10 }, { 172, 4, 15 },
  { 176, 4, 19 }, { 180, 4, 7 }, { 184, 4, 15 },
  { 188, 4, 19 }, { 192, 4, 10 }, { 196, 4, 15 },
  { 200, 4, 19 }, { 204, 4, 10 }, { 208, 4, 15 },
  { 212, 4, 19 }, { 216, 4, 9 }, { 220, 4, 15 },
  { 224, 4, 20 }, { 228, 4, 7 }, { 232, 4, 15 },
  { 236, 4, 18 }, { 240, 4, 10 }, { 244, 4, 15 },
  { 248, 4, 19 }, { 252, 4, 7 }, { 256, 4, 15 },
  { 260, 4, 18 }, { 264, 4, 10 }, { 268, 4, 7 },
  { 272, 4, 5 }, { 276, 8, 3 }, { 288, 4, 8 },
  { 292, 4, 12 }, { 296, 4, 17 }, { 300, 4, 5 },
  { 304, 4, 12 }, { 308, 4, 16 }, { 312, 4, 8 },
  { 316, 4, 12 }, { 320, 4, 17 }, { 324, 4, 8 },
  { 328, 4, 10 }, { 332, 4, 12 }, { 336, 4, 9 },
  { 340, 4, 13 }, { 344, 4, 15 }, { 348, 4, 9 },
  { 352, 4, 13 }, { 356, 4, 15 }, { 360, 4, 10 },
  { 364, 2, 21 }, { 366, 2, 22 }, { 368, 2, 25 },
  { 370, 2, 28 }, { 372, 8, 31 }, { 384, 4, 8 },
  { 388, 4, 12 }, { 392, 4, 17 }, { 396, 4, 5 },
  { 400, 4, 12 }, { 404, 4, 16 }, { 408, 4, 8 },
  { 412, 4, 12 }, { 416, 4, 17 }, { 420, 4, 8 },
  { 424, 4, 12 }, { 428, 4, 17 }, { 432, 4, 4 },
  { 436, 4, 11 }, { 440, 4, 14 }, { 444, 4, 4 },
  { 448, 4, 11 }, { 452, 4, 14 }, { 456, 4, 7 },
  { 460, 4, 18 }, { 464, 4, 20 }, { 468, 8, 15 },
  { 480, 4, 10 }, { 484, 4, 19 }, { 488, 4, 22 },
  { 492, 4, 12 }, { 496, 4, 15 }, { 500, 4, 19 },
  { 504, 4, 5 }, { 508, 4, 10 }, { 512, 4, 15 },
  { 516, 4, 3 }, { 520, 4, 10 }, { 524, 4, 12 },
  { 528, 4, 6 }, { 532, 4, 10 }, { 536, 4, 13 },
  { 540, 4, 6 }, { 544, 4, 10 }, { 548, 4, 13 },
  { 552, 4, 3 }, { 556, 4, 7 }, { 560, 4, 5 },
  { 564, 4, 3 }, { 568, 4, 10 }, { 572, 4, 12 },
  { 576, 4, 9 }, { 580, 4, 13 }, { 584, 4, 15 },
  { 588, 4, 7 }, { 592, 4, 13 }, { 596, 4, 15 },
  { 600, 4, 10 }, { 604, 4, 15 }, { 608, 4, 19 },
  { 612, 4, 8 }, { 616, 4, 12 }, { 620, 4, 17 },
  { 624, 4, 4 }, { 628, 4, 11 }, { 632, 4, 14 },
  { 636, 4, 4 }, { 640, 4, 11 }, { 644, 4, 14 },
  { 648, 4, 7 }, { 652, 4, 11 }, { 656, 4, 15 },
  { 660, 4, 7 }, { 664, 4, 11 }, { 668, 4, 15 },
  { 672, 4, 9 }, { 676, 4, 13 }, { 680, 4, 15 },
  { 684, 4, 7 }, { 688, 4, 13 }, { 692, 4, 15 },
  { 696, 4, 10 }, { 700, 4, 12 }, { 704, 4, 15 },
  { 708, 4, 10 }, { 712, 4, 12 }, { 716, 4, 15 },
  { 720, 4, 8 }, { 724, 4, 10 }, { 730, 2, 12 },
  { 732, 4, 6 }, { 736, 4, 10 }, { 742, 2, 13 },
  { 744, 4, 3 }, { 748, 4, 10 }, { 752, 4, 12 },
  { 756, 4, 2 }, { 760, 4, 11 }, { 764, 4, 13 },
  { 768, 4, 3 }, { 772, 4, 10 }, { 776, 4, 12 },
  { 780, 3, 13 }, { 792, 4, 3 }, { 796, 4, 10 },
  { 800, 4, 12 }, { 804, 3, 13 }, { 816, 4, 3 },
  { 820, 4, 10 }, { 824, 4, 12 }, { 828, 4, 1 },
  { 832, 4, 9 }, { 836, 4, 13 }, { 840, 4, 10 },
  { 844, 4, 7 }, { 848, 4, 5 }, { 852, 12, 3 },
};
const uint16_t PART2_LEN = sizeof(PART2) / sizeof(PART2[0]);


// ===== SONG 2 DATA =====

const Event PART3[] PROGMEM = {
  { 0, 1, 20 }, { 1, 1, 20 }, { 2, 1, 20 },
  { 3, 1, 24 }, { 4, 1, 29 }, { 5, 1, 24 },
  { 6, 1, 22 }, { 7, 1, 22 }, { 8, 1, 22 },
  { 9, 1, 24 }, { 10, 1, 29 }, { 11, 1, 29 },
  { 12, 1, 20 }, { 13, 1, 20 }, { 14, 1, 20 },
  { 15, 1, 24 }, { 16, 1, 29 }, { 17, 1, 24 },
  { 18, 1, 22 }, { 19, 1, 29 }, { 20, 1, 29 },
  { 21, 1, 27 }, { 22, 1, 26 }, { 23, 1, 24 },
  { 24, 1, 20 }, { 25, 1, 20 }, { 26, 1, 20 },
  { 27, 1, 24 }, { 28, 1, 29 }, { 29, 1, 24 },
  { 30, 1, 22 }, { 31, 1, 22 }, { 32, 1, 22 },
  { 33, 1, 24 }, { 34, 1, 29 }, { 35, 1, 29 },
  { 36, 2, 29 }, { 38, 1, 26 }, { 39, 1, 27 },
  { 40, 1, 26 }, { 41, 1, 24 }, { 42, 1, 22 },
  { 43, 1, 22 }, { 44, 1, 22 }, { 45, 1, 24 },
  { 46, 1, 29 }, { 47, 1, 29 }, { 48, 1, 20 },
  { 49, 1, 20 }, { 50, 1, 20 }, { 51, 1, 24 },
  { 52, 1, 29 }, { 53, 1, 24 }, { 54, 1, 22 },
  { 55, 1, 22 }, { 56, 1, 22 }, { 57, 1, 24 },
  { 58, 1, 29 }, { 59, 1, 29 }, { 60, 1, 20 },
  { 61, 1, 20 }, { 62, 1, 20 }, { 63, 1, 24 },
  { 64, 1, 29 }, { 65, 1, 24 }, { 66, 1, 22 },
  { 67, 1, 29 }, { 68, 1, 29 }, { 69, 1, 27 },
  { 70, 1, 26 }, { 71, 1, 24 }, { 72, 1, 20 },
  { 73, 1, 20 }, { 74, 1, 20 }, { 75, 1, 24 },
  { 76, 1, 29 }, { 77, 1, 24 }, { 78, 1, 22 },
  { 79, 1, 22 }, { 80, 1, 22 }, { 81, 1, 24 },
  { 82, 1, 29 }, { 83, 1, 29 }, { 84, 2, 29 },
  { 86, 1, 26 }, { 87, 1, 27 }, { 88, 1, 26 },
  { 89, 1, 24 }, { 90, 1, 22 }, { 91, 1, 22 },
  { 92, 1, 22 }, { 93, 1, 24 }, { 94, 1, 29 },
  { 95, 1, 29 }, { 96, 1, 27 }, { 97, 1, 29 },
  { 98, 1, 29 }, { 99, 1, 26 }, { 100, 1, 29 },
  { 101, 1, 29 }, { 102, 1, 24 }, { 103, 1, 29 },
  { 104, 1, 29 }, { 105, 1, 20 }, { 106, 1, 29 },
  { 107, 1, 29 }, { 108, 1, 27 }, { 109, 1, 29 },
  { 110, 1, 29 }, { 111, 1, 26 }, { 112, 1, 29 },
  { 113, 1, 29 }, { 114, 1, 24 }, { 115, 1, 29 },
  { 116, 1, 29 }, { 117, 1, 22 }, { 118, 1, 29 },
  { 119, 1, 29 }, { 120, 1, 27 }, { 121, 1, 29 },
  { 122, 1, 29 }, { 123, 1, 26 }, { 124, 1, 29 },
  { 125, 1, 29 }, { 126, 1, 24 }, { 127, 1, 29 },
  { 128, 1, 29 }, { 129, 1, 20 }, { 130, 1, 29 },
  { 131, 1, 29 }, { 132, 2, 29 }, { 134, 1, 26 },
  { 135, 1, 27 }, { 136, 1, 26 }, { 137, 1, 24 },
  { 138, 1, 22 }, { 139, 1, 22 }, { 140, 1, 22 },
  { 141, 1, 24 }, { 142, 1, 29 }, { 143, 1, 29 },
  { 144, 1, 27 }, { 145, 1, 29 }, { 146, 1, 29 },
  { 147, 1, 26 }, { 148, 1, 29 }, { 149, 1, 29 },
  { 150, 1, 24 }, { 151, 1, 29 }, { 152, 1, 29 },
  { 153, 1, 20 }, { 154, 1, 29 }, { 155, 1, 29 },
  { 156, 1, 27 }, { 157, 1, 29 }, { 158, 1, 29 },
  { 159, 1, 26 }, { 160, 1, 29 }, { 161, 1, 29 },
  { 162, 1, 24 }, { 163, 1, 29 }, { 164, 1, 29 },
  { 165, 1, 26 }, { 166, 1, 29 }, { 167, 1, 29 },
  { 168, 1, 27 }, { 169, 1, 29 }, { 170, 1, 29 },
  { 171, 1, 26 }, { 172, 1, 29 }, { 173, 1, 29 },
  { 174, 1, 24 }, { 175, 1, 29 }, { 176, 1, 29 },
  { 177, 1, 20 }, { 178, 1, 29 }, { 179, 1, 29 },
  { 180, 2, 29 }, { 182, 1, 26 }, { 183, 1, 27 },
  { 184, 1, 26 }, { 185, 1, 24 }, { 186, 1, 22 },
  { 187, 1, 22 }, { 188, 1, 22 }, { 189, 1, 24 },
  { 190, 1, 29 }, { 191, 1, 29 }, { 192, 1, 20 },
  { 193, 1, 20 }, { 194, 1, 20 }, { 195, 1, 24 },
  { 196, 1, 29 }, { 197, 1, 24 }, { 198, 1, 22 },
  { 199, 1, 22 }, { 200, 1, 22 }, { 201, 1, 24 },
  { 202, 1, 29 }, { 203, 1, 29 }, { 204, 1, 20 },
  { 205, 1, 20 }, { 206, 1, 20 }, { 207, 1, 24 },
  { 208, 1, 29 }, { 209, 1, 24 }, { 210, 1, 22 },
  { 211, 1, 29 }, { 212, 1, 29 }, { 213, 1, 27 },
  { 214, 1, 26 }, { 215, 1, 24 }, { 216, 1, 20 },
  { 217, 1, 20 }, { 218, 1, 20 }, { 219, 1, 24 },
  { 220, 1, 29 }, { 221, 1, 24 }, { 222, 1, 22 },
  { 223, 1, 22 }, { 224, 1, 22 }, { 225, 1, 24 },
  { 226, 1, 29 }, { 227, 1, 29 }, { 228, 2, 29 },
  { 230, 1, 26 }, { 231, 1, 27 }, { 232, 1, 26 },
  { 233, 1, 24 }, { 234, 1, 22 }, { 235, 1, 22 },
  { 236, 1, 22 }, { 237, 1, 24 }, { 238, 1, 29 },
  { 239, 1, 29 }, { 240, 1, 20 }, { 241, 1, 20 },
  { 242, 1, 20 }, { 243, 1, 24 }, { 244, 1, 29 },
  { 245, 1, 24 }, { 246, 1, 22 }, { 247, 1, 22 },
  { 248, 1, 22 }, { 249, 1, 24 }, { 250, 1, 29 },
  { 251, 1, 29 }, { 252, 1, 20 }, { 253, 1, 20 },
  { 254, 1, 20 }, { 255, 1, 24 }, { 256, 1, 29 },
  { 257, 1, 24 }, { 258, 1, 22 }, { 259, 1, 29 },
  { 260, 1, 29 }, { 261, 1, 27 }, { 262, 1, 26 },
  { 263, 1, 24 }, { 264, 1, 20 }, { 265, 1, 20 },
  { 266, 1, 20 }, { 267, 1, 24 }, { 268, 1, 29 },
  { 269, 1, 24 }, { 270, 1, 22 }, { 271, 1, 22 },
  { 272, 1, 22 }, { 273, 1, 24 }, { 274, 1, 29 },
  { 275, 1, 29 }, { 276, 2, 29 }, { 278, 1, 26 },
  { 279, 1, 27 }, { 280, 1, 26 }, { 281, 1, 24 },
  { 282, 1, 22 }, { 283, 1, 22 }, { 284, 1, 22 },
  { 285, 1, 24 }, { 286, 1, 29 }, { 287, 1, 29 },
  { 288, 1, 27 }, { 289, 1, 29 }, { 290, 1, 29 },
  { 291, 1, 26 }, { 292, 1, 29 }, { 293, 1, 29 },
  { 294, 1, 24 }, { 295, 1, 29 }, { 296, 1, 29 },
  { 297, 1, 20 }, { 298, 1, 29 }, { 299, 1, 29 },
  { 300, 1, 27 }, { 301, 1, 29 }, { 302, 1, 29 },
  { 303, 1, 26 }, { 304, 1, 29 }, { 305, 1, 29 },
  { 306, 1, 24 }, { 307, 1, 29 }, { 308, 1, 29 },
  { 309, 1, 22 }, { 310, 1, 29 }, { 311, 1, 29 },
  { 312, 1, 27 }, { 313, 1, 29 }, { 314, 1, 29 },
  { 315, 1, 26 }, { 316, 1, 29 }, { 317, 1, 29 },
  { 318, 1, 24 }, { 319, 1, 29 }, { 320, 1, 29 },
  { 321, 1, 20 }, { 322, 1, 29 }, { 323, 1, 29 },
  { 324, 2, 29 }, { 326, 1, 26 }, { 327, 1, 27 },
  { 328, 1, 26 }, { 329, 1, 24 }, { 330, 1, 22 },
  { 331, 1, 22 }, { 332, 1, 22 }, { 333, 1, 24 },
  { 334, 1, 29 }, { 335, 1, 29 }, { 336, 1, 27 },
  { 337, 1, 29 }, { 338, 1, 29 }, { 339, 1, 26 },
  { 340, 1, 29 }, { 341, 1, 29 }, { 342, 1, 24 },
  { 343, 1, 29 }, { 344, 1, 29 }, { 345, 1, 20 },
  { 346, 1, 29 }, { 347, 1, 29 }, { 348, 1, 27 },
  { 349, 1, 29 }, { 350, 1, 29 }, { 351, 1, 26 },
  { 352, 1, 29 }, { 353, 1, 29 }, { 354, 1, 24 },
  { 355, 1, 29 }, { 356, 1, 29 }, { 357, 1, 26 },
  { 358, 1, 29 }, { 359, 1, 29 }, { 360, 1, 27 },
  { 361, 1, 29 }, { 362, 1, 29 }, { 363, 1, 26 },
  { 364, 1, 29 }, { 365, 1, 29 }, { 366, 1, 24 },
  { 367, 1, 29 }, { 368, 1, 29 }, { 369, 1, 20 },
  { 370, 1, 29 }, { 371, 1, 29 }, { 372, 2, 29 },
  { 374, 1, 26 }, { 375, 1, 27 }, { 376, 1, 26 },
  { 377, 1, 24 }, { 378, 1, 22 }, { 379, 1, 22 },
  { 380, 1, 22 }, { 381, 1, 24 }, { 382, 1, 29 },
  { 383, 1, 29 }, { 384, 6, 20 },
};
const uint16_t PART3_LEN = sizeof(PART3) / sizeof(PART3[0]);

// ===== SONG 3 DATA =====

const Event PART4[] PROGMEM = {
  { 0, 16, 20 }, { 1, 1, 25 }, { 2, 6, 27 },
  { 8, 8, 26 }, { 16, 16, 27 }, { 20, 4, 29 },
  { 24, 8, 25 }, { 32, 16, 20 }, { 33, 1, 25 },
  { 34, 6, 27 }, { 40, 8, 26 }, { 48, 16, 27 },
  { 52, 4, 29 }, { 56, 8, 25 }, { 64, 16, 28 },
  { 68, 4, 27 }, { 72, 4, 26 }, { 76, 4, 25 },
  { 80, 16, 24 }, { 84, 4, 25 }, { 88, 4, 26 },
  { 92, 4, 24 }, { 96, 32, 25 }, { 100, 4, 26 },
  { 104, 4, 27 }, { 108, 4, 28 }, { 112, 8, 29 },
  { 120, 6, 20 }, { 128, 16, 20 }, { 129, 1, 25 },
  { 130, 6, 27 }, { 136, 8, 26 }, { 144, 16, 27 },
  { 148, 4, 29 }, { 152, 8, 25 }, { 160, 16, 20 },
  { 161, 1, 25 }, { 162, 6, 27 }, { 168, 8, 26 },
  { 176, 16, 27 }, { 180, 4, 29 }, { 184, 8, 25 },
  { 192, 16, 28 }, { 196, 4, 27 }, { 200, 4, 26 },
  { 204, 4, 25 }, { 208, 16, 24 }, { 212, 4, 25 },
  { 216, 4, 26 }, { 220, 4, 24 }, { 224, 32, 25 },
  { 228, 4, 20 }, { 232, 4, 18 }, { 236, 4, 20 },
  { 240, 16, 15 }, { 256, 16, 20 }, { 257, 1, 25 },
  { 258, 6, 27 }, { 264, 8, 26 }, { 272, 16, 27 },
  { 276, 4, 29 }, { 280, 8, 25 }, { 288, 16, 20 },
  { 289, 1, 25 }, { 290, 6, 27 }, { 296, 8, 26 },
  { 304, 16, 27 }, { 308, 4, 29 }, { 312, 8, 25 },
  { 320, 16, 28 }, { 324, 4, 27 }, { 328, 4, 26 },
  { 332, 4, 25 }, { 336, 16, 24 }, { 340, 4, 25 },
  { 344, 4, 26 }, { 348, 4, 24 }, { 352, 32, 25 },
  { 356, 4, 26 }, { 360, 4, 27 }, { 364, 4, 28 },
  { 368, 8, 29 }, { 376, 6, 20 }, { 384, 16, 20 },
  { 385, 1, 25 }, { 386, 6, 27 }, { 392, 8, 26 },
  { 400, 16, 27 }, { 404, 4, 29 }, { 408, 8, 25 },
  { 416, 16, 20 }, { 417, 1, 25 }, { 418, 6, 27 },
  { 424, 8, 26 }, { 432, 16, 27 }, { 436, 4, 29 },
  { 440, 8, 25 }, { 448, 16, 28 }, { 452, 4, 27 },
  { 456, 4, 26 }, { 460, 4, 25 }, { 464, 16, 24 },
  { 468, 4, 25 }, { 472, 4, 26 }, { 476, 4, 24 },
  { 480, 32, 25 }, { 484, 4, 20 }, { 488, 4, 18 },
  { 492, 4, 20 }, { 496, 16, 15 },
};
const uint16_t PART4_LEN = sizeof(PART4) / sizeof(PART4[0]);

// ===== SONG 3 DATA =====

const Event PART5[] PROGMEM = {
  { 2, 2, 27 }, { 4, 1, 31 }, { 6, 2, 31 },
  { 8, 1, 34 }, { 10, 2, 34 }, { 12, 4, 31 },
  { 16, 1, 27 }, { 18, 1, 27 }, { 20, 1, 27 },
  { 23, 1, 27 }, { 24, 1, 33 }, { 25, 1, 31 },
  { 26, 1, 30 }, { 27, 1, 28 }, { 28, 4, 27 },
  { 34, 2, 27 }, { 36, 1, 31 }, { 38, 2, 31 },
  { 40, 1, 34 }, { 42, 2, 34 }, { 44, 4, 31 },
  { 48, 2, 27 }, { 50, 2, 31 }, { 52, 2, 30 },
  { 54, 1, 28 }, { 55, 1, 30 }, { 56, 2, 31 },
  { 58, 2, 26 }, { 60, 4, 27 }, { 66, 2, 27 },
  { 68, 1, 30 }, { 70, 2, 30 }, { 72, 1, 31 },
  { 73, 1, 30 }, { 74, 1, 28 }, { 75, 1, 30 },
  { 76, 4, 31 }, { 80, 2, 27 }, { 82, 2, 31 },
  { 84, 1, 30 }, { 86, 1, 30 }, { 88, 1, 30 },
  { 89, 1, 35 }, { 90, 1, 33 }, { 91, 1, 30 },
  { 92, 4, 31 }, { 98, 2, 31 }, { 100, 1, 28 },
  { 102, 1, 28 }, { 104, 2, 28 }, { 106, 1, 31 },
  { 108, 4, 31 }, { 112, 1, 27 }, { 114, 1, 27 },
  { 116, 1, 27 }, { 119, 1, 27 }, { 120, 2, 33 },
  { 122, 2, 30 }, { 124, 4, 31 }, { 130, 2, 31 },
  { 132, 1, 30 }, { 133, 1, 28 }, { 134, 1, 28 },
  { 136, 1, 28 }, { 137, 1, 28 }, { 138, 1, 30 },
  { 139, 1, 33 }, { 140, 4, 31 }, { 144, 1, 27 },
  { 146, 1, 27 }, { 148, 1, 27 }, { 151, 1, 27 },
  { 152, 2, 33 }, { 154, 2, 30 }, { 156, 6, 31 },
  { 164, 4, 34 }, { 168, 2, 36 }, { 170, 2, 33 },
  { 172, 2, 34 }, { 174, 2, 31 }, { 176, 4, 18 },
};
const uint16_t PART5_LEN = sizeof(PART5) / sizeof(PART5[0]);

// ===== SONG 4 DATA =====

const Event PART6[] PROGMEM = {
  { 6, 1, 20 }, { 7, 1, 20 }, { 8, 1, 25 },
  { 10, 1, 25 }, { 11, 1, 27 }, { 12, 1, 28 },
  { 13, 1, 25 }, { 14, 1, 28 }, { 15, 1, 29 },
  { 16, 1, 31 }, { 18, 1, 31 }, { 19, 1, 29 },
  { 20, 2, 28 }, { 22, 1, 27 }, { 23, 1, 25 },
  { 24, 2, 27 }, { 26, 2, 20 }, { 28, 2, 27 },
  { 30, 2, 20 }, { 32, 1, 27 }, { 33, 1, 28 },
  { 34, 1, 27 }, { 35, 1, 27 }, { 36, 1, 20 },
  { 38, 1, 20 }, { 39, 1, 20 }, { 40, 1, 25 },
  { 42, 1, 25 }, { 43, 1, 27 }, { 44, 1, 28 },
  { 45, 1, 25 }, { 46, 1, 28 }, { 47, 1, 29 },
  { 48, 1, 31 }, { 50, 1, 31 }, { 51, 1, 29 },
  { 52, 2, 28 }, { 54, 1, 27 }, { 55, 1, 25 },
  { 56, 1, 27 }, { 58, 1, 27 }, { 60, 2, 27 },
  { 62, 1, 28 }, { 63, 1, 27 }, { 64, 1, 25 },
  { 66, 1, 25 }, { 68, 4, 25 }, { 72, 4, 20 },
  { 76, 2, 18 }, { 78, 2, 17 }, { 80, 1, 18 },
  { 81, 1, 19 }, { 82, 1, 18 }, { 83, 1, 17 },
  { 84, 2, 15 }, { 86, 2, 11 }, { 88, 1, 15 },
  { 90, 1, 15 }, { 91, 1, 17 }, { 92, 2, 18 },
  { 94, 1, 17 }, { 95, 1, 15 }, { 96, 1, 17 },
  { 98, 1, 17 }, { 100, 2, 17 }, { 104, 4, 31 },
  { 108, 2, 28 }, { 110, 2, 27 }, { 112, 1, 28 },
  { 113, 1, 29 }, { 114, 1, 28 }, { 115, 1, 27 },
  { 116, 2, 25 }, { 118, 2, 20 }, { 120, 1, 27 },
  { 122, 1, 27 }, { 124, 2, 27 }, { 126, 1, 28 },
  { 127, 1, 27 }, { 128, 2, 25 }, { 132, 2, 25 },
  { 136, 4, 25 }, { 142, 1, 20 }, { 143, 1, 20 },
  { 144, 1, 25 }, { 146, 1, 25 }, { 147, 1, 27 },
  { 148, 1, 28 }, { 149, 1, 25 }, { 150, 1, 28 },
  { 151, 1, 29 }, { 152, 1, 31 }, { 154, 1, 31 },
  { 155, 1, 29 }, { 156, 2, 28 }, { 158, 1, 27 },
  { 159, 1, 25 }, { 160, 1, 27 }, { 162, 1, 27 },
  { 164, 2, 27 }, { 166, 2, 25 }, { 168, 1, 27 },
  { 169, 1, 28 }, { 170, 1, 27 }, { 171, 1, 27 },
  { 172, 2, 20 }, { 174, 1, 11 }, { 175, 1, 11 },
  { 176, 1, 15 }, { 178, 1, 15 }, { 179, 1, 17 },
  { 180, 1, 18 }, { 181, 1, 15 }, { 182, 1, 18 },
  { 183, 1, 19 }, { 184, 1, 20 }, { 186, 1, 20 },
  { 187, 1, 19 }, { 188, 2, 18 }, { 190, 1, 17 },
  { 191, 1, 14 }, { 192, 1, 17 }, { 194, 1, 17 },
  { 196, 2, 17 }, { 198, 1, 18 }, { 199, 1, 17 },
  { 200, 2, 15 }, { 204, 2, 15 }, { 208, 8, 15 },
};
const uint16_t PART6_LEN = sizeof(PART6) / sizeof(PART6[0]);

const Event PART7[] PROGMEM = {
  { 6, 1, 20 }, { 7, 1, 20 }, { 8, 1, 25 },
  { 10, 1, 25 }, { 11, 1, 27 }, { 12, 1, 28 },
  { 13, 1, 25 }, { 14, 1, 28 }, { 15, 1, 29 },
  { 16, 1, 31 }, { 18, 1, 31 }, { 19, 1, 29 },
  { 20, 2, 28 }, { 22, 1, 27 }, { 23, 1, 25 },
  { 24, 2, 27 }, { 26, 2, 20 }, { 28, 2, 27 },
  { 30, 2, 20 }, { 32, 1, 27 }, { 33, 1, 28 },
  { 34, 1, 27 }, { 35, 1, 24 }, { 36, 1, 20 },
  { 38, 1, 20 }, { 39, 1, 20 }, { 40, 2, 20 },
  { 44, 2, 20 }, { 46, 1, 25 }, { 48, 2, 25 },
  { 52, 2, 25 }, { 54, 1, 20 }, { 56, 1, 20 },
  { 58, 1, 20 }, { 60, 4, 20 }, { 64, 1, 19 },
  { 66, 2, 19 }, { 68, 4, 18 }, { 72, 2, 20 },
  { 76, 1, 20 }, { 78, 1, 20 }, { 80, 1, 20 },
  { 82, 1, 20 }, { 84, 1, 20 }, { 86, 1, 20 },
  { 88, 2, 20 }, { 92, 2, 20 }, { 96, 1, 20 },
  { 98, 1, 20 }, { 100, 2, 20 }, { 104, 4, 28 },
  { 108, 2, 25 }, { 110, 2, 20 }, { 112, 1, 25 },
  { 113, 1, 27 }, { 114, 1, 25 }, { 115, 1, 20 },
  { 116, 2, 18 }, { 118, 2, 15 }, { 120, 1, 20 },
  { 122, 2, 20 }, { 124, 2, 24 }, { 126, 2, 20 },
  { 128, 4, 22 }, { 132, 4, 19 }, { 136, 4, 18 },
  { 142, 1, 20 }, { 143, 1, 20 }, { 144, 2, 20 },
  { 146, 1, 18 }, { 147, 1, 20 }, { 148, 1, 25 },
  { 149, 1, 20 }, { 150, 1, 25 }, { 151, 1, 27 },
  { 152, 1, 28 }, { 154, 1, 28 }, { 155, 1, 25 },
  { 156, 2, 25 }, { 158, 1, 20 }, { 160, 1, 20 },
  { 162, 1, 20 }, { 164, 1, 20 }, { 166, 1, 20 },
  { 168, 2, 20 }, { 172, 2, 20 }, { 174, 1, 11 },
  { 175, 1, 11 }, { 176, 4, 11 }, { 180, 4, 12 },
  { 184, 1, 15 }, { 186, 1, 15 }, { 188, 2, 15 },
  { 190, 2, 11 }, { 192, 1, 15 }, { 194, 1, 15 },
  { 196, 2, 15 }, { 198, 2, 11 }, { 200, 4, 12 },
  { 204, 4, 10 }, { 208, 8, 11 },
};
const uint16_t PART7_LEN = sizeof(PART7) / sizeof(PART7[0]);

// ===== SONG 5 DATA =====

const Event PART8[] PROGMEM = {
  { 0, 1, 15 }, { 1, 3, 19 }, { 4, 1, 22 },
  { 5, 3, 25 }, { 8, 1, 19 }, { 9, 1, 23 },
  { 10, 1, 27 }, { 11, 1, 29 }, { 12, 1, 27 },
  { 13, 3, 25 }, { 16, 1, 22 }, { 17, 1, 23 },
  { 18, 1, 20 }, { 19, 1, 15 }, { 20, 1, 23 },
  { 21, 1, 22 }, { 22, 1, 19 }, { 23, 1, 15 },
  { 24, 1, 22 }, { 25, 2, 18 }, { 27, 2, 17 },
  { 29, 2, 15 }, { 32, 1, 15 }, { 33, 3, 19 },
  { 36, 1, 22 }, { 37, 3, 25 }, { 40, 1, 19 },
  { 41, 1, 23 }, { 42, 1, 27 }, { 43, 1, 29 },
  { 44, 1, 27 }, { 45, 3, 25 }, { 48, 1, 22 },
  { 49, 1, 23 }, { 50, 1, 20 }, { 51, 1, 15 },
  { 52, 1, 23 }, { 53, 1, 22 }, { 54, 1, 19 },
  { 55, 1, 15 }, { 56, 1, 22 }, { 57, 2, 18 },
  { 59, 2, 17 }, { 61, 2, 15 }, { 64, 1, 15 },
  { 65, 3, 23 }, { 68, 1, 22 }, { 69, 3, 20 },
  { 72, 1, 15 }, { 73, 1, 23 }, { 74, 1, 22 },
  { 75, 1, 20 }, { 76, 1, 19 }, { 77, 3, 20 },
  { 80, 1, 15 }, { 81, 3, 19 }, { 84, 1, 22 },
  { 85, 3, 25 }, { 88, 1, 19 }, { 89, 1, 23 },
  { 90, 1, 27 }, { 91, 1, 29 }, { 92, 1, 27 },
  { 93, 3, 25 }, { 96, 1, 22 }, { 97, 1, 23 },
  { 98, 1, 20 }, { 99, 1, 15 }, { 100, 1, 23 },
  { 101, 1, 22 }, { 102, 1, 19 }, { 103, 1, 15 },
  { 104, 1, 22 }, { 105, 2, 20 }, { 107, 2, 18 },
  { 109, 2, 19 }, { 112, 1, 15 }, { 113, 3, 23 },
  { 116, 1, 22 }, { 117, 3, 20 }, { 120, 1, 15 },
  { 121, 1, 23 }, { 122, 1, 22 }, { 123, 1, 20 },
  { 124, 1, 19 }, { 125, 3, 20 }, { 128, 1, 15 },
  { 129, 3, 19 }, { 132, 1, 22 }, { 133, 3, 25 },
  { 136, 1, 19 }, { 137, 1, 23 }, { 138, 1, 27 },
  { 139, 1, 29 }, { 140, 1, 27 }, { 141, 3, 25 },
  { 144, 1, 22 }, { 145, 1, 23 }, { 146, 1, 20 },
  { 147, 1, 15 }, { 148, 1, 23 }, { 149, 1, 22 },
  { 150, 1, 19 }, { 151, 1, 15 }, { 152, 1, 22 },
  { 153, 2, 20 }, { 155, 2, 18 }, { 157, 2, 19 },
};
const uint16_t PART8_LEN = sizeof(PART8) / sizeof(PART8[0]);
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
    NULL, 0,
    2,    // ticksPerQuarter
    170,  // tempoBPM
    6,    // timeBeats
    8,    // timeBeatType
    390   // totalTicks
  },
  // Song 3
  {
    PART4, PART4_LEN,
    NULL, 0,
    8,    // ticksPerQuarter
    150,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    512   // totalTicks
  },
  // Song 3
  {
    PART5, PART5_LEN,
    NULL, 0,
    4,    // ticksPerQuarter
    100,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    180   // totalTicks
  },
  // Song 4
  {
    PART6, PART6_LEN,
    PART7, PART7_LEN,
    2,    // ticksPerQuarter
    92,  // tempoBPM
    4,    // timeBeats
    4,    // timeBeatType
    216   // totalTicks
  },
  // Song 5
  {
    PART8, PART8_LEN,
    NULL, 0,
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
  uint8_t songIndex;
  const char* title;
} RFIDMapping;

const RFIDMapping RFID_MAPPINGS[] = {
  // Example mappings - replace with your actual RFID UIDs
  { { 0x1B, 0x79, 0xF3, 0x00 }, 0, "Daisy Bell" },  // Card 1 -> Song 1
  { { 0x86, 0x8E, 0x5A, 0x30 }, 1, "Glasgow City Police Pipers" },  // Card 2 -> Song 2
  { { 0x09, 0x63, 0x15, 0xC1 }, 2, "Homecoming" },  // Card 3 -> Song 3
  { { 0x63, 0x4A, 0xE9, 0xFB }, 3, "Laundry Machine" }, // Card 4 -> Song 4
  { { 0x33, 0x0E, 0xDE, 0x0B }, 4, "Simple Gifts" }, // Card 5 -> Song 5
  { { 0x86, 0x06, 0xCA, 0x30 }, 5, "The Happy Farmer" }, // Card 6 -> Song 6

};
const uint8_t NUM_RFID_MAPPINGS = sizeof(RFID_MAPPINGS) / sizeof(RFID_MAPPINGS[0]);

const uint8_t SONG_TO_PLAY = 255;
// SONG_DATA_END

// RFID Pins (Arduino Mega)
#define RST_PIN 5  // Moved near SPI pins for cleaner wiring
#define SS_PIN 53   // SPI Slave Select

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
      return RFID_MAPPINGS[i].songIndex;
    }
  }
  currentSongTitle = NULL;
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
  lcd.setCursor(0, 0);
  if (currentSongTitle != NULL) {
    lcd.print(currentSongTitle);
  } else {
    lcd.print(F("Song "));
    lcd.print(songIndex + 1);
  }
  lcd.setCursor(0, 1);
  lcd.print(song.tempoBPM);
  lcd.print(F(" BPM"));
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