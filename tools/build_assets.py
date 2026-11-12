#!/usr/bin/env python3
"""
Build firmware/ESP32_Radar/web_assets.h from the editable files in web/.

Usage:
    python tools/build_assets.py

Run this after editing web/index.html (or replacing web/logo.jpg), then
re-upload the sketch with the Arduino IDE. Python is only needed for this
build step; the generated header is already included in the project, so the
sketch compiles without Python.
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
WEB = ROOT / "web"
OUT = ROOT / "firmware" / "ESP32_Radar" / "web_assets.h"

RAW_DELIM = "LACAKIDX"


def main() -> int:
    html_path = WEB / "index.html"
    logo_path = WEB / "logo.jpg"
    bg_path = WEB / "bg.jpg"

    if not html_path.exists():
        print(f"ERROR: {html_path} not found", file=sys.stderr)
        return 1
    if not logo_path.exists():
        print(f"ERROR: {logo_path} not found", file=sys.stderr)
        return 1
    if not bg_path.exists():
        print(f"ERROR: {bg_path} not found", file=sys.stderr)
        return 1

    html = html_path.read_text(encoding="utf-8")
    logo = logo_path.read_bytes()
    bg = bg_path.read_bytes()

    html_bytes = html.encode("utf-8")
    if f'){RAW_DELIM}"' in html:
        print(
            f"ERROR: index.html contains the raw-string terminator ){RAW_DELIM}\"",
            file=sys.stderr,
        )
        return 1

    out = []
    out.append("// AUTO-GENERATED FILE - do not edit by hand.")
    out.append("// Source: web/index.html + web/logo.jpg + web/bg.jpg")
    out.append("// Regenerate with: python tools/build_assets.py")
    out.append("#pragma once")
    out.append("#include <pgmspace.h>")
    out.append("")
    out.append(f"static const unsigned int INDEX_HTML_LEN = {len(html_bytes)};")
    out.append(f'static const char INDEX_HTML[] PROGMEM = R"{RAW_DELIM}(')
    out.append(html.rstrip("\n"))
    out.append(f'){RAW_DELIM}";')
    out.append("")
    out.append(f"static const unsigned int LOGO_JPG_LEN = {len(logo)};")
    out.append("static const uint8_t LOGO_JPG[] PROGMEM = {")
    for offset in range(0, len(logo), 18):
        chunk = logo[offset : offset + 18]
        row = ", ".join(f"0x{byte:02X}" for byte in chunk)
        comma = "," if offset + 18 < len(logo) else ""
        out.append(f"  {row}{comma}")
    out.append("};")
    out.append("")
    out.append(f"static const unsigned int BG_JPG_LEN = {len(bg)};")
    out.append("static const uint8_t BG_JPG[] PROGMEM = {")
    for offset in range(0, len(bg), 18):
        chunk = bg[offset : offset + 18]
        row = ", ".join(f"0x{byte:02X}" for byte in chunk)
        comma = "," if offset + 18 < len(bg) else ""
        out.append(f"  {row}{comma}")
    out.append("};")
    out.append("")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(out), encoding="utf-8", newline="\n")

    print(f"Wrote {OUT}")
    print(f"  index.html : {len(html_bytes):,} bytes")
    print(f"  logo.jpg   : {len(logo):,} bytes")
    print(f"  bg.jpg     : {len(bg):,} bytes")
    print(f"  header     : {OUT.stat().st_size:,} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
