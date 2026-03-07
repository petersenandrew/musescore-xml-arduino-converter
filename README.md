# MusicXML to Arduino Buzzer Converter

Convert MusicXML scores into compact Arduino code to play music through one or two buzzers.

## What it does

This project parses MuseScore `.musicxml` files and generates Arduino-compatible C++ code that plays the music on your microcontroller using PWM on GPIO pins. All note data is stored in PROGMEM (flash memory) to minimize RAM usage, ideal for memory-constrained boards like the Arduino Uno.

**Features:**
- ✅ Multi-part support (e.g., Violin + Piano)
- ✅ Automatic time signature detection (4/4, 3/4, 6/8, etc.)
- ✅ Tempo extraction from MusicXML
- ✅ Compact event-based storage (absolute time + duration + pitch)
- ✅ PROGMEM-optimized for flash storage, minimal SRAM usage
- ✅ In-place code generation (update `.ino` with one command)

## Prerequisites

- **Node.js** (v12+)
- **Arduino IDE** or compatible environment
- **Arduino Uno** or compatible board with PWM pins
- **2× Buzzers/Speakers** (optional; connected to GPIO pins 5 & 6 by default)
- **MuseScore 3+** to create/edit `.musicxml` files

## Setup

1. Clone/download this repository:
   ```bash
   git clone https://github.com/tegze/musescore-xml-arduino-converter.git
   cd musescore-xml-arduino-converter
   ```

2. No npm dependencies needed—the converter runs with only Node.js built-ins.

## Usage

### Step 1: Export MusicXML from MuseScore

1. Open your score in MuseScore
2. **File → Export** (or Save As)
3. Choose format: **MusicXML (.musicxml)**
4. Save to this repository folder (e.g., `mysong.musicxml`)

### Step 2: Generate Arduino Code

Convert the MusicXML and update `arduino.ino` in one command:

```bash
node converter.js mysong.musicxml arduino.ino
```

**With note articulation** (adds silence between consecutive identical notes):

```bash
node converter.js mysong.musicxml --articulation 2 arduino.ino
```

The `--articulation` flag (1-10 ticks) shortens each note by that amount when followed by the same pitch, creating natural separation. Default is 0 (no gap). Example values:
- `--articulation 1` – Very subtle gap
- `--articulation 2` – Natural articulation (recommended)
- `--articulation 3-5` – Staccato/percussive

**Output:**
```
Inlined song data into arduino.ino (parts: P1, P2, tempo 120, divisions 4, articulation 2 ticks).
```

The converter extracts:
- Tempo (BPM)
- Time signature (beats/beat-type)
- Note events (time, duration, frequency)
- All part names and data

### Step 3: Upload to Arduino

1. Open `arduino.ino` in the Arduino IDE
2. Select your board: **Tools → Board → Arduino/Genuino Uno**
3. Select your COM port: **Tools → Port → COM[X]**
4. Click **Upload** (or Ctrl+U)

The sketch will automatically run on startup and play the song once.

## Multi-Song Support & RFID Control

### Adding Multiple Songs

You can now store multiple songs in a single Arduino sketch and select which one to play:

**Replace mode (default):**
```bash
node converter.js song1.musicxml arduino.ino
```

**Append mode (add additional songs):**
```bash
node converter.js song2.musicxml --append arduino.ino
node converter.js song3.musicxml --append arduino.ino
```

Each appended song gets added to the `SONGS` array with its own `PARTx` data.

### Playback Modes

Edit the `SONG_TO_PLAY` constant in `arduino.ino`:

```cpp
const uint8_t SONG_TO_PLAY = 0;   // Play specific song (0 = first, 1 = second, etc.)
const uint8_t SONG_TO_PLAY = 254; // Play all songs sequentially
const uint8_t SONG_TO_PLAY = 255; // RFID mode (scan card to select song)
```

### RFID Scanner Integration

The system supports RFID-based song selection using the MFRC522 module.

