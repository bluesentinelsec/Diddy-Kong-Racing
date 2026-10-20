#!/usr/bin/env python3
"""Produce the game data the port loads from your own Diddy Kong Racing ROM.

The two files, assets/assets.bin (the packed asset table) and
assets/assets.lut.bin (its lookup table), sit in the ROM exactly as the game
reads them, so this script verifies the ROM by SHA-1 and copies the two byte
ranges out. No other tooling is needed.

    python3 tools/extract-assets.py                 # finds the ROM in baseroms/ or the repo root
    python3 tools/extract-assets.py /path/to/rom.z64

Byte-swapped .v64 / .n64 dumps are recognised and converted first. Supported
ROMs are listed in ROMS; US 1.1 is the revision the port is built for.
"""
import hashlib
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# sha1 of the big-endian ROM -> (name, lut offset, lut size, assets offset, assets size)
ROMS = {
    "6d96743d46f8c0cd0edb0ec5600b003c89b93755": ("US 1.1 (us.v80)", 0xED0E0, 0xD0, 0xED1B0, 0xB8CFD0 - 0xED1B0),
    "0cb115d8716dbbc2922fda38e533b9fe63bb9670": ("US 1.0 (us.v77)", 0xECB60, 0xD0, 0xECC30, 0xAC9630 - 0xECC30),
}
EXTENSIONS = (".z64", ".v64", ".n64")


def to_big_endian(data):
    """N64 dumps come in three byte orders; the first word says which."""
    magic = data[:4]
    if magic == b"\x80\x37\x12\x40":
        return data
    if magic == b"\x37\x80\x40\x12":  # .v64: 16-bit swapped
        out = bytearray(data)
        out[0::2], out[1::2] = data[1::2], data[0::2]
        return bytes(out)
    if magic == b"\x40\x12\x37\x80":  # .n64: 32-bit swapped
        out = bytearray(data)
        out[0::4], out[1::4], out[2::4], out[3::4] = data[3::4], data[2::4], data[1::4], data[0::4]
        return bytes(out)
    return data


def candidates(arg):
    if arg:
        return [arg]
    found = []
    for d in (os.path.join(ROOT, "baseroms"), ROOT):
        if os.path.isdir(d):
            for f in sorted(os.listdir(d)):
                if f.lower().endswith(EXTENSIONS):
                    found.append(os.path.join(d, f))
    return found


def main():
    arg = sys.argv[1] if len(sys.argv) > 1 else None
    for path in candidates(arg):
        with open(path, "rb") as f:
            rom = to_big_endian(f.read())
        sha = hashlib.sha1(rom).hexdigest()
        if sha not in ROMS:
            print(f"skipping {path}: sha1 {sha} is not a supported ROM")
            continue
        name, lutOff, lutLen, binOff, binLen = ROMS[sha]
        out = os.path.join(ROOT, "assets")
        os.makedirs(out, exist_ok=True)
        with open(os.path.join(out, "assets.lut.bin"), "wb") as f:
            f.write(rom[lutOff:lutOff + lutLen])
        with open(os.path.join(out, "assets.bin"), "wb") as f:
            f.write(rom[binOff:binOff + binLen])
        print(f"ROM: {path} ({name})")
        print(f"wrote assets/assets.lut.bin ({lutLen} bytes) and assets/assets.bin ({binLen} bytes)")
        return 0
    sys.exit("extract-assets: no supported ROM found. Put your US 1.1 .z64 in baseroms/ "
             "(or pass its path); it is identified by SHA-1 and never copied anywhere.")


if __name__ == "__main__":
    sys.exit(main())
