# 5. RTC battery & yearly maintenance

[← Back to guides](README.md) · [← 4. Daylight saving time](04_daylight_saving_time.md)

---

## What the coin cell actually does

The DS3231 runs from the Arduino's power. The **CR2032 only keeps the clock ticking when that power is gone**, e.g. during a campus power outage or while the setup is unplugged.

In backup mode the DS3231 draws about 1 µA, so a fresh CR2032 could in theory last for years. In practice, cells usually die from age, a poor contact, or the charging problem described below.

If the cell is dead **and** the power goes out, the clock stops. When power returns, the Arduino notices (the DS3231's *lost-power flag*), holds the relay in `FAILSAFE_STATE`, and fast-blinks until you replace the cell and re-sync. It never runs the lights on a wrong clock.

## When to replace the CR2032

The sync script tracks this for you. Replace the cell when **any** of these happens:

| Sign | Where you see it |
|------|------------------|
| `REPLACE THE COIN CELL NOW` | `sync_rtc.py` report (the RTC lost power since the last sync) |
| `Coin cell is 3.0 years old: replace it` | `sync_rtc.py` report (based on the install date you recorded) |
| Arduino **L** LED **fast-blinks** after a power outage | On site |
| Multimeter reads **below 2.8 V** across the cell | Fresh CR2032 ≈ 3.0–3.3 V |

### How to swap it

1. **Leave the Arduino powered**, then pop the old cell out and push the new one in **with `+` facing up**. With power present the clock never stops, so the time is kept.
2. Plug in the laptop and run:

   ```bash
   py laptop_sync/sync_rtc.py --battery-replaced
   ```

   This syncs the clock and records today as the install date, so future syncs can tell you when it's due again.

If you swapped the cell with the power off, that's fine too: the clock stopped, so the Arduino will fast-blink until you run the same command.

## ZS-042 charging circuit warning

Many cheap blue **"ZS-042"** DS3231 modules (the ones with a second 8-pin chip, the AT24C32 EEPROM, and a coin-cell holder on the back) include a **battery-charging circuit**: a small diode plus a 200 Ω resistor (often marked **`201`**). It was designed for **rechargeable LIR2032** cells.

With a normal **non-rechargeable CR2032**, especially when the module is powered from **5 V**, that circuit pushes current *into* the CR2032. Over months this can **swell, leak or kill the cell**.

**Pick one fix:**

1. **Power the module from 3.3 V instead of 5 V (easiest).** Move the red wire from the Uno's `5V` pin to its `3.3V` pin. At 3.3 V the voltage after the diode is below the CR2032's own voltage, so almost no charging current flows.
2. **Remove the charging path.** Desolder or snip one leg of the small resistor marked `201`, or the diode next to it, near the SCL/VCC pins. Search "ZS-042 remove charging resistor" for photos.
3. **Use an LIR2032 rechargeable cell** instead of a CR2032.

The **Adafruit DS3231** breakout and most "mini" DS3231 boards have **no** charging circuit and need none of this.

## Yearly checklist (about 10 minutes, all on site)

DST no longer requires visits. One check a year keeps the clock accurate and catches battery problems early. Pick a date that suits your schedule, ideally during the **lights-off** period.

- [ ] Close the Arduino IDE Serial Monitor and plug the laptop into the Arduino
- [ ] Run `sync_rtc.py` and read the report:
  - `REPLACE THE COIN CELL NOW` or `…years old: replace it` → swap the cell, run again with `--battery-replaced`
  - Drift flagged `[!!]` (more than ±5 ppm) → watch it next year; replace the module if it keeps happening
- [ ] Check the "Local time" line matches your phone
- [ ] Unplug the laptop; confirm the Arduino's L LED does a short blink every 2 s

## How accurate is it between syncs?

The DS3231 has a built-in temperature-compensated crystal: **±2 ppm** from 0–40 °C, which works out to **at most about 1 minute per year**. The sync script measures the actual drift each time (in ppm), so you'll see if a module is going bad.

---

**Next:** [6. Troubleshooting →](06_troubleshooting.md)
