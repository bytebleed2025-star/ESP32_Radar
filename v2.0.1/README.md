# v2.0.1 — ST7735 + HC-SR04 + inductive counter + buzzer

Branch: `v2.0.1` (based on the final v2.0 branch)

This is the v2.0 fixed-direction build with the **MAX98357A I2S audio replaced
by a GPIO23 buzzer**. There is no MP3/WAV playback and no ESP8266Audio
dependency.

## Download & flash

Prebuilt **v2.0.1 ST7735** firmware (`FW_VERSION 2.0.1`):

| File | Use |
|---|---|
| [`../bin/v2.0.1/DetektorAir-v2.0.1-ST7735-factory.bin`](../bin/v2.0.1/DetektorAir-v2.0.1-ST7735-factory.bin) | blank / new board, flash at **0x0** |
| [`../bin/v2.0.1/DetektorAir-v2.0.1-ST7735-ota.bin`](../bin/v2.0.1/DetektorAir-v2.0.1-ST7735-ota.bin) | web **CONFIG → UPDATE FIRMWARE** or ArduinoOTA |

Blank board over USB (from the repository root):

```cmd
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\v2.0.1\DetektorAir-v2.0.1-ST7735-factory.bin
```

Checksums: [`../bin/v2.0.1/SHA256SUMS.txt`](../bin/v2.0.1/SHA256SUMS.txt).

## Renders

| Modern UI — idle | Modern UI — detected |
|---|---|
| ![](st7735-modern-idle.png) | ![](st7735-modern-detect.png) |

| Boot logo |
|---|
| ![](../v2.0/st7735-boot.png) |

![Web UI desktop](web-desktop.png)

![Web UI mobile](web-mobile.png)

## Hardware

| Function | ESP32 pin | Notes |
|---|---|---|
| HC-SR04 `TRIG` | GPIO 5 | 10 µs trigger |
| HC-SR04 `ECHO` | GPIO 18 | 1 kΩ / 2 kΩ divider to 3.3 V |
| ST7735 `SCLK` | GPIO 14 | |
| ST7735 `MOSI` | GPIO 13 | |
| ST7735 `CS` | GPIO 27 | |
| ST7735 `DC` | GPIO 26 | |
| ST7735 `RST` | GPIO 25 | |
| ST7735 `BL` | GPIO 32 | |
| **Buzzer `+`** | **GPIO 23** | active or passive |
| Buzzer `−` | GND | |
| Inductive sensor output | GPIO 34 | 1 kΩ series + 10 kΩ pull-up to 3.3 V |
| Inductive sensor supply | 6–36 V | common GND |
| Reset button | GPIO 33 | button to GND, internal pull-up |

There is no servo. The HC-SR04 measures continuously in one fixed direction.
GPIO 19/21/22 are now free; they were the MAX98357A I2S pins in v2.0.

### Buzzer wiring

```text
Active buzzer:   + -> GPIO 23,  - -> GND
Passive buzzer:  + -> GPIO 23,  - -> GND
```

- The firmware supports both types; select **ACTIVE** or **PASSIVE** in the web
  UI.
- Active: GPIO HIGH = sound.
- Passive: 2200 Hz tone on GPIO 23.
- A small 3.3 V buzzer can be driven directly. If the buzzer draws more than
  about 12 mA, use an NPN transistor (1 kΩ base resistor) or a small N-channel
  MOSFET.
- Never connect a 5 V buzzer directly to GPIO 23.

## Buzzer behaviour

At boot, after WiFi/AP and the HC-SR04 start:

| State | Pattern | Default |
|---|---|---|
| Idle | long ON / long OFF | **1000 ms / 1500 ms** |
| Proximity pulse | short ON / short OFF | **500 ms / 750 ms** |
| Alert hold | short pattern duration after a pulse | **5000 ms** |

- Each new inductive detection increments the counter, saves it in NVS, and
  restarts the short alert pattern.
- After the alert hold expires, the long idle pattern resumes.
- All timings are adjustable in the web UI and stored in NVS.
- The buzzer can be enabled/disabled from the web UI.
- The counter can still be reset by GPIO33, the web UI, or the existing
  **RESET COUNTER** button.

## Web UI

The **CONTROL** panel now includes a **BUZZER PATTERN** card:

- **BUZZER: ON/OFF**
- **ACTIVE / PASSIVE**
- LONG ON, LONG OFF, SHORT ON, SHORT OFF, ALERT HOLD (ms)
- **SIMPAN TIMING** saves all five values to the ESP32

