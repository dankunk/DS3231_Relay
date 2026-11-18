# 1. Arduino Setup

[← Guides](README.md)

---

## Install the Arduino IDE and RTClib

1. Install **Arduino IDE 2** from <https://www.arduino.cc/en/software>.
2. Open **Tools → Manage Libraries…**.
3. Search for **`RTClib`** and install **RTClib by Adafruit**. When the IDE offers to install the dependencies (Adafruit BusIO), choose **Install All**.

## Configure the sketch

Open [`Arduino_sketches/ds3231_trigger_relay/ds3231_trigger_relay.ino`](../Arduino_sketches/ds3231_trigger_relay/ds3231_trigger_relay.ino). All user settings are at the top of the file.

### Time zone

```cpp
const int8_t STD_OFFSET_HOURS = -7;   // standard-time offset from UTC (US Mountain = -7)
const bool   OBSERVE_DST      = true; // apply US daylight saving rules
```

The defaults are for US Mountain Time (MST/MDT). Set these once for your location. **Don't change them when daylight saving starts or ends**, because the sketch switches between standard and daylight time by itself. For other time zones, see [Daylight saving time](04_daylight_saving_time.md#changing-the-time-zone-or-disabling-dst).

### Schedule

```cpp
Event events[] = {
  // { hour, minute, second, GPIO pin, state }
  {  0,  0,  0,  8, LOW  },   // 00:00:00 → pin 8 LOW  (lights off)
  {  8,  0,  0,  8, HIGH },   // 08:00:00 → pin 8 HIGH (lights on)
};
```

- Times are **local wall-clock time**, the same time a phone shows in summer and in winter.
- `HIGH` turns the relay on (lights on) and `LOW` turns it off (lights off).
- Events can be listed in any order, and you can add as many as you need.
- **Schedules that run past midnight work.** For example, `{22,0,0,8,HIGH}` and `{6,0,0,8,LOW}` keep the lights on from 22:00 to 06:00.
- The sketch always applies **the most recent event**. After a power cut or a reset, the relay goes straight back to the correct state.

### Fail-safe state

```cpp
const uint8_t FAILSAFE_STATE = LOW;   // relay state when the clock can't be trusted
```

The sketch doesn't guess the time when any of these is true:

- the clock has never been set
- the DS3231 isn't responding
- the DS3231 reports that it lost power

Instead, it holds the relay in `FAILSAFE_STATE` and fast-blinks the **L** LED until the clock is synced. Set it to `HIGH` if the lights should stay on when the clock is unavailable.

## Upload

1. Connect the Arduino to the computer with a USB cable.
2. Select **Tools → Board → Arduino AVR Boards → Arduino Uno**, or your board.
3. Select the Arduino's port under **Tools → Port** (for example, `COM3`).
4. Click **Upload** and wait for *Done uploading*.

Uploading doesn't reset the clock, which is kept in the DS3231. The sync and battery dates are also kept, in the Arduino's EEPROM. You can change the schedule and upload again at any time.

> A new board **fast-blinks** after the first upload because the clock hasn't been set yet. This is expected. Finish the [wiring](02_wiring.md), then run the [clock sync](03_sync_clock_from_laptop.md).

## Serial commands

Open **Tools → Serial Monitor**, then set the baud rate to **57600** and the line ending to **New Line**.

| Command | Description |
|---------|-------------|
| `STATUS` | Shows UTC and local time, DST state, the lost-power flag, the last sync date and the battery install date |
| `HELP` | Lists the available commands |

The sketch also prints a status line once a minute, and a line each time the relay switches:

```
UTC 2026-11-16 18:45:00 | local 2026-11-16 11:45:00 (UTC-7, standard)
  -> pin 8 HIGH (relay ON)
```

> Close the Serial Monitor before you run the sync script. Only one program can use the serial port at a time.

---

**Next:** [2. Wiring →](02_wiring.md)
