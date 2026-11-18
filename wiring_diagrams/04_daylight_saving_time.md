# 4. Daylight Saving Time

**Summary:** You don't need to do anything for daylight saving time. The DS3231 keeps UTC all year, and the Arduino works out whether standard or daylight time applies.

[← Guides](README.md) · [← 3. Clock sync](03_sync_clock_from_laptop.md)

---

## Why the RTC stores UTC

An RTC set to local time has to be reset at every daylight saving change. UTC never shifts for daylight saving, so the timer stores UTC and calculates local time from it:

| Component | Role |
|-----------|------|
| **Laptop** (`sync_rtc.py`) | Writes **UTC** to the DS3231 |
| **DS3231** | Counts seconds in UTC all year |
| **Arduino** | Reads UTC, decides whether DST is in effect, and applies the offset: −7 h (MST) or −6 h (MDT) by default. The schedule is compared against the resulting local time. |

The sketch calculates the DST rule instead of storing a table of dates, so it keeps working in future years without changes.

## The DST rule (US, since 2007)

- **DST starts** on the **second Sunday in March** at 02:00 local standard time. Clocks move forward to 03:00.
- **DST ends** on the **first Sunday in November** at 02:00 local daylight time. Clocks move back to 01:00.

| Year | DST starts | DST ends |
|------|------------|----------|
| 2027 | Sun 14 March | Sun 7 November |
| 2028 | Sun 12 March | Sun 5 November |
| 2029 | Sun 11 March | Sun 4 November |
| 2030 | Sun 10 March | Sun 3 November |
| 2031 | Sun 9 March | Sun 2 November |

The calculation (`isDST()` in [`ds3231_trigger_relay.ino`](../Arduino_sketches/ds3231_trigger_relay/ds3231_trigger_relay.ino)) has been checked against the published US DST transitions for every year from 2007 to 2099, down to the second of each switch. After each sync, `sync_rtc.py` also prints the local time the Arduino calculated and compares its UTC offset with the laptop's time zone. This gives an independent check on the real hardware.

## Schedule behavior on transition days

Every second, the sketch sets the relay to match the most recent scheduled event before the current time. It doesn't wait for an exact time to pass. As a result:

- **Spring forward:** an event scheduled between 02:00 and 02:59, a time that doesn't exist that night, takes effect at 03:00.
- **Fall back:** the hour from 01:00 to 01:59 occurs twice. The relay state is the same both times, so nothing flickers.
- Events at all other times are unaffected. The lights follow the wall clock.

## Changing the time zone or disabling DST

Edit the two settings at the top of the sketch and upload it again. You don't need to re-sync the RTC, because it still holds UTC.

| Location | `STD_OFFSET_HOURS` | `OBSERVE_DST` |
|----------|--------------------|---------------|
| US Mountain | `-7` | `true` |
| Arizona (no DST) | `-7` | `false` |
| US Pacific | `-8` | `true` |
| US Central | `-6` | `true` |
| US Eastern | `-5` | `true` |
| UTC (no conversion) | `0` | `false` |

> The built-in rule is the **US** daylight saving rule. Other regions change on different dates, so you would need to modify `isDST()`.

> If daylight saving time is made permanent, set `OBSERVE_DST = false` and set `STD_OFFSET_HOURS` to the new year-round offset (for example, −6 for US Mountain). Then upload the sketch again.

---

**Next:** [5. Battery and maintenance →](05_battery_and_maintenance.md)
