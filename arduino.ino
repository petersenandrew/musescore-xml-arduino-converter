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
  73, 92, 98, 110, 123, 131, 147, 165, 185, 196,
  220, 247, 262, 277, 294, 311, 330, 370, 392, 440,
  466, 494, 523, 554, 587, 659, 740, 784, 880, 932,
  988, 1109, 1175, 1319,
};

const Event PART1[] PROGMEM = {
  { 96, 4, 24 }, { 100, 4, 23 }, { 104, 4, 24 },
  { 108, 4, 21 }, { 112, 4, 19 }, { 116, 4, 18 },
  { 120, 8, 19 }, { 128, 4, 17 }, { 132, 8, 14 },
  { 144, 12, 19 }, { 156, 12, 14 }, { 168, 12, 24 },
  { 180, 12, 21 }, { 192, 4, 24 }, { 196, 4, 23 },
  { 200, 4, 24 }, { 204, 4, 21 }, { 208, 4, 19 },
  { 212, 4, 18 }, { 216, 8, 19 }, { 224, 4, 17 },
  { 228, 8, 14 }, { 240, 4, 18 }, { 244, 4, 24 },
  { 248, 4, 18 }, { 252, 4, 17 }, { 256, 4, 14 },
  { 260, 4, 19 }, { 264, 10, 18 }, { 276, 8, 18 },
  { 288, 4, 21 }, { 292, 4, 22 }, { 296, 4, 21 },
  { 300, 4, 19 }, { 304, 4, 18 }, { 308, 4, 17 },
  { 312, 8, 21 }, { 320, 4, 18 }, { 324, 8, 16 },
  { 336, 4, 24 }, { 340, 4, 22 }, { 344, 4, 21 },
  { 348, 8, 19 }, { 356, 4, 20 }, { 360, 10, 21 },
  { 372, 4, 21 }, { 384, 4, 21 }, { 388, 4, 22 },
  { 392, 4, 21 }, { 396, 4, 19 }, { 400, 4, 18 },
  { 404, 4, 17 }, { 408, 8, 21 }, { 416, 4, 18 },
  { 420, 8, 16 }, { 428, 4, 21 }, { 432, 4, 19 },
  { 436, 4, 25 }, { 440, 4, 23 }, { 444, 8, 21 },
  { 452, 4, 19 }, { 456, 10, 24 }, { 468, 8, 24 },
  { 480, 12, 24 }, { 492, 12, 21 }, { 504, 12, 18 },
  { 516, 12, 14 }, { 528, 4, 16 }, { 532, 4, 17 },
  { 536, 4, 18 }, { 540, 8, 16 }, { 548, 4, 18 },
  { 552, 10, 14 }, { 564, 8, 14 }, { 576, 12, 19 },
  { 588, 12, 24 }, { 600, 12, 21 }, { 612, 12, 18 },
  { 624, 4, 16 }, { 628, 4, 17 }, { 632, 4, 18 },
  { 636, 8, 19 }, { 644, 4, 21 }, { 648, 10, 19 },
  { 660, 4, 19 }, { 668, 4, 21 }, { 672, 4, 22 },
  { 676, 4, 21 }, { 680, 4, 19 }, { 684, 8, 24 },
  { 692, 4, 21 }, { 696, 4, 19 }, { 700, 6, 18 },
  { 708, 4, 18 }, { 716, 4, 19 }, { 720, 8, 21 },
  { 730, 2, 18 }, { 732, 8, 16 }, { 742, 2, 18 },
  { 744, 4, 16 }, { 748, 6, 14 }, { 756, 4, 14 },
  { 764, 4, 14 }, { 768, 8, 18 }, { 776, 4, 21 },
  { 780, 3, 19 }, { 792, 8, 18 }, { 800, 4, 21 },
  { 804, 3, 19 }, { 812, 2, 21 }, { 814, 2, 22 },
  { 816, 4, 24 }, { 820, 4, 21 }, { 824, 4, 18 },
  { 828, 8, 19 }, { 836, 4, 14 }, { 840, 10, 18 },
  { 852, 12, 18 },
};
const uint16_t PART1_LEN = sizeof(PART1) / sizeof(PART1[0]);

