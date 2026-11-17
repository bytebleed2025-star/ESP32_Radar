#!/usr/bin/env python3
"""Build firmware/ESP32_Radar/st7735_images.h from the source artwork.

TJpg_Decoder only supports *baseline* JPEGs. The artwork in
Reference Image/ST7735_RefImageNew is progressive, so it is re-encoded here
as baseline 160x128 (the ST7735 landscape resolution).

Usage:
    python tools/build_st7735_images.py
"""

from io import BytesIO
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Reference Image" / "ST7735_RefImageNew"
OUT = ROOT / "firmware" / "ESP32_Radar" / "st7735_images.h"

WIDTH = 160
HEIGHT = 128
QUALITY = 88
PER_LINE = 18

IMAGES = [
    ("ST7735_WELCOME_JPG", "WelcomeScreen.jpg"),
    ("ST7735_SCAN0_JPG", "RadarScan_00.jpg"),
    ("ST7735_SCAN1_JPG", "RadarScan_01.jpg"),
    ("ST7735_SCAN2_JPG", "RadarScan_02.jpg"),
    ("ST7735_SCAN3_JPG", "RadarScan_03.jpg"),
]


def encode_baseline(path: Path) -> bytes:
    image = Image.open(path).convert("RGB")
    if image.size != (WIDTH, HEIGHT):
        image = image.resize((WIDTH, HEIGHT), Image.LANCZOS)
    buffer = BytesIO()
    image.save(
        buffer,
        format="JPEG",
        quality=QUALITY,
        subsampling=2,
        optimize=True,
        progressive=False,
    )
    return buffer.getvalue()


def format_array(name: str, data: bytes) -> list[str]:
    out = [f"static const unsigned int {name}_LEN = {len(data)};"]
    out.append(f"static const uint8_t {name}[] PROGMEM = {{")
    for start in range(0, len(data), PER_LINE):
        chunk = data[start:start + PER_LINE]
        body = ", ".join(f"0x{b:02X}" for b in chunk)
        if start + PER_LINE < len(data):
            body += ","
        out.append("  " + body)
    out.append("};")
    return out


def main() -> int:
    lines = [
        "// Auto-generated ST7735 JPEG images (welcome + scan backgrounds)",
        "// Regenerate with: python tools/build_st7735_images.py",
        "#pragma once",
        "#include <Arduino.h>",
        "",
    ]

    for name, filename in IMAGES:
        path = SRC / filename
        if not path.exists():
            print(f"ERROR: {path} not found")
            return 1
        data = encode_baseline(path)
        if data[:2] != b"\xFF\xD8":
            print(f"ERROR: {filename} did not encode as JPEG")
            return 1
        # Reject an accidental progressive encode.
        if b"\xFF\xC2" in data[:4096]:
            print(f"ERROR: {filename} encoded as progressive JPEG")
            return 1
        lines.extend(format_array(name, data))
        lines.append("")
        print(f"{filename:20s} {len(data):6d} bytes  baseline {WIDTH}x{HEIGHT}")

    OUT.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"Wrote {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
