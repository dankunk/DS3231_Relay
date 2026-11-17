# 3. Sync the clock from a laptop

Plug a laptop into the Arduino's USB port and run one script. It sets the DS3231 to the exact time, checks its health, and tracks the coin cell's age. No Wi-Fi on the Arduino, no extra boards, and nothing to unplug.

[← Back to guides](README.md) · [← 2. Wiring](02_wiring.md)

---

## When to run it

- **First install** (the Arduino fast-blinks until the clock is set)
- **About once a year** to correct drift (a DS3231 drifts at most ~1 minute per year)
- **After replacing the coin cell**, or whenever the Arduino's L LED fast-blinks

**Not** for daylight saving. That's automatic ([guide 4](04_daylight_saving_time.md)).

## One-time laptop setup

1. Install **Python 3** from https://www.python.org/downloads/. On Windows, tick **"Add python.exe to PATH"** in the installer.
2. Install the serial library. Open a terminal (Windows: *Start → "Terminal"* or *"Command Prompt"*) and run:

   ```bash
   py -m pip install pyserial
   ```

   (Mac/Linux: `python3 -m pip install pyserial`)

   On Windows, `sync_rtc.bat` also installs it for you the first time.

3. Copy the [`laptop_sync/`](../laptop_sync) folder from this repo onto the laptop (or clone the whole repo).

## Running it

1. **Close the Arduino IDE's Serial Monitor** if it's open.
2. Plug the laptop into the Arduino's USB port.
3. Run the sync:
   - **Windows:** double-click **`laptop_sync/sync_rtc.bat`**, or in a terminal:

     ```bash
     py laptop_sync/sync_rtc.py
     ```

   - **Mac/Linux:**

     ```bash
     python3 laptop_sync/sync_rtc.py
     ```

4. It takes about 10 seconds. Read the report, then unplug the laptop.

The script finds the Arduino's port automatically. If it can't (e.g. two Arduinos plugged in), add `--port COM3` (Windows) or `--port /dev/ttyACM0` (Mac/Linux). `--list-ports` shows what's connected.

### Example output

```
=== DS3231 relay timer: clock sync ===

Time source: internet time (time.google.com) (+/-12 ms)
This computer's clock is off by +254 ms
Arduino port: COM3
USB round trip: 4.1 ms (compensated automatically)

--- Before sync ---
RTC time:        2026-11-16 18:41:07 UTC
Lost-power flag: clear
RTC temperature: 22.25 C
RTC error:       +2.70 s
[OK] Drift since last sync (100 days ago): +0.31 ppm (~10 s/year; a healthy DS3231 is within +/-2 ppm)

--- Setting clock ---
RTC set to 2026-11-16 18:41:06 UTC
[OK] Measured RTC error after sync: +1 ms  (worst case about +/-14 ms incl. USB timing)

--- Arduino now reports ---
Local time:   2026-11-16 11:41:07 (UTC-7, standard time)
Next change:  2027-03-14 03:00:00 local (automatic)
[OK] Matches this computer's time zone.

[OK] Coin cell is 1.1 years old. Replace by about 2028-10-12.

Done. The relay schedule is running. You can unplug the laptop.
```

### Options

| Command | What it does |
|---------|--------------|
| `sync_rtc.py` | Check health, then set the clock |
| `sync_rtc.py --check` | Only measure and report; changes nothing |
| `sync_rtc.py --battery-replaced` | Also record that the coin cell was replaced **today** |
| `sync_rtc.py --battery-replaced 2026-05-01` | …or on a specific date |
| `sync_rtc.py --no-ntp` | Use the laptop's clock instead of internet time |
| `sync_rtc.py --verbose` | Also show the Arduino's log lines |

The first time it runs, the script asks when the coin cell was installed (press Enter to skip). That date is stored on the Arduino, so later syncs can tell you when it's due.

## Where the time comes from

1. **Internet time (NTP)**, if the laptop can reach a time server. The script asks the servers directly, so it's accurate to a few milliseconds even if the laptop's own clock is off. Campus networks like eduroam usually allow this. The *microcontroller* couldn't join eduroam, but the laptop can.
2. **The laptop's own clock**, if no time server answers. The script warns you. Windows keeps its clock roughly right automatically, but it can be off by a second or more. Click *Settings → Time & language → Date & time → Sync now* first if you're offline.

## Does the USB connection add delay?

A little, a few milliseconds, and the script cancels it out:

1. **It measures the delay.** The script pings the Arduino 12 times and times the round trip (typically 2–20 ms).
2. **It sends early.** It sends `SET <time>` ahead of time *without* the final newline. Then it sends that single newline byte early by the measured one-way delay, so it arrives right on the second.
3. **The Arduino acts instantly.** The sketch checks for serial input constantly and writes the DS3231 the moment the newline arrives. Writing the seconds register also restarts the DS3231's sub-second counter, so the new second starts right then.
4. **It measures the result.** The Arduino reports the exact moment its RTC's seconds roll over, and the script compares that with true time. That's the *"Measured RTC error after sync"* line.

Typical result: **within a few milliseconds**. The "worst case" figure is an upper bound for unusual USB adapters whose delay differs in each direction. Even that is hundreds of times smaller than one year of normal DS3231 drift, and far below anything that matters for a light schedule.

## Will syncing blink the lights?

Possibly, for 1–2 seconds. On an Uno, opening the serial port normally **restarts the Arduino** (that's how the IDE uploads sketches). The script tries to prevent this, which works on some boards. During a restart the relay briefly drops to off, then the sketch immediately restores the correct state.

To avoid any effect:

- **Sync during the lights-off period** (simplest), or
- Use a board that doesn't restart on connect (Uno R4, Leonardo, Micro), or
- Advanced: a 10 µF capacitor between the Uno's **RESET** and **GND** pins disables the auto-restart. Remove it before uploading a new sketch.

> HID / metal-halide lamps need several minutes to restrike after any power interruption, so sync them during lights-off. LED fixtures don't care.

## No laptop handy? Check from the Serial Monitor

Typing `STATUS` in the Arduino IDE's Serial Monitor (57600 baud, *New Line*) shows the time, DST, lost-power flag, last sync date and battery date. It can't set the clock accurately by hand; use the script for that.

---

**Next:** [4. Daylight saving time: how it works →](04_daylight_saving_time.md)
