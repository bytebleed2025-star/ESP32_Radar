# ST7735 local display (128×160, no touch)

This branch adds a local ST7735 SPI TFT to the radar. It is **display-only**:
the web UI, OTA and the existing radar behaviour are unchanged.

The serial Nextion display is disabled on this branch by default.

## Wiring

| ST7735 module | ESP32 |
|---|---|
| `VCC` | **3.3 V** |
| `GND` | GND |
| `SCL` / `SCLK` | **GPIO 14** |
| `SDA` / `MOSI` | **GPIO 13** |
| `CS` | **GPIO 27** |
| `DC` / `RS` | **GPIO 26** |
| `RES` / `RST` | **GPIO 25** |
| `BL` / `LED` | **GPIO 32** |

No touch and no MISO connection are used.

> Use **3.3 V logic**. Many modules have a 3.3 V regulator and accept 5 V on
> `VCC`, but the logic pins must not be driven with 5 V.

These pins do not conflict with the radar:

```text
TRIG 5, ECHO 18, SERVO 19, Buzzer 23
```

## Build

### PlatformIO

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
```

TFT_eSPI and the ST7735 pin setup are in `platformio.ini` and
`firmware/ESP32_Radar/st7735_ui.h`.

### Arduino IDE

Install the **TFT_eSPI** library, then compile `firmware/ESP32_Radar`.
The ST7735 setup is embedded in `st7735_ui.h`, so you do not need to edit the
library's `User_Setup.h`.

## Screen layout

```text
┌──────────────────────┐
│ DETEKTOR AIR   HAND  │
│ DIST   ANGLE   NEAR  │
│  35     118     35   │
│ AREA BERSIH          │
│                      │
│      radar PPI       │
│                      │
│ R 200 S 2            │
└──────────────────────┘
```

- Radar PPI at ~8 FPS
- Header shows AUTO/HAND
- Distance, angle and nearest contact update at 4 Hz
- Footer shows range, step and STOP when paused
- Amber contacts are inside the water-alert threshold

AUTO runs only while a **visible** web UI is connected. Minimizing or closing
the page stops the sweep after a 3 s heartbeat timeout; opening it again
resumes AUTO. HAND mode is independent of this gate.

## Display settings

In `st7735_ui.h` or `platformio.ini`:

```cpp
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160
#define ST7735_ROTATION 0
```

If the image is shifted or colours are wrong, change the init sequence:

- `ST7735_INITB` — common 1.8" 128×160 green-tab module
- `ST7735_INITR_GREENTAB` — red-tab variant
- `ST7735_INITR_BLACKTAB` — black-tab variant
- `TFT_RGB_ORDER` — `TFT_BGR` or `TFT_RGB`

If the display is rotated, set `ST7735_ROTATION` to `1`, `2` or `3`.

## Enabling both displays

`NEXTION_ENABLED 0` disables the Nextion UART on this branch. The ST7735 pins
do not conflict with the Nextion pins (GPIO 16/17), so both can be enabled if
needed, but the ST7735 is intended to replace the Nextion here.

## Files

- `firmware/ESP32_Radar/st7735_ui.h` — driver and layout
- `platformio.ini` — TFT_eSPI dependency and build flags
- `firmware/ESP32_Radar/ESP32_Radar.ino` — calls into `St7735UI`