The web UI also retains the sonar scope, telemetry, proximity counter, config
and OTA pages.

## Build

```bash
pio run -e esp32dev
```

PlatformIO result: **1,158,061 bytes (58.9%) flash**, **51,488 bytes (15.7%)
RAM**.

The verified display configuration is:

```text
ST7735_GREENTAB
HSPI
SPI 1 MHz
MISO = GPIO12
```

This was confirmed with the `HWSPI-hspi-1mhz` test on the actual panel. The
1 MHz clock is deliberate: it is reliable on breadboard jumper wiring. It can
be increased later if the wiring is short and shielded.

ESP8266Audio and the embedded MP3/WAV arrays are no longer linked, so the
firmware is smaller than v2.0.

## Firmware images

```text
bin/v2.0.1/DetektorAir-v2.0.1-ST7735-factory.bin   flash at 0x0 (blank board)
bin/v2.0.1/DetektorAir-v2.0.1-ST7735-ota.bin       web OTA / ArduinoOTA
```

See [`../bin/README.md`](../bin/README.md) for all versions.

## Display init variants

If the ST7735 stays **white** (backlight on, no image) with the normal firmware,
try these OTA images. They are the same v2.0.1 firmware with the ST7735 SPI
clock lowered to **15 MHz** and a different init sequence:

| File | Init |
|---|---|
| [`../bin/v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-initb-15mhz-ota.bin`](../bin/v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-initb-15mhz-ota.bin) | `ST7735_INITB` |
| [`../bin/v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-redtab-15mhz-ota.bin`](../bin/v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-redtab-15mhz-ota.bin) | `ST7735_REDTAB` |
| [`../bin/v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-greentab-15mhz-ota.bin`](../bin/v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-greentab-15mhz-ota.bin) | `ST7735_GREENTAB` |

Upload them through the web OTA page and try INITB → REDTAB → GREENTAB.

If all three stay white, the problem is wiring, the FPC ribbon, or power, not
the firmware. Recheck `SCL=GPIO14`, `SDA=GPIO13`, `RST=GPIO25`, `DC=GPIO26`,
`CS=GPIO27`, `VDD=3.3 V`, `GND=GND`.

### Diagnostic test firmware

Flash this **factory** image at **0x0** over USB. It contains only a minimal
ST7735 test: 15 MHz, red → green → blue, then `ST7735 OK`.

[`../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-test-factory.bin`](../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-test-factory.bin)

If that stays white, try the **software-SPI** tests. They bypass TFT_eSPI and
the ESP32 HSPI peripheral completely:

- [`../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-SWSPI-greentab-factory.bin`](../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-SWSPI-greentab-factory.bin)
- [`../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-SWSPI-redtab-factory.bin`](../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-SWSPI-redtab-factory.bin)

If software SPI works but TFT_eSPI stays white, try the hardware-SPI tests at
**1 MHz with MISO=GPIO12**:

- [`../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-HWSPI-hspi-1mhz-factory.bin`](../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-HWSPI-hspi-1mhz-factory.bin)
- [`../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-HWSPI-vspi-1mhz-factory.bin`](../bin/v2.0.1/test/DetektorAir-v2.0.1-ST7735-HWSPI-vspi-1mhz-factory.bin)

Serial at 115200 should print:

```text
ST7735 TEST BOOT
TFT 160 x 128
ST7735 TEST DONE
```

or for SWSPI:

```text
SWSPI TEST BOOT
TFT 160 x 128
SWSPI TEST DONE
```

- Serial prints but the screen stays white → display, ribbon, soldering, or
  controller power problem.
- Screen cycles red/green/blue → the panel works; the issue is in the radar UI
  init/config.
- SWSPI works but TFT_eSPI does not → HSPI/pin configuration issue.
- No serial output → the test image did not boot; reflash at 0x0 and press EN.

## Notes

- The working ST7735 configuration is `ST7735_GREENTAB`, HSPI, **1 MHz**,
  `MISO=GPIO12`. Keep `platformio.ini` and `st7735_ui.h` in sync if you change
  it.
- The board can be a 30-pin or 38-pin ESP32-WROOM-32.
- The buzzer timing values are clamped to 50–60000 ms; alert hold is
  0–600000 ms.
- OTA leaves the NVS counter, buzzer timings and WiFi settings intact.
- The modern UI uses rounded tiles, a large distance readout, a pass counter
  and a 46-sample history sparkline.
