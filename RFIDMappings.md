const RFIDMapping RFID_MAPPINGS[] = {
  // Add your RFID mappings here
  // Format: { { 0xXX, 0xXX, 0xXX, 0xXX }, songIndex, "Song Name", "Mood" },
  { { 0xE2, 0x83, 0x50, 0x20 }, 0, "Olymic Fanfare", "Euphoric" },
  { { 0x86, 0x06, 0xCA, 0x30 }, 1, "Morning Mood", "Calm" },
  { { 0x86, 0x8E, 0x5A, 0x30 }, 2, "William Tell Overture", "Energized" },
  { { 0x09, 0x63, 0x15, 0xC1 }, 3, "Home on the Range", "Melancholic" },
  { { 0x1B, 0x79, 0xF3, 0x00 }, 4, "crossing field", "Intense" },
};