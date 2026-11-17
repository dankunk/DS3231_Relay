# DS3231 relay timer: precise grow-light control without Wi-Fi

An Arduino switches a Digital Loggers IoT Relay on a daily schedule, using a DS3231 real-time clock (RTC) wired to it. To set the clock, plug a laptop into the Arduino's USB port and run one script. No Wi-Fi is needed on campus, and nothing has to be unplugged or carried home.

- **Daylight saving is automatic.** The RTC holds UTC and the Arduino converts it to Mountain Time (MST/MDT). No March/November visits.
- **The sync is accurate to a few milliseconds.** The script uses internet time and measures and cancels out the USB delay.
- **The coin cell is tracked.** Each sync reports whether the RTC lost power, how fast it's drifting, and how old the battery is.

![How the system works](wiring_diagrams/images/system_overview.svg)

## Full guides with wiring diagrams

**→ [wiring_diagrams/](wiring_diagrams/README.md)**: start here for a first build.

1. [Set up and flash the Arduino](wiring_diagrams/01_arduino_setup.md)
2. [Wiring: Arduino, DS3231 and IoT Relay](wiring_diagrams/02_wiring.md)
3. [Sync the clock from a laptop](wiring_diagrams/03_sync_clock_from_laptop.md)
4. [Daylight saving time: how it works](wiring_diagrams/04_daylight_saving_time.md)
5. [RTC battery & yearly maintenance](wiring_diagrams/05_battery_and_maintenance.md)
6. [Troubleshooting](wiring_diagrams/06_troubleshooting.md)

## What's in this repo

| Path | What it is |
|------|-----------|
| [`Arduino_sketches/ds3231_trigger_relay/`](Arduino_sketches/ds3231_trigger_relay) | The Arduino sketch: light schedule in local time, automatic DST, fail-safe, and USB clock-sync commands |
| [`laptop_sync/sync_rtc.py`](laptop_sync/sync_rtc.py) | Laptop script that sets the clock over USB and reports RTC/battery health |
| [`laptop_sync/sync_rtc.bat`](laptop_sync/sync_rtc.bat) | Windows double-click launcher for the script |
| [`wiring_diagrams/`](wiring_diagrams/README.md) | Step-by-step guides, wiring diagrams, photos |

## Quick start (if you've done this before)

### 1. Arduino

Edit the schedule at the top of `ds3231_trigger_relay.ino` (times are normal **local wall-clock** time) and upload it. It needs the **RTClib by Adafruit** library.

```cpp
Event events[] = {
  // { hour, minute, second, GPIO pin, state }
  {  0,  0,  0,  8, LOW  },   // at 00:00:00 → pin 8 LOW  (lights off)
  {  8,  0,  0,  8, HIGH },   // at 08:00:00 → pin 8 HIGH (lights on)
};
```

`HIGH` = relay on, `LOW` = relay off. Events can be in any order and overnight schedules work.

### 2. Wiring

**DS3231 VCC→5V (3.3V for ZS-042 modules), GND→GND, SDA→A4, SCL→A5. Pin 8→relay +, GND→relay −.** Plug the lights into a **Normally OFF** outlet. [Diagram](wiring_diagrams/02_wiring.md).

### 3. Set the clock (first install, then about once a year)

Close the Arduino Serial Monitor, plug in a laptop, and run:

```bash
py -m pip install pyserial
```

```bash
py laptop_sync/sync_rtc.py
```

On Windows you can just double-click `laptop_sync/sync_rtc.bat`. Add `--battery-replaced` after swapping the coin cell, or use `--check` to only report.

### 4. Check

The Arduino's **L** LED gives a short blink every 2 s when running normally. **Fast blinking** means the clock isn't set or the RTC lost power: run the sync script.

### Time-zone settings

Already set for Colorado, at the top of the sketch:

```cpp
const int8_t STD_OFFSET_HOURS = -7;   // MST
const bool   OBSERVE_DST      = true; // switches to MDT (-6) automatically
```

## Upgrading from the old Pico W version

The Pico W is no longer needed; its code is in this repo's git history. An RTC set by the old Pico script holds local time, so wire it to the Arduino, upload the new sketch, and run `sync_rtc.py` once. Until you do, the lights will switch 6–7 hours off. [Details](wiring_diagrams/04_daylight_saving_time.md#migrating-from-the-old-pico-w--local-time-setup-one-time-only).

## Battery: the short version

- The CR2032 only runs the clock during power cuts.
- `sync_rtc.py` tells you when to replace it: if the RTC lost power since the last sync, or the cell is ~3 years old.
- Blue **ZS-042** modules have a charging circuit that can damage a CR2032 at 5 V. [Read the warning](wiring_diagrams/05_battery_and_maintenance.md#zs-042-charging-circuit-warning).

### 
NOTE: This codebase was refactored using Claude Opus 5.5 (high). 