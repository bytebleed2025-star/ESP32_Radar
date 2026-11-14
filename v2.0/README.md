# v2.0 — HC-SR04 + ST7735 + MAX98357A + inductive proximity

Branch: `v2.0` (based on the ST7735 display branch)

## Download & flash

Prebuilt **v2.0 ST7735** firmware (`FW_VERSION 2.0.0`):

| File | Use |
|---|---|
| [`bin/DetektorAir-v2.0-ST7735-factory.bin`](bin/DetektorAir-v2.0-ST7735-factory.bin) | new / blank board, flash at **0x0** |
| [`bin/DetektorAir-v2.0-ST7735-ota.bin`](bin/DetektorAir-v2.0-ST7735-ota.bin) | web **CONFIG → UPDATE FIRMWARE** or ArduinoOTA |

Blank board over USB:

```cmd
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\DetektorAir-v2.0-ST7735-factory.bin
```

Existing v2.0 board: upload `bin/DetektorAir-v2.0-ST7735-ota.bin` from the web
CONFIG tab (or use ArduinoOTA on port 3232, password `lacakota`).

Checksums: [`bin/SHA256SUMS.txt`](bin/SHA256SUMS.txt).

> The older root files `DetektorAir-ST7735-factory.bin` and
> `DetektorAir-ST7735-ota.bin` are from the **servo sweep** ST7735 branch.
> Do not use them for v2.0.

## Renders

| ST7735 boot | ST7735 idle | ST7735 detected |
|---|---|---|
| ![](st7735-boot.png) | ![](st7735-idle.png) | ![](st7735-detect.png) |

![Web UI desktop](web-desktop.png)

![Web UI mobile](web-mobile.png)

## Wiring (exact)

> **Common ground is mandatory.** Connect the ESP32 GND, HC-SR04 GND, ST7735
> GND, MAX98357A GND, 5 V supply 0 V and inductive-sensor 0 V together.

### Power

Recommended supply: **5 V, at least 2 A** (WiFi peaks + audio transients).

| From | To | Note |
|---|---|---|
| 5 V PSU `+` | ESP32 `5V` / `VIN` | do not use the 3.3 V pin |
| 5 V PSU `+` | HC-SR04 `VCC` | HC-SR04 is a 5 V module |
| 5 V PSU `+` | MAX98357A `VIN` | amplifier supply |
| 5 V PSU `−` | common `GND` | star ground |
| ESP32 `3V3` | ST7735 `VCC` | display logic is 3.3 V |
| ESP32 `3V3` | MAX98357A `SD` | enables the amplifier |
| ESP32 `3V3` | 10 kΩ pull-up | inductive sensor output |

Add **470 µF + 100 nF** across 5 V/GND near the MAX98357A. Do **not** power the
HC-SR04 or MAX98357A from the ESP32 3.3 V regulator.

> If you power the ESP32 through its `5V` pin, disconnect USB first (or use USB
> as the only 5 V source) on boards that do not have a power-path diode.

### ESP32 board (38-pin DevKit)

This build targets a standard **ESP32 DevKit 38-pin** board (ESP32-WROOM-32,
4 MB flash) **without a microSD slot**. Use the **GPIO labels printed on the
board**, not the physical pin position, because 30-pin and 38-pin boards place
the same GPIOs differently.

- `5V` / `VIN` → 5 V supply; `3V3` → ST7735 VCC; `GND` → common ground.
- GPIO 34 is input-only (inductive sensor). GPIO 35/36/39 are also input-only.
- GPIO 6–11 are connected to the SPI flash and must not be used.
- GPIO 5 is a strapping pin but is used for HC-SR04 `TRIG`; it is
  high-impedance at boot, so it is normally safe.
- Some 38-pin boards have a **microSD slot** wired to GPIO 2/4/12/13/14/15.
  This build's board has **no SD slot**, so GPIO 13/14 are free for the ST7735.
  If you use a different 38-pin board that does have the slot, remove any SD
  card and verify that the display initialises; otherwise remap the TFT pins
  and rebuild.