**Hardware Requirements:**
- MFRC522 RFID Reader Module
- RFID cards/tags
- Arduino Uno or compatible

**Wiring:**
```
MFRC522 Module → Arduino
SDA    → Pin 10
SCK    → Pin 13
MOSI   → Pin 11
MISO   → Pin 12
IRQ    → (not connected)
GND    → GND
RST    → Pin 9
3.3V   → 3.3V
```

**Required Library:**
Install the MFRC522 library in Arduino IDE:
1. **Sketch → Include Library → Manage Libraries**
2. Search for "MFRC522"
3. Install "MFRC522 by GithubCommunity"

**Configuring RFID Card Mappings:**

1. Set `SONG_TO_PLAY = 255` to enable RFID mode
2. Upload the sketch and open Serial Monitor (9600 baud)
3. Scan your RFID cards to see their UIDs:
   ```
   Card UID: 0xDE 0xAD 0xBE 0xEF
   Unknown card - no song mapped
   ```
4. Edit the `RFID_MAPPINGS` array in `arduino.ino`:
   ```cpp
   const RFIDMapping RFID_MAPPINGS[] = {
     {{0xDE, 0xAD, 0xBE, 0xEF}, 0, "Song 0 Title", "Mood 0"},
     {{0xCA, 0xFE, 0xBA, 0xBE}, 1, "Song 1 Title", "Mood 1"},
     {{0x12, 0x34, 0x56, 0x78}, 2, "Song 2 Title", "Mood 2"},
   };
   ```
5. Upload again and scan cards to play songs!

**Usage:**
- Set `SONG_TO_PLAY = 255` in arduino.ino
- Upload the sketch
- Scan an RFID card near the reader
- The corresponding song will play
- After playback, scan another card to play a different song

## Hardware Wiring

### Single Buzzer (Pin 5)
```
Arduino Pin 5 → Buzzer+ 
Arduino GND   → Buzzer-
```

### Two Buzzers (Pins 5 & 6)
```
Arduino Pin 5 → Buzzer 1+ (e.g., Violin)
Arduino Pin 6 → Buzzer 2+ (e.g., Piano)
Arduino GND   → Buzzer 1- & Buzzer 2- (common)
```

**Note:** Use current-limiting resistors (220-330Ω) if buzzers draw >40mA.

## Configuration

Edit `arduino.ino` to change pin assignments:

```cpp
const int BZ1 = 5; // Pin for part 1 (Violin)
const int BZ2 = 6; // Pin for part 2 (Piano)

// RFID pins (if using MFRC522)
const int RST_PIN = 9;
const int SS_PIN = 10;
```

Song selection mode:
```cpp
const uint8_t SONG_TO_PLAY = 0;   // Play song at index 0
const uint8_t SONG_TO_PLAY = 254; // Play all songs sequentially
const uint8_t SONG_TO_PLAY = 255; // RFID mode
```

## Memory Optimization

On Arduino Uno (2KB SRAM, 32KB Flash):

- **All note data stored in PROGMEM** → Uses flash, not SRAM
- **Compact struct:** 5 bytes per event (start time, duration, pitch index)
- **Shared frequency table** → Only unique pitches stored once

If you still get memory warnings:
1. Trim unnecessary parts from the MusicXML before exporting
2. Use a board with more SRAM (Arduino Mega, ESP32)
3. Contact us for delta-encoding optimization

## Converter Command Reference

### Generate header file (separate file)
```bash
node converter.js input.musicxml output.h
```

### Update Arduino sketch in-place (replace existing song)
```bash
node converter.js input.musicxml arduino.ino
```
or
```bash
node converter.js input.musicxml --inline arduino.ino
```

### Append new song to existing songs
```bash
node converter.js input.musicxml --append arduino.ino
```
This adds the new song to the SONGS array without removing existing songs.

### Add note articulation (silence between identical consecutive notes)
```bash
node converter.js input.musicxml -a 2 arduino.ino
```
or
```bash
node converter.js input.musicxml --articulation 3 arduino.ino
```

