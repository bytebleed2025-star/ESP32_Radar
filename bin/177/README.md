# 1.77" TFT (128×160, ST7735) build

Same hybrid firmware as `v2.0.3` — WiFi AP+STA, web UI/OTA and Bluetooth
SPP — with the mirror radar screen for the **1.77" 128×160 ST7735 SPI**
module (the one with `GND VCC SCK SDA RES RS CS` on its 7-pin header).

## Wiring

| 1.77" pin | ESP32 |
|---|---|
| GND (1) | GND |
| VCC (2) | 3.3 V |
| SCK (3) | GPIO14 |
| SDA (4) | GPIO13 (MOSI) |
| RES (5) | GPIO25 |
| RS (6) | GPIO26 (DC) |
| CS (7) | GPIO27 |

The `LEDA` pins and `GT_*` (touch) pins are not used. Backlight is on when the
module is powered.

HC-SR04, buzzer and SN04 keep their pins (5, 18, 23, 34, 33).

## Build

```bash
pio run -e esp32dev-177
```

## If the image does not fit

These panels differ only in the controller RAM offset. Two builds are
provided:

| Build | TAB | Use when |
|---|---|---|
| `...-177-00` | REDTAB (0,0) | strip/garbage on the **left/top** edge, content clipped on the right/bottom |
| `...-177-21` | GREENTAB (2,1) | strip/garbage on the **right/bottom** edge, content clipped on the left/top |

Flash one, look at the edge, and keep the one with a clean border.

If the image is **rotated 90°**, tell me and I’ll switch the rotation.