- The onboard LED is usually GPIO 2; it is not used.

### Complete pin map

| Module | Module pin | Connects to | Wire / note |
|---|---|---|---|
| **HC-SR04** | `VCC` | 5 V | |
| | `TRIG` | **GPIO 5** | direct, 3.3 V trigger |
| | `ECHO` | **GPIO 18** | through 1 kΩ / 2 kΩ divider |
| | `GND` | GND | |
| **ST7735** | `VCC` | 3.3 V | |
| | `GND` | GND | |
| | `SCL` / `SCLK` | **GPIO 14** | SPI clock |
| | `SDA` / `MOSI` | **GPIO 13** | SPI data |
| | `CS` | **GPIO 27** | |
| | `DC` / `A0` | **GPIO 26** | |
| | `RES` / `RST` | **GPIO 25** | |
| | `LED` / `BL` | **GPIO 32** | HIGH = backlight on |
| **MAX98357A** | `VIN` | 5 V | |
| | `GND` | GND | |
| | `BCLK` | **GPIO 19** | I2S bit clock |
| | `LRC` / `LRCLK` | **GPIO 21** | I2S word clock |
| | `DIN` | **GPIO 22** | I2S data |
| | `SD` | 3.3 V | amplifier enable |
| | `GAIN` | leave open | default 9 dB |
| | `OUT+` | speaker `+` | 4 Ω or 8 Ω |
| | `OUT−` | speaker `−` | never to GND |
| **Inductive sensor** | brown | +6–36 V sensor supply | not ESP32 5 V unless rated |
| | blue | 0 V / GND | common with ESP32 |
| | black | **GPIO 34** via 1 kΩ | NPN NO output |
| **Pull-up** | 10 kΩ | GPIO 34 → 3.3 V | mandatory |
| **Reset button** | leg 1 | **GPIO 33** | |
| | leg 2 | GND | momentary NO to GND |

### HC-SR04

```text
HC-SR04 VCC  ─────────────── 5 V
HC-SR04 GND  ─────────────── GND
HC-SR04 TRIG ─────────────── GPIO 5
HC-SR04 ECHO ── 1 kΩ ──┬──── GPIO 18
                        └──── 2 kΩ ──── GND
```

- `ECHO` is a **5 V** output. The 1 kΩ / 2 kΩ divider gives about **3.3 V** at
  GPIO 18. Never connect ECHO directly to the ESP32.
- `TRIG` accepts the ESP32 3.3 V pulse.
- GPIO 5 is a boot strapping pin. It works with the HC-SR04 `TRIG` input
  (high impedance), but if the board has trouble booting, move `TRIG` to
  GPIO 4 and rebuild.
- Range 2–400 cm. The HC-SR04 is **not waterproof**; keep the transducer dry.
- Aim the sensor at the contact/water surface and keep the cone clear.

### ST7735 display

```text
ST7735 VCC ───── 3.3 V
ST7735 GND ───── GND
ST7735 SCL ───── GPIO 14
ST7735 SDA ───── GPIO 13
ST7735 CS  ───── GPIO 27
ST7735 DC  ───── GPIO 26
ST7735 RES ───── GPIO 25
ST7735 LED ───── GPIO 32
```

- Use the **128×160 SPI, no-touch** module. MISO, touch and microSD are unused.
- Firmware is configured as **160×128 landscape**, `ST7735_INITB`,
  `TFT_RGB_ORDER = TFT_BGR`, rotation `1`.
- `LED`/`BL` is driven HIGH by GPIO 32. If the module's backlight draws more
  than about 12 mA, drive it with a 2N2222/MOSFET instead of the GPIO directly.
- If colours are swapped or the image is offset, change `ST7735_INITB` /
  `TFT_RGB_ORDER` / `ST7735_ROTATION` in `firmware/ESP32_Radar/st7735_ui.h`.

