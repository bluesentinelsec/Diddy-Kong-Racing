#!/usr/bin/env python3
"""Generate the app icons: a red tile with a yellow trim and white "DKR".

Writes the iOS asset catalog icon (ios/Assets.xcassets) and the Android
launcher mipmaps (android/app/src/main/res/mipmap-*). The PNGs are committed,
so this only needs running when the design changes. Needs Pillow.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RED = (200, 16, 46)
YELLOW = (255, 194, 14)
WHITE = (255, 255, 255)

FONTS = [
    "/System/Library/Fonts/Supplemental/Arial Black.ttf",
    "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
]


def font(size):
    for path in FONTS:
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    sys.exit("make-icons: no bold TrueType font found; edit FONTS in tools/make-icons.py")


def icon(size):
    """Full-bleed tile: the platforms apply their own corner masks."""
    im = Image.new("RGBA", (size, size), RED + (255,))
    d = ImageDraw.Draw(im)
    trim = size * 0.055
    inset = size * 0.075
    d.rounded_rectangle([inset, inset, size - inset, size - inset], radius=size * 0.19,
                        outline=YELLOW, width=int(trim))
    f = font(int(size * 0.30))
    text = "DKR"
    box = d.textbbox((0, 0), text, font=f)
    w, h = box[2] - box[0], box[3] - box[1]
    x = (size - w) / 2 - box[0]
    y = (size - h) / 2 - box[1] - size * 0.01
    # A yellow shadow under the letters ties them to the trim.
    d.text((x + size * 0.012, y + size * 0.012), text, font=f, fill=YELLOW)
    d.text((x, y), text, font=f, fill=WHITE)
    return im


def main():
    ios = os.path.join(ROOT, "ios", "Assets.xcassets", "AppIcon.appiconset")
    os.makedirs(ios, exist_ok=True)
    icon(1024).convert("RGB").save(os.path.join(ios, "icon-1024.png"))
    android = os.path.join(ROOT, "android", "app", "src", "main", "res")
    for name, px in (("mdpi", 48), ("hdpi", 72), ("xhdpi", 96), ("xxhdpi", 144), ("xxxhdpi", 192)):
        out = os.path.join(android, f"mipmap-{name}")
        os.makedirs(out, exist_ok=True)
        icon(1024).resize((px, px), Image.LANCZOS).save(os.path.join(out, "ic_launcher.png"))
    print("icons written")


if __name__ == "__main__":
    main()
