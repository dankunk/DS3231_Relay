# Photos of the real build

The diagrams in [`../images/`](../images) show the wiring. Photos of *your* actual hardware make it much easier for the next person (or future you) to rebuild it. Drop JPG/PNG files in this folder and link them from the guides.

## Shots worth taking

| File name (suggested) | What to photograph | Add it to |
|-----------------------|--------------------|-----------|
| `arduino_ds3231_wired.jpg` | Arduino + DS3231 wired, wire colors visible | [02_wiring.md](../02_wiring.md) |
| `ds3231_module_front_back.jpg` | Your DS3231 module, both sides (pin labels + coin cell holder) | [02_wiring.md](../02_wiring.md) |
| `relay_terminal_closeup.jpg` | The IoT Relay's green + / − terminal with wires in | [02_wiring.md](../02_wiring.md) |
| `zs042_charging_resistor.jpg` | Close-up of the removed/cut charging resistor (if ZS-042) | [05_battery_and_maintenance.md](../05_battery_and_maintenance.md#zs-042-charging-circuit-warning) |
| `laptop_sync_output.png` | Screenshot of a successful `sync_rtc.py` run | [03_sync_clock_from_laptop.md](../03_sync_clock_from_laptop.md) |
| `campus_install.jpg` | The whole setup installed on campus (lights in a Normally OFF outlet) | [README.md](../README.md) |

## How to add one to a guide

Put the image in this folder, then add a line like this to the guide's markdown (the path is relative to the guide):

    ![Arduino wired to the DS3231](photos/arduino_ds3231_wired.jpg)

Keep photos under ~1 MB each (resize phone photos first) so the repo stays small.
