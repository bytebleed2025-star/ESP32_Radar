# Wiring — ESP32 30-pin and 38-pin

Exact wiring for the LACAK Detektor Air radar with an **ST7735 1.77"/1.8"**
display. The GPIO numbers are the same for both boards; only the physical
header position differs.

> The 2.4" **ILI9341 SPI** build (`feature/ili9341-2.4`) uses the same ESP32
> pins except `LED → GPIO32` instead of the 1.77" LEDA wiring.

---

## 1. Signal map (30-pin and 38-pin identical)

| Signal | ESP32 GPIO | Notes |
|---|---|---|
| TFT GND | GND | common ground |
| TFT VCC | **3V3** | never 5 V |
| TFT LEDA (1.77" only) | **3V3** | backlight anode |
| TFT SCK | **GPIO14** | SPI clock |
| TFT SDA | **GPIO13** | SPI MOSI |
| TFT RES | **GPIO25** | reset |
| TFT RS / DC | **GPIO26** | data/command |
| TFT CS | **GPIO27** | chip select |
| TFT BL (1.8" only) | **GPIO32** | backlight control |
| HC-SR04 TRIG | **GPIO5** | output |
| HC-SR04 ECHO | **GPIO18** | via 5 V→3.3 V divider |
| Buzzer | **GPIO23** | active or transistor |
| SN04-N signal | **GPIO34** | input-only, needs pull-up |
| SN04 counter reset | **GPIO33** | optional button to GND |

**Do not connect:** TFT MISO (1.77" module has none), `GT_*` touch pins, SD
pins.

---

## 2. 30-pin DevKit (DOIT DevKit V1)

```text
LEFT HEADER (USB end at bottom)        RIGHT HEADER
EN                                    3V3   ← TFT VCC + LEDA
VP  36                                GND
VN  39                                15
34   ← SN04 signal                    2
35                                    4
32                                    16
33   ← reset button                   17
25   ← TFT RES                        5    ← HC-SR04 TRIG
26   ← TFT RS/DC                      18   ← HC-SR04 ECHO (divider)
27   ← TFT CS                         19
14   ← TFT SCK                        21
12   (leave open)                     RX0  3
13   ← TFT SDA                         TX0  1
GND                                   22
VIN  ← 5 V for HC-SR04                23   ← buzzer

TFT GND → any GND (left GND or right GND)
SN04 BU → GND
```

Some 30-pin boards print the header in a different order — always follow the
**labels on the board**, not the position.

---

## 3. 38-pin board

Same signals; connect by the printed GPIO label:

```text
3V3  → TFT VCC + LEDA
GND  → TFT GND, HC-SR04 GND, SN04 BU
14   → TFT SCK
13   → TFT SDA
25   → TFT RES
26   → TFT RS
27   → TFT CS
5    → HC-SR04 TRIG
18   → HC-SR04 ECHO (divider)
23   → buzzer
34   → SN04 signal
33   → reset button
5V/VIN → HC-SR04 VCC
```

The 38-pin board only adds spare pins (0, 2, 4, 15, 16, 17, 19, 21, 22, 35,
36, 39). None are required.

---

## 4. HC-SR04 (5 V sensor, 3.3 V ESP32)

```text
HC-SR04 VCC  ─── 5V / VIN
HC-SR04 GND  ─── GND
HC-SR04 TRIG ─── GPIO5
HC-SR04 ECHO ──[1k]──┬── GPIO18
                     │
                   [2k]
                     │
                    GND
```

ECHO is 5 V — never connect it directly to the ESP32.

---

## 5. SN04-N inductive sensor (6–30 V)

Label on the sensor: `BN 6~30V`, `BK NPN.NO`, `BU 0V`.

```text
BN (brown) ─── +12 V (or 9 V battery)
BU (blue)  ─── GND (common with ESP32)
BK (black) ──[1k]── GPIO34
                     │
                   [10k]
                     │
                    3V3
```

- GPIO34 has **no internal pull-up**, so the 10 kΩ is required.
- The sensor needs **6–30 V**; it will not run from 5 V.
- All grounds must be common.

Optional counter reset:

```text
GPIO33 ── push button ── GND
```

GPIO33 uses the internal pull-up (active LOW, ~40 ms debounce). A short press
clears the PASSES counter, saves it to NVS, and updates the web UI, Bluetooth
and TFT at once. The firmware prints `[BTN] counter reset (GPIO33)`.

Free pins if GPIO33 is taken (38-pin): 0, 2, 4, 15, 16, 17, 19, 21, 22, 35,
36, 39. On the 30-pin board: 2, 4, 15, 16, 17, 19, 21, 22, 35, 36, 39. Change
the pin at build time with:

```ini
build_flags = -DPIN_PROX_RESET=13
```

---

## 6. Buzzer (GPIO23)

**3.3 V active buzzer:**

```text
Buzzer + ─── GPIO23
Buzzer − ─── GND
```

**5 V buzzer (louder):**

```text
GPIO23 ──[1k]── Base  (2N2222 / BC337)
Emitter ─── GND
Collector ─── Buzzer −
Buzzer + ─── 5 V
```

---

## 7. ST7735 1.77" vs 1.8"

| | 1.77" module | 1.8" module |
|---|---|---|
| Header | `GND VCC SCK SDA RES RS CS` (1–7) | `VCC GND CS RESET DC MOSI SCK LED MISO` |
| Backlight | `LEDA` (pins 8/9) → 3.3 V | `LED` → GPIO32 |
| MISO | not present | GPIO12 (optional) |
| Touch / SD | `GT_*` unused | none |

Both use the same ST7735 driver and the same ESP32 pins.

---

## 8. Power

- Bench test: power the ESP32 from USB.
- TFT VCC/LEDA and all logic: **3.3 V only**.
- HC-SR04: **5 V / VIN**.
- SN04: separate **9–12 V** supply, grounds joined.
- 9 V battery: use a **5 V buck** into VIN/5V; a carbon-zinc 9 V cannot supply
  the ESP32 current peaks (random resets/freezes).

---

## 9. Notes and pitfalls

- **GPIO12** is a strapping pin (flash voltage). Leave it open on the 1.77"
  build; do not add a pull-up.
- **GPIO5/15** are strapping pins; the HC-SR04 TRIG on GPIO5 is high-impedance
  at boot, which is fine.
- Keep TFT wires short; 1 MHz SPI is tolerant but long dupont leads can pick up
  noise.
- White/grey screen with backlight on → data wiring or wrong TAB, not power.
- Black screen → backlight (LEDA) or power.
- If the image is shifted with a 2 px strip, use the matching TAB build
  (`...-177-00` or `...-177-21`).

---

## 10. Quick check list

1. 3.3 V between TFT VCC and GND.
2. 3.3 V on LEDA (1.77").
3. Continuity: SCK↔14, SDA↔13, RES↔25, RS↔26, CS↔27.
4. HC-SR04 TRIG↔5, ECHO↔18 through the divider.
5. SN04 BK↔34 through 1 kΩ, 10 kΩ to 3.3 V.
6. Common GND everywhere.
7. Serial at 115200 shows `Ready.` and the `[DIAG]` heartbeat.
