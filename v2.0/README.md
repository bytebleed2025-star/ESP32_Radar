# v2.0 — HC-SR04 + ST7735 + MAX98357A + inductive proximity

Branch: `v2.0` (based on the ST7735 display branch)

## Hardware

| Function | ESP32 pin | Notes |
|---|---|---|
| HC-SR04 TRIG | GPIO 5 | 10 µs trigger |
| HC-SR04 ECHO | GPIO 18 | 1 kΩ / 2 kΩ divider to 3.3 V |
| ST7735 SCLK | GPIO 14 | 3.3 V logic |
| ST7735 MOSI | GPIO 13 | |
| ST7735 CS | GPIO 27 | |
| ST7735 DC | GPIO 26 | |
| ST7735 RST | GPIO 25 | |
| ST7735 BL | GPIO 32 | |
| MAX98357A BCLK | GPIO 19 | servo pin is free in v2.0 |
| MAX98357A LRC | GPIO 21 | |
| MAX98357A DIN | GPIO 22 | |
| MAX98357A VIN | 5 V | speaker amp supply |
| MAX98357A GND | GND | common ground |
| Inductive sensor output | GPIO 34 | NPN open collector, 10 kΩ pull-up to 3.3 V, 1 kΩ series |
| Inductive sensor supply | 6–36 V sensor rating | common GND required |

No servo is used. The HC-SR04 measures continuously in one fixed direction.

### NPN inductive sensor

Typical LJ12A3-4-Z/BX:

```text
brown  → sensor supply + (6–36 V)
blue   → GND (common with ESP32)
black  → GPIO 34
         10 kΩ from GPIO 34 to 3.3 V
         1 kΩ in series with the black wire
```

The sensor pulls the output low when metal is detected. ESP32 GPIO34 is
input-only and needs the external pull-up.

## Behaviour

- Measure distance at a fixed interval.
- Show distance and proximity count on the ST7735.
- Count each new inductive detection (debounced).
- Save the count in NVS so it survives reboot.
- Reset the counter from the web UI.
- Play a WAV/MP3 alert through MAX98357A on each detection.
- Web UI and OTA remain available.
- ST7735 background and web background will be replaced with the supplied
  artwork.

## Pending decisions

1. Audio storage: microSD card for MP3/WAV, or audio embedded in ESP32 flash?
2. Inductive sensor output: NO or NC?
3. Background artwork: ST7735 128×160 PNG + web background image.