### Combine flags
```bash
node converter.js newsong.musicxml --append --articulation 2 arduino.ino
```

### Clear all songs (start fresh)
```bash
node clear_songs.js arduino.ino
```
This removes all songs but preserves the structure, allowing you to add fresh songs. Creates a backup file automatically.

**Articulation values:** 1-10 ticks (default 0 = no gap)
- Applies only when the same note plays twice in a row
- Each tick = 1 division unit; adjust based on your tempo
- Example: at 180 BPM with 4 divisions/quarter, 2 ticks ≈ 67ms

### Defaults
- Input: `bob.musicxml`
- Output: `song_data.h` (unless `.ino` specified)
- Articulation: `0` (disabled)

## Example Songs

The repository includes test songs:
- `bob.musicxml` – 3/4 waltz for testing (2 parts)
- `scotland.musicxml` – Additional test file

Try them:
```bash
node converter.js scotland.musicxml arduino.ino
```

## Troubleshooting

### Compiler error: "data section exceeds available space"
- You're using too much SRAM. All data should be `PROGMEM`.
- Check: Are you storing large arrays in RAM instead of flash?
- Solution: Regenerate with the latest converter (inlines all to PROGMEM).

### No sound from buzzers
1. Check connections and pin assignments
2. Verify buzzer polarity (+ to GPIO, - to GND)
3. Test with a simple tone: `tone(5, 440, 1000);` in Arduino IDE
4. Confirm tempo isn't 0 (should be ~120-180 BPM)

### "Missing SONG_DATA_BEGIN/END markers"
- The `arduino.ino` file must have the markers in place.
- Re-download a fresh copy or manually add:
  ```cpp
  // SONG_DATA_BEGIN
  // ... song data ...
  // SONG_DATA_END
  ```

### Wrong tempo or time signature in output
- Check the MuseScore score: **Format → Style → Timings**
- Ensure the `<sound tempo="...">` tag exists in MusicXML
- Fallback: converter defaults to 120 BPM, 4/4 if missing

### Notes run together / no articulation between repeated pitches
- Use the `--articulation` flag to add gaps:
  ```bash
  node converter.js song.musicxml --articulation 2 arduino.ino
  ```
- Increase the value for more pronounced separation (3-5 for staccato effect)
- Adjust based on tempo and personal preference

## Technical Details

### Event Structure
```cpp
struct Event {
  uint16_t t;  // Absolute start time in ticks
  uint16_t d;  // Duration in ticks
  uint8_t n;   // Index into NOTE_FREQS array
};
```

### Timing Calculation
- 1 tick = 1 quarter note division
- Playback time = `tick_count * TICKS_PER_QUARTER / (TEMPO_BPM / 60) / 1_000_000` µs
- Frequency PWM uses `500000 / frequency_hz` µs for half-period

### Why PROGMEM?
AVR microcontrollers have:
- **Flash:** 32KB (stores all const data)
- **SRAM:** 2KB (stores variables during runtime)

Storing large tables in SRAM quickly exhausts memory. Using `PROGMEM` keeps them in flash and reads them on-demand, leaving SRAM for playback state.

## Limitations

- **Maximum polyphony:** 1 note per buzzer (use highest pitch if chord)
- **Pitch range:** Limited by hardware PWM frequency (~16 kHz max)
- **Resolution:** Based on `divisions` value in MusicXML (typically 4 = 1/16 note)

## Future Improvements

- [x] Multi-song support with append mode
- [x] RFID-based song selection
- [ ] MIDI input support
- [ ] Delta-time encoding for further compression
- [ ] Polyphony mixing (software PWM)
- [ ] Web UI for converter
- [ ] Support for other microcontrollers (STM32, ESP32)
- [ ] Song metadata (titles, artist names)

## License

MIT – Feel free to use and modify.

## Contributing

Found a bug or want to add features? Issues and PRs welcome!

---

**Questions?** Check the example songs or open an issue on GitHub.
