# 1. Set up and flash the Arduino

[← Back to guides](README.md)

---

## Step 1: Install the Arduino IDE and RTClib

1. Install **Arduino IDE 2** from https://www.arduino.cc/en/software.
2. Open it and go to **Tools → Manage Libraries…** (or the books icon on the left).
3. Search for **`RTClib`** and install **"RTClib by Adafruit"**. When it asks to install dependencies (Adafruit BusIO), click **Install All**.

## Step 2: Check the settings at the top of the sketch

Open [`Arduino_sketches/ds3231_trigger_relay/ds3231_trigger_relay.ino`](../Arduino_sketches/ds3231_trigger_relay/ds3231_trigger_relay.ino).

### Time zone (set once, then leave it)

```cpp
const int8_t STD_OFFSET_HOURS = -7;   // Colorado (MST) = -7
const bool   OBSERVE_DST      = true; // MDT = -6 in summer, switched automatically
```

These are correct for Colorado. **Don't change them for daylight saving.** The sketch switches between MST and MDT by itself ([guide 4](04_daylight_saving_time.md)).

### Your light schedule

```cpp
Event events[] = {
  // { hour, minute, second, GPIO pin, state }
  {  0,  0,  0,  8, LOW  },   // at 00:00:00 → pin 8 LOW  (lights off)
  {  8,  0,  0,  8, HIGH },   // at 08:00:00 → pin 8 HIGH (lights on)
};
```

- Times are **local wall-clock time**: whatever your phone says, summer or winter.
- `HIGH` = relay ON (lights on), `LOW` = relay OFF (lights off).
- Events can be in any order. Add as many lines as you like.
- **Overnight schedules work**, e.g. `{22,0,0,8,HIGH}` plus `{6,0,0,8,LOW}` keeps the lights on from 10 pm to 6 am.
- The sketch always applies **the most recent event**. After a power outage or reboot, the relay comes straight back to the right state.

### Fail-safe

```cpp
const uint8_t FAILSAFE_STATE = LOW;   // relay state if the clock can't be trusted
```

If the clock has never been set, the DS3231 is unplugged, or it reports that it lost power (dead coin cell during a power cut), the sketch **won't guess**. It holds the relay in `FAILSAFE_STATE` and fast-blinks the Arduino's **L** LED until you sync the clock from a laptop. Change it to `HIGH` if your plants are better off with the lights stuck on than off.

## Step 3: Upload

1. Plug the Arduino into your computer with a USB cable.
2. **Tools → Board →** *Arduino AVR Boards →* **Arduino Uno** (or whichever board you have).
3. **Tools → Port →** pick the port that says *Arduino Uno* (e.g. `COM3`).
4. Click **Upload** (the → arrow). Wait for *"Done uploading."*

Uploading doesn't erase the clock (it lives in the DS3231) or the saved sync/battery dates (they live in the Arduino's EEPROM), so you can change the schedule and re-upload at any time.

> A brand-new setup will **fast-blink** after upload because the clock hasn't been set yet. That's expected: wire it up ([guide 2](02_wiring.md)) and run the laptop sync ([guide 3](03_sync_clock_from_laptop.md)).

## Talking to the sketch (optional)

Open **Tools → Serial Monitor**, set **57600 baud** and **New Line** line ending, and type:

| Command | What it does |
|---------|--------------|
| `STATUS` | Shows UTC and local time, DST, lost-power flag, last sync date, battery install date |
| `HELP` | Lists the commands |

The sketch also prints a status line once a minute and a line every time the relay switches:

```
UTC 2026-11-16 18:45:00 | local 2026-11-16 11:45:00 (UTC-7, standard)
  -> pin 8 HIGH (relay ON)
```

> Close the Serial Monitor before running the laptop sync script. Only one program can use the port at a time.

---

**Next:** [2. Wiring →](02_wiring.md)
