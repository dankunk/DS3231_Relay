# 6. Troubleshooting

[← Guides](README.md)

---

## Status LED (Arduino **L** LED)

| LED | Meaning | Action |
|-----|---------|--------|
| Short blink every 2 s | Running normally | None |
| Fast blink | The clock was never set, the RTC lost power, or the RTC isn't responding. The relay is held in `FAILSAFE_STATE`. | Check the DS3231 wiring, then run `sync_rtc.py`. Add `--battery-replaced` if you replaced the cell. |
| Off or solid | The sketch isn't running | Check the Arduino's power, then upload the sketch again |

## Sync script messages

| Message | Likely cause | Resolution |
|---------|--------------|------------|
| `pyserial package is missing` | The library isn't installed | `py -m pip install pyserial` |
| `'py' is not recognized` | Python isn't installed or isn't on PATH | Install Python from python.org with **Add python.exe to PATH** selected, or try `python` instead of `py` |
| `No Arduino found` | The cable is unplugged, the cable is charge-only, or the board uses an unusual USB chip | Try a different cable. Run with `--list-ports` and pass `--port COMx`. |
| `More than one Arduino-like port found` | Several boards are connected | Pass `--port COMx` |
| `Serial port problem … Access is denied` | Another program has the port open, such as the Arduino IDE Serial Monitor | Close that program and run the script again |
| `The Arduino isn't answering` | The board is running a different sketch, or you selected the wrong port | Upload `ds3231_trigger_relay` ([guide 1](01_arduino_setup.md)) |
| `Arduino says: RTC not found` | DS3231 wiring problem | Check VIN, GND, SDA → A4 and SCL → A5 |
| `Could not reach an internet time server` | No internet connection, or the network blocks NTP (UDP port 123) | The script uses the laptop's clock instead. Sync the system clock first, or use a different network. |
| `RTC seconds aren't ticking: oscillator stopped` | The RTC lost all power | Setting the clock restarts the oscillator. Replace the coin cell. |
| `the RTC was holding LOCAL time` | The RTC was previously set to local time by other software | Expected on the first sync. The sync stores UTC from then on. |
| `This computer is on UTC-5…` | The laptop's time zone differs from the sketch's | This is fine when traveling. Otherwise, check `STD_OFFSET_HOURS` in the sketch. |

## Lights

| Symptom | Likely cause | Resolution |
|---------|--------------|------------|
| Off by **several whole hours** | The RTC holds local time instead of UTC | Run `sync_rtc.py` |
| Off by **exactly 1 hour** | `STD_OFFSET_HOURS` or `OBSERVE_DST` is wrong for your location | See [Changing the time zone](04_daylight_saving_time.md#changing-the-time-zone-or-disabling-dst) |
| Off by a few minutes | Normal drift over a long period | Run `sync_rtc.py` |
| Never switch, but the serial log shows `-> pin 8 HIGH` | The light is in the wrong outlet, or the control wires are loose or reversed | Use a **Normally OFF** outlet. Connect pin 8 to **+** and GND to **−**. |
| Inverted (on when they should be off) | The light is in the **Normally ON** outlet | Move it to a **Normally OFF** outlet |
| Arduino resets when the lights switch | The Arduino is powered from a switched outlet | Plug the Arduino's power adapter into the **Always ON** outlet |
| Lights go off briefly during a sync | The Arduino restarts when its USB cable is moved or the serial port opens | Expected. Sync during the lights-off period ([details](03_sync_clock_from_laptop.md#does-syncing-interrupt-the-lights)). |

## Arduino IDE

| Symptom | Resolution |
|---------|------------|
| The Serial Monitor shows unreadable characters | Set the baud rate to **57600** |
| Typed commands have no effect | Set the line ending to **New Line** |
| Upload fails, or the port is missing | Check **Tools → Board** and **Tools → Port**. Close the sync script and any other program using the serial port. |
| `RTClib.h: No such file` | Install **RTClib by Adafruit** from the Library Manager |
