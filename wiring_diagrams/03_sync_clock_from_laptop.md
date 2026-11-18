# 3. Clock Sync

`sync_rtc.py` sets the DS3231 over the Arduino's USB connection. In one run it sets the clock to within a few milliseconds of UTC, checks the RTC's health, and tracks the coin cell's age. The Arduino needs no network connection or extra hardware.

[← Guides](README.md) · [← 2. Wiring](02_wiring.md)

---

## When to sync

- **At first installation.** The Arduino fast-blinks until the clock is set.
- **About once a year**, to correct drift. A DS3231 drifts by about one minute per year at most.
- **After replacing the coin cell**, or whenever the **L** LED is fast-blinking.

You don't need to sync for daylight saving time, because the sketch handles it ([guide 4](04_daylight_saving_time.md)).

## Laptop setup (one time)

1. Install **Python 3** from <https://www.python.org/downloads/>. On Windows, select **Add python.exe to PATH** during installation.
2. Install the serial library:

   ```bash
   py -m pip install pyserial
   ```

   On macOS or Linux, run `python3 -m pip install pyserial`. On Windows, `sync_rtc.bat` installs it automatically the first time it runs.

3. Copy the [`laptop_sync/`](../laptop_sync) folder to the laptop, or clone the repository.

## Running a sync

1. Close the Arduino IDE Serial Monitor if it's open.
2. Connect the laptop to the Arduino's USB port. If the Arduino is powered over USB, unplug its cable from the power adapter and plug it into the laptop.
3. Run the script:
   - **Windows:** double-click `laptop_sync/sync_rtc.bat`, or run:

     ```bash
     py laptop_sync/sync_rtc.py
     ```

   - **macOS / Linux:**

     ```bash
     python3 laptop_sync/sync_rtc.py
     ```

4. The sync takes about 10 seconds. Review the report, then plug the USB cable back into the power adapter.

The script finds the Arduino's serial port automatically. If more than one candidate port is connected, pass `--port COM3` (Windows) or `--port /dev/ttyACM0` (macOS/Linux). To see the available ports, run the script with `--list-ports`.

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

| Command | Description |
|---------|-------------|
| `sync_rtc.py` | Check the RTC's health, then set the clock |
| `sync_rtc.py --check` | Measure and report only; makes no changes |
| `sync_rtc.py --battery-replaced` | Also record that the coin cell was replaced **today** |
| `sync_rtc.py --battery-replaced 2026-05-01` | Record a specific replacement date |
| `sync_rtc.py --no-ntp` | Use the laptop's clock instead of internet time |
| `sync_rtc.py --port COM3` | Use a specific serial port |
| `sync_rtc.py --list-ports` | List the available serial ports |
| `sync_rtc.py --verbose` | Also show the Arduino's log output |

On the first run, the script asks when the coin cell was installed (press Enter to skip). The date is stored on the Arduino, so later syncs can report when the cell is due for replacement.

## Time source

1. **Internet time (NTP).** If the laptop can reach a time server, the script queries it directly. The result is accurate to a few milliseconds even if the laptop's own clock is wrong. Most home and institutional networks allow NTP.
2. **The laptop's clock.** If no time server responds, the script uses the laptop's clock and prints a warning. The operating system keeps this roughly correct, but it can be off by a second or more. If you're offline, sync the system clock first (on Windows: **Settings → Time & language → Date & time → Sync now**).

## How the USB delay is cancelled

USB adds a few milliseconds of delay. The script measures that delay and cancels it out:

1. **It measures the delay.** The script pings the Arduino 12 times and times each round trip. A round trip typically takes 2–20 ms.
2. **It sends the time early.** The script sends `SET <time>` without the final newline. It then sends the newline early by the measured one-way delay, so the newline arrives exactly on the second.
3. **The Arduino sets the clock as soon as the newline arrives.** The sketch polls the serial port continuously and writes the DS3231 the moment the newline arrives. Writing the seconds register also resets the DS3231's sub-second counter.
4. **It measures the result.** The Arduino reports the exact moment its RTC's seconds roll over, and the script compares that with the true time. The difference is shown as *Measured RTC error after sync*.

The typical result is within a few milliseconds. The worst-case figure is an upper bound for USB adapters whose delay is different in each direction. Even the worst case is far smaller than a year of normal DS3231 drift.

## Does syncing interrupt the lights?

Briefly, in some setups:

- **USB-powered Arduino (reference build):** Moving the cable from the power adapter to the laptop removes power for a few seconds, so the relay switches off. When power returns, the Arduino restores the scheduled state.
- **Barrel-jack-powered Arduino:** On an Uno, opening the serial port normally resets the board. The script tries to prevent this, but that works on only some boards. During a reset, the relay drops out for 1–2 seconds.

To avoid any interruption:

- **Sync during the lights-off period.** This is the simplest option.
- Use a board that doesn't reset when the serial port opens (Uno R4, Leonardo, Micro), with barrel-jack power.
- *Advanced:* Fit a 10 µF capacitor between the Uno's **RESET** and **GND** pins to disable the auto-reset. Remove it before uploading a new sketch.

> HID and metal-halide lamps need several minutes to restrike after any power interruption, so always sync them during the lights-off period. LED fixtures aren't affected.

## Checking status without the script

To view the time, DST state, lost-power flag, last sync date and battery date, type `STATUS` in the Arduino IDE Serial Monitor (57600 baud, **New Line**). The Serial Monitor can't set the clock accurately; use `sync_rtc.py` for that.

---

**Next:** [4. Daylight saving time →](04_daylight_saving_time.md)