#### Confirmed 1.8" ST7735S module

This is the module specified for v2.0: 1.8 inch, 128×160, SPI, ST7735S,
3.3 V, 12 o'clock viewing direction.

| Header pin | Label | Connect to |
|---|---|---|
| 1 | `GND` | GND |
| 2 | `VCC` | **3.3 V** |
| 3 | `SCL` | **GPIO 14** |
| 4 | `SDA` | **GPIO 13** |
| 5 | `RES` | **GPIO 25** |
| 6 | `DC` | **GPIO 26** |
| 7 | `CS` | **GPIO 27** |
| 8 | `BL` | **GPIO 32** |

`BL` is the backlight-control pin. If the panel stays dark while GPIO 32 is
HIGH, the module's `BL` is active-low: set `TFT_BACKLIGHT_ON` to `LOW` in
`platformio.ini` and `st7735_ui.h`.

#### Module compatibility

This build targets the common **1.8 inch, 128×160, SPI, non-touch ST7735**
module (ST7735S / ST7735R, 8 pins).

- Native resolution is 128×160. The firmware draws **160×128 landscape**
  (`ST7735_ROTATION 1`).
- The standard 8-pin module is `VCC GND SCL SDA RES DC CS BL/LED` and matches
  the pin map above exactly.
- **Not** compatible with 1.44" 128×128, 0.96"/1.14" 80×160, 135×240, or
  160×80 ST7735 modules; the layout is fixed at 160×128.
- Touch or microSD variants are fine: leave `T_CLK`, `T_CS`, `T_DIN`, `T_DO`,
  `T_IRQ` and the SD-card pins unconnected. The firmware uses neither.
- The blue 8-pin **ST7735S** board labeled
  `VCC GND SCL SDA RST DC CS BLK` matches this build. Because it is an
  ST7735S (7735R family), if the default `ST7735_INITB` gives a blank, offset
  or odd-colour image, try `ST7735_REDTAB` first, then `ST7735_GREENTAB`.
- Some boards use `BLK` as the backlight **cathode**. If the screen stays dark
  with GPIO 32 HIGH, set `TFT_BACKLIGHT_ON` to `LOW`, or tie `BLK` to GND.
- If the image is shifted by 1–3 px or the colours are wrong, select the init
  that matches your module in **both** `platformio.ini` and `st7735_ui.h`:
  - `ST7735_INITB` — current default (7735B-style 128×160)
  - `ST7735_GREENTAB` — 128×160, offset 2/1
  - `ST7735_GREENTAB2` — 128×160, offset 2/1, different MADCTL
  - `ST7735_GREENTAB3` — 128×160, offset 2/3
  - `ST7735_REDTAB` — 128×160 red-tab
- To swap red/blue, change `TFT_RGB_ORDER=TFT_BGR` to `TFT_RGB`.
- If the image is upside down, change `ST7735_ROTATION` from `1` to `3`.

### MAX98357A audio

```text
MAX98357A VIN  ───── 5 V
MAX98357A GND  ───── GND
MAX98357A BCLK ───── GPIO 19
MAX98357A LRC  ───── GPIO 21
MAX98357A DIN  ───── GPIO 22
MAX98357A SD   ───── 3.3 V
MAX98357A GAIN ───── leave open (9 dB)
MAX98357A OUT+ ───── speaker +
MAX98357A OUT− ───── speaker −
```

- Use a **4 Ω or 8 Ω speaker rated 3 W or more**.
- Never connect either speaker terminal to GND; the outputs are bridged.
- `SD` must be HIGH or the amplifier stays in shutdown.
- The firmware plays the embedded **MP3** alert on every inductive detection.
  To use the embedded WAV instead, set `PROX_AUDIO_USE_MP3 0` in
  `firmware/ESP32_Radar/audio_player.h`.
- Audio pins are set by `g_out->SetPinout(19, 21, 22);` in the same file.

### LJ12A3-4-Z/BX inductive sensor (NPN NO)

