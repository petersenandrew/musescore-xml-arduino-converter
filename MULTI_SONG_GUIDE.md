# Multi-Song & RFID Quick Start Guide

## Adding Multiple Songs

### Initial Song
Start by converting your first song:
```bash
node converter.js bob.musicxml arduino.ino
```

### Adding More Songs
Use the `--append` flag to add additional songs without removing the first:
```bash
node converter.js scotlandHIGH.musicxml --append arduino.ino
node converter.js scotlandLOW.musicxml --append arduino.ino
```

Now your Arduino has 3 songs stored!

## Playing Specific Songs

### Edit arduino.ino
Change the `SONG_TO_PLAY` constant:

```cpp
const uint8_t SONG_TO_PLAY = 0;   // Play first song (bob.musicxml)
const uint8_t SONG_TO_PLAY = 1;   // Play second song (scotlandHIGH.musicxml)
const uint8_t SONG_TO_PLAY = 2;   // Play third song (scotlandLOW.musicxml)
```

### Play All Songs
```cpp
const uint8_t SONG_TO_PLAY = 254; // Plays all songs in sequence with 1-second pauses
```

## RFID Card Setup

### 1. Hardware Setup

**Components needed:**
- MFRC522 RFID module
- RFID cards or key fobs
- Jumper wires

**Connections:**
```
MFRC522 → Arduino Uno
-----------------------
SDA     → Pin 10
SCK     → Pin 13  
MOSI    → Pin 11
MISO    → Pin 12
RST     → Pin 9
GND     → GND
3.3V    → 3.3V
```

### 2. Install MFRC522 Library

In Arduino IDE:
1. Go to **Sketch → Include Library → Manage Libraries**
2. Search for "MFRC522"
3. Install "MFRC522 by GithubCommunity"

### 3. Find Your RFID Card UIDs

Edit arduino.ino and set:
```cpp
const uint8_t SONG_TO_PLAY = 255; // Enable RFID mode
```

Upload the sketch and open **Serial Monitor** (9600 baud).

Scan each of your RFID cards near the reader. You'll see output like:
```
RFID Music Player Ready. Scan a card...
Card UID: 0xDE 0xAD 0xBE 0xEF
Unknown card - no song mapped
```

Write down each card's UID!

### 4. Map Cards to Songs

Edit the `RFID_MAPPINGS` array in arduino.ino:

```cpp
const RFIDMapping RFID_MAPPINGS[] = {
  {{0xDE, 0xAD, 0xBE, 0xEF}, 0},  // Blue card → Song 0 (bob.musicxml)
  {{0x12, 0x34, 0x56, 0x78}, 1},  // Red card → Song 1 (scotlandHIGH.musicxml)
  {{0xAB, 0xCD, 0xEF, 0x01}, 2},  // Green card → Song 2 (scotlandLOW.musicxml)
};
```

Replace the example UIDs with your actual card UIDs from step 3.

### 5. Upload and Test

1. Upload the modified sketch
2. Open Serial Monitor to see debug output
3. Scan a card - it should play the corresponding song!
4. After the song finishes, scan another card to play a different song

## Example Workflow

```bash
# Convert 3 different songs
node converter.js happy_birthday.musicxml arduino.ino
node converter.js jingle_bells.musicxml --append arduino.ino
node converter.js mary_had_a_lamb.musicxml --append arduino.ino

# Upload to Arduino
# Scan your cards to find UIDs
# Update RFID_MAPPINGS with your card UIDs
# Upload again - now your cards control the music!
```

## Troubleshooting

**"Unknown card - no song mapped"**
- Double-check the UID in serial monitor matches the UID in your code exactly
- Make sure you copied all 4 bytes correctly (0xDE, 0xAD, 0xBE, 0xEF)
- UIDs are case-insensitive in the serial monitor but must use correct hex in code

**Card not detected at all**
- Check wiring connections
- Ensure MFRC522 library is installed
- Verify 3.3V power (NOT 5V) to the RFID module
- Try a different RFID card/tag

**Songs playing incorrectly**
- Verify `NUM_SONGS` matches the number of songs you've added
- Check that song indices in RFID_MAPPINGS are valid (0 to NUM_SONGS-1)
- Ensure each song was properly appended (check for PARTx arrays in code)

## Tips

- **Label your cards!** Use stickers or markers to identify which song each card plays
- **Different card types:** The MFRC522 works with MIFARE cards, tags, and key fobs (13.56 MHz)
- **Range:** Typical read range is 2-5cm, depending on card type and antenna size
- **Multiple readers:** You can use multiple MFRC522 modules with different SS pins for advanced projects

## Adding More Songs Later

Just use `--append` again:
```bash
node converter.js new_song.musicxml --append arduino.ino
```

Then add a new entry to `RFID_MAPPINGS`:
```cpp
{{0x99, 0x88, 0x77, 0x66}, 3},  // Purple card → Song 3 (new_song.musicxml)
```

Enjoy your RFID-controlled music player! 🎵
