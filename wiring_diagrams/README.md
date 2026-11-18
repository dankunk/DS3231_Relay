# Build and Operation Guide

These guides cover how to build, configure, sync and maintain the DS3231 Relay Timer.

![How the system works](images/system_overview.svg)

## How it works

The DS3231 real-time clock is wired permanently to the Arduino and keeps time in **UTC**. The Arduino converts UTC to local time, applies daylight saving time, and switches the IoT Relay to match the schedule. About once a year, you connect a laptop over USB and run `sync_rtc.py`. The script sets the RTC from internet time, cancels out the USB delay, and reports the clock's drift and battery health.

## Guides

Work through the guides in order for a first build.

| # | Guide | Covers |
|---|-------|--------|
| 1 | [Arduino setup](01_arduino_setup.md) | Arduino IDE and RTClib, the schedule, uploading, serial commands |
| 2 | [Wiring](02_wiring.md) | DS3231 and IoT Relay connections, outlet assignment, powering the Arduino |
| 3 | [Clock sync](03_sync_clock_from_laptop.md) | Python setup, running `sync_rtc.py`, reading the report, how the USB delay is cancelled |
| 4 | [Daylight saving time](04_daylight_saving_time.md) | Why the RTC stores UTC, the DST rule, changing time zones |
| 5 | [Battery and maintenance](05_battery_and_maintenance.md) | When to replace the coin cell, other DS3231 modules, the yearly checklist |
| 6 | [Troubleshooting](06_troubleshooting.md) | LED codes, script errors, lights switching at the wrong time |

## Diagrams and photos

| Diagram | File |
|---------|------|
| System overview | [images/system_overview.svg](images/system_overview.svg) |
| Wiring: Arduino Uno, DS3231 and IoT Relay | [images/arduino_ds3231_relay.svg](images/arduino_ds3231_relay.svg) |

For photos of the reference build, see [photos/](photos/README.md).

## Bill of materials

For the full parts list with Digi-Key part numbers and prices, see [`BOM/`](../BOM/README.md).

## Pin reference

| Arduino Uno pin | Connects to | Wire color in diagram |
|-----------------|-------------|-----------------------|
| 5V | DS3231 VIN | red |
| GND | DS3231 GND | black |
| A4 (or the SDA pin next to AREF) | DS3231 SDA | blue |
| A5 (or the SCL pin next to AREF) | DS3231 SCL | yellow |
| Digital pin 8 | IoT Relay control **+** | purple |
| GND | IoT Relay control **−** | black |
| USB port | USB power adapter, or a laptop during a sync | gray |
