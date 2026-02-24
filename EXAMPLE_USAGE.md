# Example Usage - RFID Music Box

## Complete Example: 3-Song RFID Music Player

### Step 1: Prepare Your Songs

Make sure you have 3 MusicXML files ready (or use the included examples):
- `bob.musicxml`
- `scotlandHIGH.musicxml`  
- `scotlandLOW.musicxml`

### Step 2: Generate Multi-Song Arduino Code

```bash
# First song (replaces any existing song data)
node converter.js bob.musicxml arduino.ino

# Add second song
node converter.js scotlandHIGH.musicxml --append arduino.ino

# Add third song
node converter.js scotlandLOW.musicxml --append arduino.ino
```

You should see output like:
```
Inlined song data into arduino.ino (parts: P1, P2, tempo 170, divisions 2).
Appended Song 2 to arduino.ino (parts: P1, P2, tempo 120, divisions 4).
Appended Song 3 to arduino.ino (parts: P1, P2, tempo 120, divisions 4).
```

### Step 3: Test Without RFID First

Edit arduino.ino and set:
```cpp
const uint8_t SONG_TO_PLAY = 0; // Test song 0
```

Upload to your Arduino and verify the first song plays correctly.

Try changing to `1` or `2` to test the other songs.

### Step 4: Wire Up RFID Reader

Connect MFRC522 module:
```
MFRC522 SDA   → Arduino Pin 10
MFRC522 SCK   → Arduino Pin 13
MFRC522 MOSI  → Arduino Pin 11
MFRC522 MISO  → Arduino Pin 12
MFRC522 RST   → Arduino Pin 9
MFRC522 GND   → Arduino GND
MFRC522 3.3V  → Arduino 3.3V
```

Connect buzzers:
```
Buzzer 1 (+)  → Arduino Pin 5
Buzzer 1 (-)  → Arduino GND
Buzzer 2 (+)  → Arduino Pin 6  
Buzzer 2 (-)  → Arduino GND
```

### Step 5: Install MFRC522 Library

In Arduino IDE:
1. **Sketch → Include Library → Manage Libraries**
2. Search for "MFRC522"
3. Install "MFRC522 by GithubCommunity"

### Step 6: Scan Your Cards to Get UIDs

Edit arduino.ino and set:
```cpp
const uint8_t SONG_TO_PLAY = 255; // RFID mode
```

Upload and open Serial Monitor (9600 baud).

Scan each of your 3 RFID cards. Note their UIDs:
```
RFID Music Player Ready. Scan a card...
Card UID: 0x12 0x34 0x56 0x78
Unknown card - no song mapped
Card UID: 0xAB 0xCD 0xEF 0x01
Unknown card - no song mapped
Card UID: 0xDE 0xAD 0xBE 0xEF
Unknown card - no song mapped
```

### Step 7: Map Cards to Songs

Edit the RFID_MAPPINGS array in arduino.ino:

```cpp
const RFIDMapping RFID_MAPPINGS[] = {
  {{0x12, 0x34, 0x56, 0x78}, 0},  // Card 1 → bob.musicxml
  {{0xAB, 0xCD, 0xEF, 0x01}, 1},  // Card 2 → scotlandHIGH.musicxml
  {{0xDE, 0xAD, 0xBE, 0xEF}, 2},  // Card 3 → scotlandLOW.musicxml
};
```

### Step 8: Final Upload

Upload the sketch one more time with your card UIDs configured.

### Step 9: Enjoy!

- Scan card 1 → plays bob.musicxml
- Scan card 2 → plays scotlandHIGH.musicxml
- Scan card 3 → plays scotlandLOW.musicxml

## Adding More Songs Later

Want to add a 4th song? Easy!

```bash
node converter.js mysong.musicxml --append arduino.ino
```

Then add to RFID_MAPPINGS:
```cpp
{{0x99, 0x88, 0x77, 0x66}, 3},  // Card 4 → mysong.musicxml
```

## Sequential Play Mode

Want the Arduino to play all songs in order automatically?

```cpp
const uint8_t SONG_TO_PLAY = 254; // Play all with 1-second pauses
```

## Troubleshooting Examples

**Problem:** "Card UID: 0x12 0x34 0x56 0x78 → Unknown card"

**Solution:** Your UID isn't in RFID_MAPPINGS. Add it:
```cpp
{{0x12, 0x34, 0x56, 0x78}, 0},
```

---

**Problem:** Buzzer not making sound

**Solution:** 
1. Check wiring
2. Test with simple tone first: `tone(5, 440, 1000);`
3. Verify buzzer polarity (+ to pin, - to GND)

---

**Problem:** "Playing song 2" but wrong song plays

**Solution:** Songs are 0-indexed. Song 0 is first, Song 1 is second, etc.
Check your RFID_MAPPINGS indices.

---

**Problem:** Ran out of memory

**Solution:** Arduino Uno has limited space. Try:
- Shorter songs
- Fewer songs
- Use Arduino Mega (more memory)

## Advanced: Custom Pin Configuration

Change buzzer pins:
```cpp
const int BZ1 = 5;  // Change to any PWM pin
const int BZ2 = 6;  // Change to any PWM pin
```

Change RFID pins:
```cpp
const int RST_PIN = 9;   // Can use any digital pin
const int SS_PIN = 10;   // Must be SPI SS pin (10 on Uno)
```

## Complete Command Reference

```bash
# Replace mode (start fresh with one song)
node converter.js song.musicxml arduino.ino

# Append mode (add to existing songs)
node converter.js song.musicxml --append arduino.ino

# With articulation (add gaps between repeated notes)
node converter.js song.musicxml --articulation 2 --append arduino.ino

# Short form
node converter.js song.musicxml -a 2 --append arduino.ino

# Clear all songs and start fresh (creates backup)
node clear_songs.js arduino.ino
```

## Starting Over

If you want to remove all songs and start fresh:

```bash
# Clear all songs (keeps structure intact)
node clear_songs.js arduino.ino

# This creates arduino.ino.backup automatically
# Now add your new songs
node converter.js newsong1.musicxml arduino.ino
node converter.js newsong2.musicxml --append arduino.ino
```

Happy music making! 🎵
