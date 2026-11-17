# 4. Daylight saving time: how it works

**Short version:** you no longer touch anything for DST. The DS3231 holds UTC all year, and the Arduino works out whether it's MST or MDT on its own.

[← Back to guides](README.md) · [← 3. Laptop sync](03_sync_clock_from_laptop.md)

---

## Why the RTC stores UTC

Previously the RTC held *local* time (UTC−6), so every March and November someone had to re-set it or edit code.

Now:

| Device | Does what |
|--------|-----------|
| **Laptop** (`sync_rtc.py`) | Writes **UTC** to the DS3231. UTC never jumps for daylight saving. |
| **DS3231** | Just counts seconds in UTC, all year round. |
| **Arduino** | Reads UTC, decides if DST is in effect, and adds **−7 h** (MST) or **−6 h** (MDT). The schedule is compared against that local time. |

Because the DST rule is calculated, not stored, the same sketch works in 2027, 2030 and so on without changes.

## The rule the Arduino uses (US, since 2007)

- **DST starts** on the **2nd Sunday of March** at **2:00 am** local standard time. The clocks jump to 3:00 am.
- **DST ends** on the **1st Sunday of November** at **2:00 am** local daylight time. The clocks fall back to 1:00 am.

Upcoming changes for Colorado:

| Year | DST starts (spring forward) | DST ends (fall back) |
|------|-----------------------------|----------------------|
| 2027 | Sun March 14 | Sun November 7 |
| 2028 | Sun March 12 | Sun November 5 |
| 2029 | Sun March 11 | Sun November 4 |
| 2030 | Sun March 10 | Sun November 3 |
| 2031 | Sun March 9  | Sun November 2 |

The calculation (`isDST()` in [`ds3231_trigger_relay.ino`](../Arduino_sketches/ds3231_trigger_relay/ds3231_trigger_relay.ino)) was checked against the real US DST dates for every year from 2007 to 2099, including the exact second of each switch. After each sync the laptop script also prints the local time the Arduino computed and compares its UTC offset with the laptop's own time zone. That gives an independent check on real hardware.

## What happens to the schedule on change days

The sketch decides the relay state from "what's the most recent event before *now*?" every second, instead of waiting for an exact time to tick past. So:

- **Spring forward:** an event scheduled between 2:00 and 2:59 am (a time that doesn't exist that night) still takes effect at 3:00 am.
- **Fall back:** the 1:00–1:59 am hour happens twice. The relay state is the same both times, so nothing flickers.
- Events at other times are unaffected. The lights simply follow the wall clock.

## Moving to another time zone, or turning DST off

Edit the two settings at the top of the sketch and re-upload. The RTC doesn't need re-syncing, because it's still UTC.

| Location | `STD_OFFSET_HOURS` | `OBSERVE_DST` |
|----------|-------------------|---------------|
| Colorado / Mountain | `-7` | `true` |
| Arizona (no DST) | `-7` | `false` |
| Pacific | `-8` | `true` |
| Central | `-6` | `true` |
| Eastern | `-5` | `true` |
| Keep everything in UTC | `0` | `false` |

> The DST rule is the **US** rule. Other countries switch on different dates, so `isDST()` would need changing.

> If DST is ever made permanent, set `OBSERVE_DST = false` and `STD_OFFSET_HOURS` to the new permanent offset (−6 for Colorado), then re-upload.

## Migrating from the old Pico W / local-time setup (one time only)

An RTC set with the old Pico W script holds **local** time. With the new sketch it would be **6–7 hours off** until it's re-synced.

1. Wire the DS3231 to the Arduino ([guide 2](02_wiring.md)) and upload the new sketch ([guide 1](01_arduino_setup.md)).
2. Run `sync_rtc.py` ([guide 3](03_sync_clock_from_laptop.md)). It will note that the RTC was holding local time; that's expected.

The Pico W is no longer needed. Its code is still in this repo's git history if you ever want it.

---

**Next:** [5. RTC battery & yearly maintenance →](05_battery_and_maintenance.md)
