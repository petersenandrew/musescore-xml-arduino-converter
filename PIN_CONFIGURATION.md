# Arduino Uno Pin Configuration

This document defines the pin mapping for the Music Player RFID Jukebox running on Arduino Uno.

## Digital Pins

| Pin | Component | Function |
|-----|-----------|----------|
| 4 | Servo | Servo control signal |
| 5 | Buzzer 1 | Violin audio output |
| 6 | Buzzer 2 | Piano composite audio output |
| 9 | RFID | MFRC522 Reset |
| 10 | RFID | SPI Slave Select (SS) |
| 11 | RFID | SPI MOSI (Master Out Slave In) |
| 12 | RFID | SPI MISO (Master In Slave Out) |
| 13 | RFID | SPI SCK (Serial Clock) |

## Analog Pins

| Pin | Component | Function |
|-----|-----------|----------|
| A4 | LCD Display | I2C Data (SDA) |
| A5 | LCD Display | I2C Clock (SCL) |

## SPI Connection (RFID)

The RFID module (MFRC522) is connected via SPI using the hardware SPI pins:

| Arduino Pin | SPI Signal | RFID Module |
|-------------|-----------|-------------|
| 10 | SS / CS (Chip Select) | SS |
| 11 | MOSI / COPI (Controller Out, Peripheral In) | MOSI |
| 12 | MISO / CIPO (Controller In, Peripheral Out) | MISO |
| 13 | SCK (Serial Clock) | SCK |
| 9 | RST (Reset) | RST |

**Note:** These are the hardware SPI pins on Arduino Uno and must be used. MOSI/MISO are legacy names; COPI/CIPO are modern replacements.

## I2C Connection (LCD Display)

The LCD display (20x2, address 0x27) is connected via I2C:
- **SDA** → Analog 4 (A4)
- **SCL** → Analog 5 (A5)

## Audio Output

- **BZ1 (Violin)** → Digital 5: Produces the main melody
- **BZ2 (Piano Composite)** → Digital 6: Produces harmonic accompaniment

Both buzzers operate via PWM square-wave generation at frequencies corresponding to musical notes.

## Servo

- **Pin 4**: Controls the disc ejection mechanism with angles 90° (home) and 170° (eject)

## Power Considerations

- **Arduino Uno**: Powers at 5V
- **RFID Module (MFRC522)**: 3.3V only - use a level shifter or power regulator
- **LCD Display**: 5V logic compatible (check your specific module)
- **Servo**: 5V
- **Buzzers**: 5V with PWM
