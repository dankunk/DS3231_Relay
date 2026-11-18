# Bill of Materials

This list covers all the parts in the reference build. Every part is available from [Digi-Key](https://www.digikey.com). Each link opens a Digi-Key search for that part number.

## Core parts

| # | Qty | Part | Manufacturer / MPN | Digi-Key part number | Unit price (USD) |
|---|-----|------|--------------------|----------------------|------------------|
| 1 | 1 | IoT Power Relay (Digital Loggers IoT Relay) | SparkFun Electronics / COM-14236 | [COM-14236-ND](https://www.digikey.com/en/products/result?keywords=COM-14236-ND) | 37.20 |
| 2 | 1 | Power cord for the relay: NEMA 5-15P to IEC C13, 16 AWG, 7.5 ft | Qualtek / 212013-01 | [Q124-ND](https://www.digikey.com/en/products/result?keywords=Q124-ND) | 6.96 |
| 3 | 1 | DS3231 Precision RTC, STEMMA QT | Adafruit / 5188 | [1528-5188-ND](https://www.digikey.com/en/products/result?keywords=1528-5188-ND) | 13.95 |
| 4 | 1 | STEMMA QT (JST SH 4-pin) to premium male header cable | Adafruit / 4209 | [1528-4209-ND](https://www.digikey.com/en/products/result?keywords=1528-4209-ND) | 0.95 |
| 5 | 2 | CR1220 3 V lithium coin cell (one installed, one spare) | FDK America / CR1220 | [SY033-ND](https://www.digikey.com/en/products/result?keywords=SY033-ND) | 0.79 |
| 6 | 1 | USB 2.0 cable, A plug to B plug, 0.5 m (1.64 ft) | Same Sky / CBL-UA-UB-05BP | [2223-CBL-UA-UB-05BP-ND](https://www.digikey.com/en/products/result?keywords=2223-CBL-UA-UB-05BP-ND) | 3.52 |

The core parts total **$64.16** (USD, excluding tax and shipping). Prices are from the reference build's order, so check Digi-Key for current pricing.

## Additional parts

You also need these parts. Any equivalent part works.

| Qty | Part | Notes | Digi-Key search |
|-----|------|-------|-----------------|
| 1 | Arduino Uno R3 | Or a compatible board. See the [wiring guide](../wiring_diagrams/02_wiring.md#ds3231-to-arduino) for other boards' I²C pins. | [Arduino A000066](https://www.digikey.com/en/products/result?keywords=A000066) |
| 1 | 5 V USB wall adapter | Any phone charger with a USB-A port works. It plugs into the relay's **Always ON** outlet. | [USB wall adapter](https://www.digikey.com/en/products/result?keywords=5V%20USB%20wall%20adapter) |
| 2 | Jumper wires, male-to-male | Small hobby jumper wires that connect Arduino pin 8 and GND to the relay's control terminal | [Jumper wires](https://www.digikey.com/en/products/result?keywords=jumper%20wire%20male%20to%20male) |
| 2 | Cable ties, thin (about 2.5 mm wide) | Hold the relay in the 3D-printed mount. Use ties long enough to wrap around the relay and the tray. | [Cable ties](https://www.digikey.com/en/products/result?keywords=cable%20tie%202.5mm) |

## Not included

- **Grow light:** any light with a standard US plug. It plugs into a **Normally OFF** outlet on the relay.
- **3D-printed mount (optional):** see [`enclosure/`](../enclosure/README.md).
- **Laptop with Python 3:** used for clock syncs. See the [clock sync guide](../wiring_diagrams/03_sync_clock_from_laptop.md).