```text
brown ───── +6…36 V sensor supply
blue  ───── 0 V / GND (common with ESP32)
black ── 1 kΩ ──┬──── GPIO 34
                 ├──── 10 kΩ ──── 3.3 V
                 └──── 100 nF ─── GND   (optional noise filter)
```

- The NPN output is **HIGH when idle** and pulls **LOW** when metal is
  detected. This matches `INPUT` + external pull-up and NPN-NO logic.
- GPIO 34 is **input-only** and has no usable internal pull-up: the external
  **10 kΩ to 3.3 V is required**.
- Do **not** connect the black wire directly to the ESP32. It must go through
  the **1 kΩ series resistor**, with the **10 kΩ pull-up to 3.3 V** at the
  GPIO 34 node. Never feed the 6–36 V sensor supply into the ESP32.
- If the sensor uses a separate supply, its 0 V must still connect to ESP32
  GND.
- Sensing distance is about **4 mm** (adjustable with the rear screw); the
  target must be metal.
- Firmware counts each new detection with **80 ms debounce** and saves the
  count in NVS.

### Reset button

```text
GPIO 33 ──── momentary NO button ──── GND
```

- Internal pull-up is enabled in firmware; no external resistor is needed.
- Press for at least **40 ms**. This resets only the **proximity counter**, not
  the WiFi settings.

### Pin restrictions

| Pin | Reason |
|---|---|
| GPIO 6–11 | connected to SPI flash on most ESP32 modules |
| GPIO 23 | legacy buzzer assignment, not used in v2.0 |
| GPIO 19 / 21 / 22 | used by MAX98357A I2S audio |
| GPIO 34 / 35 / 36 / 39 | input-only, no internal pull-up; GPIO 34 is the sensor input |
| GPIO 5 | boot strapping pin; used by HC-SR04 TRIG (see above) |

There is **no servo** in v2.0. The HC-SR04 measures continuously in one fixed
direction.

### Where the pins are defined

| Signal | File |
|---|---|
| HC-SR04 TRIG / ECHO | `PIN_TRIG`, `PIN_ECHO` in `ESP32_Radar.ino` |
| ST7735 SPI + backlight | `platformio.ini` build flags **and** `st7735_ui.h` |
| MAX98357A I2S | `SetPinout(19, 21, 22)` in `audio_player.h` |
| Inductive sensor / button | `PIN_PROX`, `PIN_PROX_RESET` in `proximity.h` |

> When changing ST7735 pins under PlatformIO, update **both** `platformio.ini`
> and `st7735_ui.h` so the PlatformIO and Arduino IDE builds stay identical.

## Behaviour

- HC-SR04 measures continuously in one fixed direction.
- ST7735 shows the boot logo for 1.5 s, then distance and proximity count.
- The provided droplet artwork is an outline normally and fills with a green
  gradient when a contact is inside the alert threshold.
- Each new inductive detection increments and saves the counter in NVS.
- Reset the counter with the **physical button on GPIO33** or the
  **RESET COUNTER** button in the web UI.
- Each detection plays the embedded **MP3** alert through the MAX98357A.
  A WAV version is embedded too; switch with `PROX_AUDIO_USE_MP3`.
- Web UI and OTA remain available.
- Web background uses `web/bg.jpg`; the header logo remains `web/logo.jpg`.

## Audio files

```text
firmware/ESP32_Radar/audio_prox.h   embedded MP3 + WAV arrays
firmware/ESP32_Radar/prox_alert.mp3 reference MP3
firmware/ESP32_Radar/prox_alert.wav reference WAV
```

ESP8266Audio is GPL-3.0; the firmware/source must be distributed together.

## Build result

- PlatformIO: **1,274,177 bytes (64.8%)**, RAM **51,336 (15.7%)**
- Arduino: **1,460,491 bytes (74%)**, RAM **54,136 (16%)**
- Arduino IDE needs TFT_eSPI and ESP8266Audio 1.9.9 installed.
