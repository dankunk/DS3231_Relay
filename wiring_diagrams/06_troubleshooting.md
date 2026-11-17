# 6. Troubleshooting

[← Back to guides](README.md)

---

## Arduino L LED

| LED | Meaning | Fix |
|-----|---------|-----|
| Short blink every 2 s | Running normally | — |
| Fast blink | Clock never set, RTC lost power (dead coin cell during an outage), or RTC unplugged. Relay held in `FAILSAFE_STATE` | Check wiring, then run `sync_rtc.py` (add `--battery-replaced` if you swapped the cell) |
| Off or solid | Sketch not running | Re-upload the sketch |

## Laptop sync script

| Message | Likely cause | Fix |
|---------|--------------|-----|
| `pyserial package is missing` | Library not installed | `py -m pip install pyserial` |
| `'py' is not recognized` | Python not installed, or not on PATH | Install from python.org with **"Add python.exe to PATH"** ticked. Try `python` instead of `py` |
| `No Arduino found` | Cable unplugged, charge-only cable, or unusual USB chip | Try another cable; run with `--list-ports` and pass `--port COMx` |
| `More than one Arduino-like port found` | Several boards plugged in | Pass `--port COMx` |
| `Serial port problem … Access is denied` | Arduino IDE Serial Monitor (or another program) has the port open | Close it and run again |
| `The Arduino isn't answering` | Old sketch on the board, or wrong port | Upload the current `ds3231_trigger_relay` sketch ([guide 1](01_arduino_setup.md)) |
| `Arduino says: RTC not found` | DS3231 wiring | Check VCC, GND, SDA→A4, SCL→A5 |
| `Could not reach an internet time server` | No internet, or the network blocks NTP (UDP 123) | It falls back to the laptop clock. Sync the laptop's clock in Windows settings first, or try a phone hotspot |
| `RTC seconds aren't ticking: oscillator stopped` | RTC lost all power | Setting the clock restarts it; replace the coin cell |
| `about -6 h: the RTC was holding LOCAL time` | First sync after moving from the old Pico method | Expected once, nothing to do |
| `This computer is on UTC-5…` | Laptop's time zone isn't Mountain | Fine if you're travelling; otherwise check `STD_OFFSET_HOURS` in the sketch |

## Lights

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Wrong by **exactly 6 or 7 h** | RTC still holds local time from the old Pico script | Run `sync_rtc.py` |
| Wrong by **exactly 1 h** | `STD_OFFSET_HOURS` / `OBSERVE_DST` wrong | Should be `-7` and `true` for Colorado |
| Wrong by a few minutes | Normal drift after a long time | Run `sync_rtc.py` |
| Never switch, but serial shows `-> pin 8 HIGH` | Lights in the wrong outlet, or relay wires loose/backwards | Use a **Normally OFF** outlet; pin 8 → **+**, GND → **−** |
| Inverted (on when they should be off) | Lights in the **Normally ON** outlet | Move them to **Normally OFF** |
| Blink off briefly when the laptop connects | The Uno restarts when the serial port opens | Normal; sync during lights-off ([details](03_sync_clock_from_laptop.md#will-syncing-blink-the-lights)) |

## Arduino IDE

| Symptom | Fix |
|---------|-----|
| Serial Monitor shows garbage | Set it to **57600** baud |
| Typed commands do nothing | Set the line ending to **New Line** |
| Upload fails / port missing | Check **Tools → Board/Port**; close the sync script or any other serial program |
| `RTClib.h: No such file` | Install **RTClib by Adafruit** in the Library Manager |