const Event PART2[] PROGMEM = {
  { 0, 4, 32 }, { 4, 4, 31 }, { 8, 4, 32 },
  { 12, 4, 30 }, { 16, 4, 28 }, { 20, 4, 27 },
  { 24, 8, 28 }, { 32, 4, 26 }, { 36, 8, 24 },
  { 44, 4, 32 }, { 48, 2, 30 }, { 50, 2, 24 },
  { 52, 2, 25 }, { 54, 2, 26 }, { 56, 2, 27 },
  { 58, 2, 28 }, { 60, 2, 29 }, { 62, 2, 30 },
  { 64, 2, 33 }, { 66, 2, 32 }, { 68, 2, 30 },
  { 70, 2, 27 }, { 72, 8, 28 }, { 80, 4, 24 },
  { 84, 12, 27 }, { 96, 4, 9 }, { 100, 4, 14 },
  { 104, 4, 18 }, { 108, 4, 9 }, { 112, 4, 14 },
  { 116, 4, 18 }, { 120, 4, 8 }, { 124, 4, 14 },
  { 128, 4, 19 }, { 132, 4, 6 }, { 136, 4, 14 },
  { 140, 4, 17 }, { 144, 4, 8 }, { 148, 4, 14 },
  { 152, 4, 19 }, { 156, 4, 6 }, { 160, 4, 14 },
  { 164, 4, 17 }, { 168, 4, 9 }, { 172, 4, 14 },
  { 176, 4, 18 }, { 180, 4, 6 }, { 184, 4, 14 },
  { 188, 4, 18 }, { 192, 4, 9 }, { 196, 4, 14 },
  { 200, 4, 18 }, { 204, 4, 9 }, { 208, 4, 14 },
  { 212, 4, 18 }, { 216, 4, 8 }, { 220, 4, 14 },
  { 224, 4, 19 }, { 228, 4, 6 }, { 232, 4, 14 },
  { 236, 4, 17 }, { 240, 4, 9 }, { 244, 4, 14 },
  { 248, 4, 18 }, { 252, 4, 6 }, { 256, 4, 14 },
  { 260, 4, 17 }, { 264, 4, 9 }, { 268, 4, 6 },
  { 272, 4, 4 }, { 276, 8, 2 }, { 288, 4, 7 },
  { 292, 4, 11 }, { 296, 4, 16 }, { 300, 4, 4 },
  { 304, 4, 11 }, { 308, 4, 15 }, { 312, 4, 7 },
  { 316, 4, 11 }, { 320, 4, 16 }, { 324, 4, 7 },
  { 328, 4, 9 }, { 332, 4, 11 }, { 336, 4, 8 },
  { 340, 4, 12 }, { 344, 4, 14 }, { 348, 4, 8 },
  { 352, 4, 12 }, { 356, 4, 14 }, { 360, 4, 9 },
  { 364, 2, 20 }, { 366, 2, 21 }, { 368, 2, 24 },
  { 370, 2, 27 }, { 372, 8, 30 }, { 384, 4, 7 },
  { 388, 4, 11 }, { 392, 4, 16 }, { 396, 4, 4 },
  { 400, 4, 11 }, { 404, 4, 15 }, { 408, 4, 7 },
  { 412, 4, 11 }, { 416, 4, 16 }, { 420, 4, 7 },
  { 424, 4, 11 }, { 428, 4, 16 }, { 432, 4, 3 },
  { 436, 4, 10 }, { 440, 4, 13 }, { 444, 4, 3 },
  { 448, 4, 10 }, { 452, 4, 13 }, { 456, 4, 6 },
  { 460, 4, 17 }, { 464, 4, 19 }, { 468, 8, 14 },
  { 480, 4, 9 }, { 484, 4, 18 }, { 488, 4, 21 },
  { 492, 4, 11 }, { 496, 4, 14 }, { 500, 4, 18 },
  { 504, 4, 4 }, { 508, 4, 9 }, { 512, 4, 14 },
  { 516, 4, 2 }, { 520, 4, 9 }, { 524, 4, 11 },
  { 528, 4, 5 }, { 532, 4, 9 }, { 536, 4, 12 },
  { 540, 4, 5 }, { 544, 4, 9 }, { 548, 4, 12 },
  { 552, 4, 2 }, { 556, 4, 6 }, { 560, 4, 4 },
  { 564, 4, 2 }, { 568, 4, 9 }, { 572, 4, 11 },
  { 576, 4, 8 }, { 580, 4, 12 }, { 584, 4, 14 },
  { 588, 4, 6 }, { 592, 4, 12 }, { 596, 4, 14 },
  { 600, 4, 9 }, { 604, 4, 14 }, { 608, 4, 18 },
  { 612, 4, 7 }, { 616, 4, 11 }, { 620, 4, 16 },
  { 624, 4, 3 }, { 628, 4, 10 }, { 632, 4, 13 },
  { 636, 4, 3 }, { 640, 4, 10 }, { 644, 4, 13 },
  { 648, 4, 6 }, { 652, 4, 10 }, { 656, 4, 14 },
  { 660, 4, 6 }, { 664, 4, 10 }, { 668, 4, 14 },
  { 672, 4, 8 }, { 676, 4, 12 }, { 680, 4, 14 },
  { 684, 4, 6 }, { 688, 4, 12 }, { 692, 4, 14 },
  { 696, 4, 9 }, { 700, 4, 11 }, { 704, 4, 14 },
  { 708, 4, 9 }, { 712, 4, 11 }, { 716, 4, 14 },
  { 720, 4, 7 }, { 724, 4, 9 }, { 730, 2, 11 },
  { 732, 4, 5 }, { 736, 4, 9 }, { 742, 2, 12 },
  { 744, 4, 2 }, { 748, 4, 9 }, { 752, 4, 11 },
  { 756, 4, 1 }, { 760, 4, 10 }, { 764, 4, 12 },
  { 768, 4, 2 }, { 772, 4, 9 }, { 776, 4, 11 },
  { 780, 3, 12 }, { 792, 4, 2 }, { 796, 4, 9 },
  { 800, 4, 11 }, { 804, 3, 12 }, { 816, 4, 2 },
  { 820, 4, 9 }, { 824, 4, 11 }, { 828, 4, 0 },
  { 832, 4, 8 }, { 836, 4, 12 }, { 840, 4, 9 },
  { 844, 4, 6 }, { 848, 4, 4 }, { 852, 12, 2 },
};
const uint16_t PART2_LEN = sizeof(PART2) / sizeof(PART2[0]);



