# Windowsill-watering

Automatic windowsill irrigation controller for three pump zones, built on an Arduino-compatible LGT8F328P board.

## Overview

This project monitors soil moisture and water tank level for three separate zones and automatically controls pumps to keep plants hydrated. It also includes leak detection and emergency stop functionality.

## Features

- 3 pump zones with individual moisture sensing
- Capacitive moisture sensors for each zone
- Resistive empty-tank detection per pump
- Leak detection via resistive alarm sensors
- Rotary encoder + push-button configuration menu
- I2C LCD display for status and setup
- EEPROM-backed configuration storage
- Emergency stop button and alarm mode

## Hardware

- Board: `LGT8F328P` (Arduino-compatible)
- Sensors:
  - Capacitive moisture sensors on analog inputs
  - Resistive empty-tank sensors on digital pins
  - Leak alarm sensors on digital pins
- Output:
  - Pump relays
  - Alarm LED

## Pin assignments

| Function | Pins |
|---|---|
| Moisture sensors | `A3`, `A6`, `A7` |
| Pump relays | `A0`, `A1`, `A2` |
| Leak alarm sensors | `7`, `8`, `9` |
| Zone control buttons | `4`, `5`, `6` |
| Rotary encoder A/B | `10`, `11` |
| Encoder button | `12` |
| Emergency STOP button | `2` |
| External leak interrupt | `3` |
| Alarm LED | `13` |
| I2C LCD | `A4` (SDA), `A5` (SCL) |

## Software

- PlatformIO project configuration: `platformio.ini`
- Framework: `arduino`
- Library dependencies:
  - `mathertel/OneButton`
  - `mathertel/RotaryEncoder`
  - `marcoschwartz/LiquidCrystal_I2C`

## Build & Upload

1. Open the project in PlatformIO.
2. Select the `LGT8F328P` environment.
3. Build and upload to the board.

## Operation

- On startup, the system loads pump settings from EEPROM or initializes defaults.
- The LCD shows moisture values and pump states.
- Press the encoder button to enter setup mode.
- Rotate the encoder to change values.
- Double-click to toggle setup mode and save settings.
- Long-press the encoder button to select the next pump zone.
- External interrupts trigger emergency stop or leak alarm.

## Notes

- Default pump settings are stored in EEPROM with a signature validation.
- Zone configuration includes minimum/maximum moisture, pump run time, and pause time.
- The current project source files are `src/main.cpp` and `src/_Pumper.h`.

## License

This project does not currently specify a license. Add a `LICENSE` file if you want to make reuse terms explicit.
