#!/usr/bin/env python3
"""
Remove the background from images and save transparent PNGs.

The heavy lifting is done by rembg (U2Net and friends). The first run
downloads the selected model into rembg's cache (for example
~/.rembg/models), so it needs internet access once.

Install:
    pip install "rembg[cpu]"

Examples:
    python tools/remove_bg.py photo.jpg
    python tools/remove_bg.py photo.jpg cutout.png
    python tools/remove_bg.py photos/ cutouts/
    python tools/remove_bg.py logo.png logo-clean.png --trim
    python tools/remove_bg.py portrait.jpg out.png --alpha-matting
    python tools/remove_bg.py product.jpg out.png --model isnet-general-use

Useful models:
    u2net               general purpose (default)
    u2netp              smaller/faster, slightly lower quality
    isnet-general-use   good all-round alternative
    isnet-anime         anime/illustration
    birefnet-general    high quality, slower
    silueta             small model
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

SUPPORTED = {".jpg", ".jpeg", ".png", ".webp", ".bmp", ".tif", ".tiff"}
DEFAULT_MODEL = "u2net"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Remove the background from an image (or a folder of images).",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python tools/remove_bg.py photo.jpg\n"
            "  python tools/remove_bg.py photo.jpg cutout.png\n"
            "  python tools/remove_bg.py photos/ cutouts/\n"
            "  python tools/remove_bg.py logo.png logo-clean.png --trim\n"
        ),
    )
    parser.add_argument("input", type=Path, help="image file or folder")
    parser.add_argument(
        "output",
        nargs="?",
        type=Path,
        help="output .png file or folder (default: <name>-no-bg.png)",
    )
    parser.add_argument(
        "--model",
        default=DEFAULT_MODEL,
        help=f"rembg model to use (default: {DEFAULT_MODEL})",
    )
    parser.add_argument(
        "--alpha-matting",
        action="store_true",
        help="soften edges (hair, fur); slower",
    )
    parser.add_argument(
        "--fg-threshold",
        type=int,
        default=240,
        metavar="N",
        help="alpha-matting foreground threshold (default: 240)",
    )
    parser.add_argument(
        "--bg-threshold",
        type=int,
        default=10,
        metavar="N",
        help="alpha-matting background threshold (default: 10)",
    )
    parser.add_argument(
        "--erode",
        type=int,
        default=10,
        metavar="N",
        help="alpha-matting edge erode size (default: 10)",
    )
    parser.add_argument(
        "--post-process",
        action="store_true",
        help="clean up the generated mask",
    )
    parser.add_argument(
        "--trim",
        action="store_true",
        help="crop transparent borders after removal",
    )
    parser.add_argument(
        "--trim-pad",
        type=int,
        default=0,
        metavar="PX",
        help="pixels of padding kept by --trim (default: 0)",
    )
    parser.add_argument(
        "--bg",
        default="transparent",
        metavar="COLOR",
        help='fill the background with a color instead of transparency '
             '(e.g. "#ffffff", "black", "transparent")',
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="replace output files that already exist",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="only print errors and the final summary",
    )
    return parser.parse_args(argv)


def load_dependencies():
    try:
        from PIL import Image, ImageColor
    except ImportError as exc:
        raise SystemExit(
            "ERROR: Pillow is required.\n"
            'Install it with: pip install "rembg[cpu]"'
        ) from exc

    try:
        from rembg import new_session, remove
    except ImportError as exc:
        raise SystemExit(
            "ERROR: rembg is not installed.\n"
            "Install it with:\n"
            '    pip install "rembg[cpu]"'
        ) from exc

    return Image, ImageColor, new_session, remove


def collect_jobs(source: Path, output: Path | None) -> list[tuple[Path, Path]]:
    if source.is_dir():
        images = sorted(
            p
            for p in source.iterdir()
            if p.is_file() and p.suffix.lower() in SUPPORTED
        )
        if not images:
            raise SystemExit(f"ERROR: no supported images in {source}")

        out_dir = output if output is not None else source.with_name(
            source.name + "-no-bg"
        )
        if out_dir.exists() and not out_dir.is_dir():
            raise SystemExit(f"ERROR: output is not a folder: {out_dir}")
        return [(p, out_dir / f"{p.stem}.png") for p in images]

    if not source.is_file():
        raise SystemExit(f"ERROR: file not found: {source}")
    if source.suffix.lower() not in SUPPORTED:
        raise SystemExit(
            f"ERROR: unsupported input format {source.suffix!r}; "
            f"expected one of {', '.join(sorted(SUPPORTED))}"
        )

    if output is None:
        dst = source.with_name(f"{source.stem}-no-bg.png")
    elif output.exists() and output.is_dir():
        dst = output / f"{source.stem}.png"
    elif output.suffix:
        if output.suffix.lower() != ".png":
            raise SystemExit(
                "ERROR: output must be a .png file or a folder "
                "(transparency needs PNG)"
            )
        dst = output
    else:
        if output.exists() and not output.is_dir():
            raise SystemExit(f"ERROR: output is not a folder: {output}")
        dst = output / f"{source.stem}.png"

    return [(source, dst)]


def parse_background(value: str, ImageColor):
    text = value.strip()
    if text.lower() in {"", "transparent", "none"}:
        return None
    try:
        color = ImageColor.getrgb(text)
    except ValueError as exc:
        raise SystemExit(f"ERROR: invalid --bg color: {value!r}") from exc
    if len(color) == 4:
        return color
    return (*color, 255)


def process_image(
    src: Path,
    dst: Path,
    remove,
    session,
    args: argparse.Namespace,
    Image,
    bg_color,
) -> None:
    with Image.open(src) as opened:
        image = opened.convert("RGBA")

    result = remove(
        image,
        session=session,
        alpha_matting=args.alpha_matting,
        alpha_matting_foreground_threshold=args.fg_threshold,
        alpha_matting_background_threshold=args.bg_threshold,
        alpha_matting_erode_size=args.erode,
        post_process_mask=args.post_process,
    )

    if not isinstance(result, Image.Image):
        result = Image.fromarray(result)

    if args.trim:
        bbox = result.getchannel("A").getbbox()
        if bbox:
            pad = max(0, args.trim_pad)
            left, top, right, bottom = bbox
            result = result.crop(
                (
                    max(0, left - pad),
                    max(0, top - pad),
                    min(result.width, right + pad),
                    min(result.height, bottom + pad),
                )
            )

    if bg_color is not None:
        canvas = Image.new("RGBA", result.size, bg_color)
        canvas.alpha_composite(result)
        result = canvas

    dst.parent.mkdir(parents=True, exist_ok=True)
    result.save(dst, format="PNG", optimize=True)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    Image, ImageColor, new_session, remove = load_dependencies()

    source = args.input.expanduser().resolve()
    output = args.output.expanduser().resolve() if args.output else None
    jobs = collect_jobs(source, output)
    bg_color = parse_background(args.bg, ImageColor)

    try:
        session = new_session(args.model)
    except Exception as exc:
        raise SystemExit(
            f"ERROR: could not load rembg model {args.model!r}: {exc}\n"
            'If this is an install problem, run: pip install "rembg[cpu]"'
        ) from exc

    processed = 0
    skipped = 0
    failed = 0

    for src, dst in jobs:
        if dst.exists() and not args.overwrite:
            print(f"skip (exists): {dst}")
            skipped += 1
            continue

        try:
            process_image(
                src, dst, remove, session, args, Image, bg_color
            )
        except Exception as exc:
            print(f"FAILED {src}: {exc}", file=sys.stderr)
            failed += 1
            continue

        processed += 1
        if not args.quiet:
            print(f"{src} -> {dst}")

    print(
        f"Done: {processed} processed, {skipped} skipped, "
        f"{failed} failed"
    )

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