// ===== SONG 2 DATA =====

const Event PART3[] PROGMEM = {
  { 0, 1, 19 }, { 1, 1, 19 }, { 2, 1, 19 },
  { 3, 1, 23 }, { 4, 1, 28 }, { 5, 1, 23 },
  { 6, 1, 21 }, { 7, 1, 21 }, { 8, 1, 21 },
  { 9, 1, 23 }, { 10, 1, 28 }, { 11, 1, 28 },
  { 12, 1, 19 }, { 13, 1, 19 }, { 14, 1, 19 },
  { 15, 1, 23 }, { 16, 1, 28 }, { 17, 1, 23 },
  { 18, 1, 21 }, { 19, 1, 28 }, { 20, 1, 28 },
  { 21, 1, 26 }, { 22, 1, 25 }, { 23, 1, 23 },
  { 24, 1, 19 }, { 25, 1, 19 }, { 26, 1, 19 },
  { 27, 1, 23 }, { 28, 1, 28 }, { 29, 1, 23 },
  { 30, 1, 21 }, { 31, 1, 21 }, { 32, 1, 21 },
  { 33, 1, 23 }, { 34, 1, 28 }, { 35, 1, 28 },
  { 36, 2, 28 }, { 38, 1, 25 }, { 39, 1, 26 },
  { 40, 1, 25 }, { 41, 1, 23 }, { 42, 1, 21 },
  { 43, 1, 21 }, { 44, 1, 21 }, { 45, 1, 23 },
  { 46, 1, 28 }, { 47, 1, 28 }, { 48, 1, 19 },
  { 49, 1, 19 }, { 50, 1, 19 }, { 51, 1, 23 },
  { 52, 1, 28 }, { 53, 1, 23 }, { 54, 1, 21 },
  { 55, 1, 21 }, { 56, 1, 21 }, { 57, 1, 23 },
  { 58, 1, 28 }, { 59, 1, 28 }, { 60, 1, 19 },
  { 61, 1, 19 }, { 62, 1, 19 }, { 63, 1, 23 },
  { 64, 1, 28 }, { 65, 1, 23 }, { 66, 1, 21 },
  { 67, 1, 28 }, { 68, 1, 28 }, { 69, 1, 26 },
  { 70, 1, 25 }, { 71, 1, 23 }, { 72, 1, 19 },
  { 73, 1, 19 }, { 74, 1, 19 }, { 75, 1, 23 },
  { 76, 1, 28 }, { 77, 1, 23 }, { 78, 1, 21 },
  { 79, 1, 21 }, { 80, 1, 21 }, { 81, 1, 23 },
  { 82, 1, 28 }, { 83, 1, 28 }, { 84, 2, 28 },
  { 86, 1, 25 }, { 87, 1, 26 }, { 88, 1, 25 },
  { 89, 1, 23 }, { 90, 1, 21 }, { 91, 1, 21 },
  { 92, 1, 21 }, { 93, 1, 23 }, { 94, 1, 28 },
  { 95, 1, 28 }, { 96, 1, 26 }, { 97, 1, 28 },
  { 98, 1, 28 }, { 99, 1, 25 }, { 100, 1, 28 },
  { 101, 1, 28 }, { 102, 1, 23 }, { 103, 1, 28 },
  { 104, 1, 28 }, { 105, 1, 19 }, { 106, 1, 28 },
  { 107, 1, 28 }, { 108, 1, 26 }, { 109, 1, 28 },
  { 110, 1, 28 }, { 111, 1, 25 }, { 112, 1, 28 },
  { 113, 1, 28 }, { 114, 1, 23 }, { 115, 1, 28 },
  { 116, 1, 28 }, { 117, 1, 21 }, { 118, 1, 28 },
  { 119, 1, 28 }, { 120, 1, 26 }, { 121, 1, 28 },
  { 122, 1, 28 }, { 123, 1, 25 }, { 124, 1, 28 },
  { 125, 1, 28 }, { 126, 1, 23 }, { 127, 1, 28 },
  { 128, 1, 28 }, { 129, 1, 19 }, { 130, 1, 28 },
  { 131, 1, 28 }, { 132, 2, 28 }, { 134, 1, 25 },
  { 135, 1, 26 }, { 136, 1, 25 }, { 137, 1, 23 },
  { 138, 1, 21 }, { 139, 1, 21 }, { 140, 1, 21 },
  { 141, 1, 23 }, { 142, 1, 28 }, { 143, 1, 28 },
  { 144, 1, 26 }, { 145, 1, 28 }, { 146, 1, 28 },
  { 147, 1, 25 }, { 148, 1, 28 }, { 149, 1, 28 },
  { 150, 1, 23 }, { 151, 1, 28 }, { 152, 1, 28 },
  { 153, 1, 19 }, { 154, 1, 28 }, { 155, 1, 28 },
  { 156, 1, 26 }, { 157, 1, 28 }, { 158, 1, 28 },
  { 159, 1, 25 }, { 160, 1, 28 }, { 161, 1, 28 },
  { 162, 1, 23 }, { 163, 1, 28 }, { 164, 1, 28 },
  { 165, 1, 25 }, { 166, 1, 28 }, { 167, 1, 28 },
  { 168, 1, 26 }, { 169, 1, 28 }, { 170, 1, 28 },
  { 171, 1, 25 }, { 172, 1, 28 }, { 173, 1, 28 },
  { 174, 1, 23 }, { 175, 1, 28 }, { 176, 1, 28 },
  { 177, 1, 19 }, { 178, 1, 28 }, { 179, 1, 28 },
  { 180, 2, 28 }, { 182, 1, 25 }, { 183, 1, 26 },
  { 184, 1, 25 }, { 185, 1, 23 }, { 186, 1, 21 },
  { 187, 1, 21 }, { 188, 1, 21 }, { 189, 1, 23 },
  { 190, 1, 28 }, { 191, 1, 28 }, { 192, 1, 19 },
  { 193, 1, 19 }, { 194, 1, 19 }, { 195, 1, 23 },
  { 196, 1, 28 }, { 197, 1, 23 }, { 198, 1, 21 },
  { 199, 1, 21 }, { 200, 1, 21 }, { 201, 1, 23 },
  { 202, 1, 28 }, { 203, 1, 28 }, { 204, 1, 19 },
  { 205, 1, 19 }, { 206, 1, 19 }, { 207, 1, 23 },
  { 208, 1, 28 }, { 209, 1, 23 }, { 210, 1, 21 },
  { 211, 1, 28 }, { 212, 1, 28 }, { 213, 1, 26 },
  { 214, 1, 25 }, { 215, 1, 23 }, { 216, 1, 19 },
  { 217, 1, 19 }, { 218, 1, 19 }, { 219, 1, 23 },
  { 220, 1, 28 }, { 221, 1, 23 }, { 222, 1, 21 },
  { 223, 1, 21 }, { 224, 1, 21 }, { 225, 1, 23 },
  { 226, 1, 28 }, { 227, 1, 28 }, { 228, 2, 28 },
  { 230, 1, 25 }, { 231, 1, 26 }, { 232, 1, 25 },
  { 233, 1, 23 }, { 234, 1, 21 }, { 235, 1, 21 },
  { 236, 1, 21 }, { 237, 1, 23 }, { 238, 1, 28 },
  { 239, 1, 28 }, { 240, 1, 19 }, { 241, 1, 19 },
  { 242, 1, 19 }, { 243, 1, 23 }, { 244, 1, 28 },
  { 245, 1, 23 }, { 246, 1, 21 }, { 247, 1, 21 },
  { 248, 1, 21 }, { 249, 1, 23 }, { 250, 1, 28 },
  { 251, 1, 28 }, { 252, 1, 19 }, { 253, 1, 19 },
  { 254, 1, 19 }, { 255, 1, 23 }, { 256, 1, 28 },
  { 257, 1, 23 }, { 258, 1, 21 }, { 259, 1, 28 },
  { 260, 1, 28 }, { 261, 1, 26 }, { 262, 1, 25 },
  { 263, 1, 23 }, { 264, 1, 19 }, { 265, 1, 19 },
  { 266, 1, 19 }, { 267, 1, 23 }, { 268, 1, 28 },
  { 269, 1, 23 }, { 270, 1, 21 }, { 271, 1, 21 },
  { 272, 1, 21 }, { 273, 1, 23 }, { 274, 1, 28 },
  { 275, 1, 28 }, { 276, 2, 28 }, { 278, 1, 25 },
  { 279, 1, 26 }, { 280, 1, 25 }, { 281, 1, 23 },
  { 282, 1, 21 }, { 283, 1, 21 }, { 284, 1, 21 },
  { 285, 1, 23 }, { 286, 1, 28 }, { 287, 1, 28 },
  { 288, 1, 26 }, { 289, 1, 28 }, { 290, 1, 28 },
  { 291, 1, 25 }, { 292, 1, 28 }, { 293, 1, 28 },
  { 294, 1, 23 }, { 295, 1, 28 }, { 296, 1, 28 },
  { 297, 1, 19 }, { 298, 1, 28 }, { 299, 1, 28 },
  { 300, 1, 26 }, { 301, 1, 28 }, { 302, 1, 28 },
  { 303, 1, 25 }, { 304, 1, 28 }, { 305, 1, 28 },
  { 306, 1, 23 }, { 307, 1, 28 }, { 308, 1, 28 },
  { 309, 1, 21 }, { 310, 1, 28 }, { 311, 1, 28 },
  { 312, 1, 26 }, { 313, 1, 28 }, { 314, 1, 28 },
  { 315, 1, 25 }, { 316, 1, 28 }, { 317, 1, 28 },
  { 318, 1, 23 }, { 319, 1, 28 }, { 320, 1, 28 },
  { 321, 1, 19 }, { 322, 1, 28 }, { 323, 1, 28 },
  { 324, 2, 28 }, { 326, 1, 25 }, { 327, 1, 26 },
  { 328, 1, 25 }, { 329, 1, 23 }, { 330, 1, 21 },
  { 331, 1, 21 }, { 332, 1, 21 }, { 333, 1, 23 },
  { 334, 1, 28 }, { 335, 1, 28 }, { 336, 1, 26 },
  { 337, 1, 28 }, { 338, 1, 28 }, { 339, 1, 25 },
  { 340, 1, 28 }, { 341, 1, 28 }, { 342, 1, 23 },
  { 343, 1, 28 }, { 344, 1, 28 }, { 345, 1, 19 },
  { 346, 1, 28 }, { 347, 1, 28 }, { 348, 1, 26 },
  { 349, 1, 28 }, { 350, 1, 28 }, { 351, 1, 25 },
  { 352, 1, 28 }, { 353, 1, 28 }, { 354, 1, 23 },
  { 355, 1, 28 }, { 356, 1, 28 }, { 357, 1, 25 },
  { 358, 1, 28 }, { 359, 1, 28 }, { 360, 1, 26 },
  { 361, 1, 28 }, { 362, 1, 28 }, { 363, 1, 25 },
  { 364, 1, 28 }, { 365, 1, 28 }, { 366, 1, 23 },
  { 367, 1, 28 }, { 368, 1, 28 }, { 369, 1, 19 },
  { 370, 1, 28 }, { 371, 1, 28 }, { 372, 2, 28 },
  { 374, 1, 25 }, { 375, 1, 26 }, { 376, 1, 25 },
  { 377, 1, 23 }, { 378, 1, 21 }, { 379, 1, 21 },
  { 380, 1, 21 }, { 381, 1, 23 }, { 382, 1, 28 },
  { 383, 1, 28 }, { 384, 6, 19 },
};
const uint16_t PART3_LEN = sizeof(PART3) / sizeof(PART3[0]);

const Event PART4[] PROGMEM = {};
const uint16_t PART4_LEN = 0;
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
};
const uint8_t NUM_SONGS = sizeof(SONGS) / sizeof(SONGS[0]);

typedef struct {
  byte uid[4];
  int8_t songIndex;
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