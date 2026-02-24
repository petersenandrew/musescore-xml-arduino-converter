"use strict";

const fs = require("fs");

const args = process.argv.slice(2);
const targetFile = args[0] || "arduino.ino";

function clearSongs(inoPath) {
	const content = fs.readFileSync(inoPath, "utf8");
	const begin = "// SONG_DATA_BEGIN";
	const end = "// SONG_DATA_END";
	
	const startIdx = content.indexOf(begin);
	const endIdx = content.indexOf(end);
	if (startIdx === -1 || endIdx === -1) {
		throw new Error(`Missing ${begin}/${end} markers in ${inoPath}`);
	}

	// Extract existing RFID_MAPPINGS if present
	const rfidMappingsMatch = content.match(/const RFIDMapping RFID_MAPPINGS\[\]\s*=\s*\{([\s\S]*?)\};/);
	const existingRFIDMappings = rfidMappingsMatch ? rfidMappingsMatch[1] : `  {{0x63, 0x4A, 0xE9, 0xFB}, 0},  // RFID Tag 1 → Song 0
  {{0x1B, 0x79, 0xF3, 0x00}, 1},  // RFID Card 1 → Song 1
  {{0xDE, 0xAD, 0xBE, 0xEF}, 2},  // Example RFID → Song 2
  // Add more UID mappings here as you add songs`;

	// Build clean song data section
	const cleanDataSection = `
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
  uint8_t songIndex;
} RFIDMapping;

// Map your RFID cards to songs - RFID 1 plays Song 1, RFID 2 plays Song 2, etc.
const RFIDMapping RFID_MAPPINGS[] = {
${existingRFIDMappings}
};
const uint8_t NUM_RFID_MAPPINGS = sizeof(RFID_MAPPINGS) / sizeof(RFID_MAPPINGS[0]);

// Playback configuration
const uint8_t SONG_TO_PLAY = 255; // 255 = RFID mode, 0-253 = play specific song once, 254 = play all sequentially
`;

	// Reconstruct file
	const before = content.substring(0, startIdx + begin.length);
	const after = content.substring(endIdx);
	const updated = before + cleanDataSection + after;

	// Create backup
	const backupPath = inoPath + ".backup";
	fs.writeFileSync(backupPath, content, "utf8");
	console.log(`Backup created: ${backupPath}`);

	// Write cleaned file
	fs.writeFileSync(inoPath, updated, "utf8");
	console.log(`\nCleared all songs from ${inoPath}`);
	console.log(`Songs removed. Structure preserved for adding new songs.`);
	console.log(`\nTo add songs, use:`);
	console.log(`  node converter.js song1.musicxml arduino.ino`);
	console.log(`  node converter.js song2.musicxml --append arduino.ino`);
	console.log(`  node converter.js song3.musicxml --append arduino.ino`);
}

try {
	clearSongs(targetFile);
} catch (error) {
	console.error("Error:", error.message);
	process.exit(1);
}
