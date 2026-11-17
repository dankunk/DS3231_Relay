# Wiring & setup guides

Step-by-step guides for building, syncing and maintaining the DS3231 relay timer.

![How the system works](images/system_overview.svg)

**In short:** the DS3231 clock is wired permanently to the Arduino on campus. About once a year you plug in a laptop and run `sync_rtc.py`. It sets the clock to **UTC** from internet time, compensating for the USB delay, and reports battery health. The Arduino converts UTC to Mountain Time and handles daylight saving itself, so there are no March/November visits.

## Guides (do them in order the first time)

| # | Guide | What it covers |
|---|-------|----------------|
| 1 | [Set up and flash the Arduino](01_arduino_setup.md) | Arduino IDE + RTClib, editing the schedule, uploading, serial commands |
| 2 | [Wiring](02_wiring.md) | DS3231 + IoT Relay pin by pin, which outlet, powering the Arduino |
| 3 | [Sync the clock from a laptop](03_sync_clock_from_laptop.md) | Python setup, running `sync_rtc.py`, reading the report, how the USB delay is cancelled |
| 4 | [Daylight saving time: how it works](04_daylight_saving_time.md) | Why the RTC holds UTC, the DST rules, changing time zones, migrating from the old Pico W method |
| 5 | [RTC battery & yearly maintenance](05_battery_and_maintenance.md) | When to replace the CR2032, the ZS-042 charging-circuit warning, yearly checklist |
| 6 | [Troubleshooting](06_troubleshooting.md) | LED codes, script errors, lights at the wrong time |

## Diagrams

| Diagram | File |
|---------|------|
| System overview | [images/system_overview.svg](images/system_overview.svg) |
| Arduino Uno → DS3231 + IoT Relay | [images/arduino_ds3231_relay.svg](images/arduino_ds3231_relay.svg) |

Photos of the real build go in [photos/](photos/README.md). That page lists which shots to take.

## Parts list

| Part | Notes |
|------|-------|
| Arduino Uno (or Nano, Mega, Uno R4…) | Runs the schedule and accepts clock syncs over USB |
| DS3231 RTC module | Blue "ZS-042" boards or the Adafruit DS3231 breakout both work |
| CR2032 coin cell | Keeps the clock running through power cuts. Read [guide 5](05_battery_and_maintenance.md) first if you have a ZS-042 board. |
| Digital Loggers IoT Relay | Switches the lights |
| Jumper wires | 4 for the DS3231, 2 for the relay |
| Power for the Arduino | 9 V barrel-jack adapter (recommended: keeps USB free) or a 5 V USB adapter |
| USB-A to USB-B cable + any laptop with Python 3 | For syncing the clock |

## Quick reference: pin connections

| Arduino Uno pin | Connects to | Wire color in diagram |
|-----------------|-------------|---------------|
| 5V (or 3.3V for ZS-042 modules) | DS3231 VCC | red |
| GND | DS3231 GND | black |
| A4 (or the SDA pin by AREF) | DS3231 SDA | blue |
| A5 (or the SCL pin by AREF) | DS3231 SCL | yellow |
| Digital pin 8 | IoT Relay **+** | purple |
| GND | IoT Relay **−** | black |
| USB port | Laptop, when syncing | — |
