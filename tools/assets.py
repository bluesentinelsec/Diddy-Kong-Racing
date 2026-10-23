#!/usr/bin/env python3
"""The game data, as a tree of native asset files.

The port's assets come from your own Diddy Kong Racing ROM. This tool turns the
ROM's packed asset image into a folder of one file per asset (`unpack`), and a
folder back into the single image the game loads (`pack`):

    python3 tools/assets.py unpack [ROM]      ROM (baseroms/ by default) -> assets/tree/
    python3 tools/assets.py pack              assets/tree/ -> assets/assets.bin + assets/assets.lut.bin
    python3 tools/assets.py build [ROM]       unpack if needed, then pack (what `make assets` runs)
    python3 tools/assets.py verify            unpack(pack(tree)) must reproduce the tree

Every file holds the asset exactly as the game reads it: big-endian, the ROM's
own structures, nothing converted. The one change from the ROM is that assets
the ROM stores deflated (object models, animations, level models, level object
maps and some textures) are stored inflated, since that is what anyone editing
or replacing one needs. The game loads them inflated too.

Layout of assets/tree/:
    manifest.json             section order and table conventions (see SECTIONS)
    <section>/<NNNN>.bin      one file per entry of a section that has an offset table
    <section>.bin             a section without one, kept whole
A section's offset table is not stored; `pack` rebuilds it from the files.

Only the Python standard library is used. Byte-swapped .v64 / .n64 dumps are
converted first; the ROM is identified by SHA-1 and never copied anywhere.
"""
import hashlib
import json
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TREE = os.path.join(ROOT, "assets", "tree")
ASSETS_BIN = os.path.join(ROOT, "assets", "assets.bin")
ASSETS_LUT = os.path.join(ROOT, "assets", "assets.lut.bin")
MANIFEST_VERSION = 1
# Trailer appended to the lookup table of an image written by `pack`: the game
# and `unpack` read it to know the entries are stored inflated. A ROM's own
# lookup table has no trailer, so an image sliced straight from a ROM is read
# as compressed, exactly as before.
LUT_TRAILER = b"DKRASSET" + struct.pack(">II", 1, 1)  # magic, trailer version, flags (1 = inflated)

# sha1 of the big-endian ROM -> (name, lut offset, lut size, assets offset, assets size)
ROMS = {
    "6d96743d46f8c0cd0edb0ec5600b003c89b93755": ("US 1.1 (us.v80)", 0xED0E0, 0xD0, 0xED1B0, 0xB8CFD0 - 0xED1B0),
    "0cb115d8716dbbc2922fda38e533b9fe63bb9670": ("US 1.0 (us.v77)", 0xECB60, 0xD0, 0xECC30, 0xAC9630 - 0xECC30),
}
EXTENSIONS = (".z64", ".v64", ".n64")

# The 50 sections of the asset image, in the order of include/asset_enums.h.
# kind: "entries" (split by its offset table), "table" (the offset table of an
# entries section; regenerated on pack), "blob" (kept whole).
# unit: what one offset-table step means in bytes (the misc table counts words).
# compressed: entries are stored deflated in the ROM and inflated in the tree.
#   "rzip": 4-byte little-endian size, one level byte, raw deflate.
#   "texture": a texture whose header byte 0x1D says whether it is rzip'd after
#   its 0x20-byte header; the inflated image carries its own (clear) flag.
SECTIONS = [
    ("ai_behaviour", "entries", {"table": "ai_behaviour_table"}),
    ("ai_behaviour_table", "table", {}),
    ("textures_3d", "entries", {"table": "textures_3d_table", "compressed": "texture"}),
    ("textures_3d_table", "table", {}),
    ("textures_2d", "entries", {"table": "textures_2d_table", "compressed": "texture"}),
    ("textures_2d_table", "table", {}),
    ("game_text", "blob", {}),
    ("game_text_table", "blob", {}),
    ("menu_text", "blob", {}),
    ("menu_text_table", "blob", {}),
    ("screens", "blob", {}),
    ("screens_table", "blob", {}),
    ("sprites", "entries", {"table": "sprites_table"}),
    ("sprites_table", "table", {}),
    ("empty_14", "blob", {}),
    ("misc", "entries", {"table": "misc_table", "unit": 4}),
    ("misc_table", "table", {}),
    ("hud_element_ids", "blob", {}),
    ("menu_element_ids", "blob", {}),
    ("weather_particles", "blob", {}),
    ("level_object_maps_table", "table", {}),
    ("level_object_maps", "entries", {"table": "level_object_maps_table", "compressed": "rzip"}),
    ("level_headers_table", "table", {}),
    ("level_headers", "entries", {"table": "level_headers_table"}),
    ("level_names_table", "table", {}),
    ("level_names", "entries", {"table": "level_names_table"}),
    ("level_models_table", "table", {}),
    ("level_models", "entries", {"table": "level_models_table", "compressed": "rzip"}),
    ("object_models_table", "table", {}),
    ("object_models", "entries", {"table": "object_models_table", "compressed": "rzip"}),
    ("animation_ids", "blob", {}),
    ("object_animations_table", "table", {}),
    ("object_animations", "entries", {"table": "object_animations_table", "compressed": "rzip"}),
    ("object_headers_table", "table", {}),
    ("objects", "entries", {"table": "object_headers_table"}),
    ("level_object_translation_table", "blob", {}),
    ("empty_37_table", "blob", {}),
    ("empty_37", "blob", {}),
    ("audio_table", "blob", {}),
    ("audio", "blob", {}),
    ("particles_table", "table", {}),
    ("particles", "entries", {"table": "particles_table"}),
    ("particle_behaviors_table", "table", {}),
    ("particle_behaviors", "entries", {"table": "particle_behaviors_table"}),
    ("fonts", "blob", {}),
    ("japanese_fonts_table", "blob", {}),
    ("japanese_fonts", "blob", {}),
    ("dummy_particle_ids", "blob", {}),
    ("ttghosts_table", "blob", {}),
    ("ttghosts", "blob", {}),
]
SECTION_INDEX = {name: i for i, (name, _, _) in enumerate(SECTIONS)}
ALIGN = 16


def align(n):
    return (n + ALIGN - 1) & ~(ALIGN - 1)


# ---------------------------------------------------------------------------- ROM

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


def rom_candidates(arg):
    if arg:
        return [arg]
    found = []
    for d in (os.path.join(ROOT, "baseroms"), ROOT):
        if os.path.isdir(d):
            for f in sorted(os.listdir(d)):
                if f.lower().endswith(EXTENSIONS):
                    found.append(os.path.join(d, f))
    return found


def read_rom_image(arg):
    """Returns (rom name, lut bytes, asset image bytes) from a supported ROM."""
    for path in rom_candidates(arg):
        with open(path, "rb") as f:
            rom = to_big_endian(f.read())
        sha = hashlib.sha1(rom).hexdigest()
        if sha not in ROMS:
            print(f"skipping {path}: sha1 {sha} is not a supported ROM")
            continue
        name, lutOff, lutLen, binOff, binLen = ROMS[sha]
        print(f"ROM: {path} ({name})")
        return name, rom[lutOff:lutOff + lutLen], rom[binOff:binOff + binLen]
    sys.exit("assets: no supported ROM found. Put your US 1.1 .z64 in baseroms/ (or pass its path); "
             "it is identified by SHA-1 and never copied anywhere.")


# ---------------------------------------------------------------------------- image

def image_is_inflated(lut):
    return lut.endswith(LUT_TRAILER)


def split_image(lut, image):
    """The asset image as a list of section byte strings, in SECTIONS order."""
    count = struct.unpack(">I", lut[:4])[0]
    if count != len(SECTIONS):
        sys.exit(f"assets: expected {len(SECTIONS)} sections, the lookup table has {count}")
    offsets = struct.unpack(">%dI" % (count + 1), lut[4:4 + 4 * (count + 1)])
    return [image[offsets[i]:offsets[i + 1]] for i in range(count)]


def join_image(sections):
    """Sections -> (lut bytes, image bytes). Every section starts 16-aligned."""
    offsets = []
    image = bytearray()
    for data in sections:
        offsets.append(len(image))
        image += data
        image += bytes(align(len(image)) - len(image))
    offsets.append(len(image))
    lut = struct.pack(">I", len(sections)) + struct.pack(">%dI" % len(offsets), *offsets)
    lut += bytes(align(len(lut)) - len(lut)) + LUT_TRAILER
    return bytes(lut), bytes(image)


def parse_table(data):
    """An offset table: k non-decreasing offsets from 0, the end offset, then a
    tail (an all-ones terminator and padding). Returns (offsets incl. end, tail)."""
    words = list(struct.unpack(">%dI" % (len(data) // 4), data[:len(data) // 4 * 4]))
    m = 0
    while m + 1 < len(words) and words[m + 1] != 0xFFFFFFFF and words[m + 1] >= words[m]:
        m += 1
    return words[:m + 1], words[m + 1:]


def build_table(offsets, tail, min_words):
    """Offsets (incl. end) plus the original tail, padded with zeros to at least
    the original word count so the game's table scans see what they always did."""
    words = list(offsets) + list(tail)
    while len(words) < min_words:
        words.append(0)
    return struct.pack(">%dI" % len(words), *words)


# ---------------------------------------------------------------------------- compression

def rzip_inflate(data):
    """4-byte little-endian size, one level byte, raw deflate."""
    size = struct.unpack("<I", data[:4])[0]
    out = zlib.decompress(data[5:], -15)
    if len(out) != size:
        raise ValueError(f"inflated {len(out)} bytes, header says {size}")
    return out


def texture_inflate(data):
    """A texture entry: 0x20-byte header; byte 0x1D set means an rzip image follows."""
    if len(data) >= 0x25 and data[0x1D]:
        out = bytearray(rzip_inflate(data[0x20:]))
        if out[:0x1D] != data[:0x1D] or out[0x1E:0x20] != data[0x1E:0x20]:
            raise ValueError("inflated texture header differs from the outer header")
        out[0x1D] = 0  # the image is stored inflated now; the game only reads this byte from the file
        return bytes(out)
    return data


INFLATE = {"rzip": rzip_inflate, "texture": texture_inflate}


# ---------------------------------------------------------------------------- unpack / pack

def unpack(lut, image, tree=TREE):
    sections = split_image(lut, image)
    inflated = image_is_inflated(lut)
    manifest = {"version": MANIFEST_VERSION, "sections": []}
    os.makedirs(tree, exist_ok=True)
    for i, (name, kind, opts) in enumerate(SECTIONS):
        data = sections[i]
        entry = {"name": name, "kind": kind}
        if kind == "blob":
            with open(os.path.join(tree, name + ".bin"), "wb") as f:
                f.write(data)
            entry["size"] = len(data)
        elif kind == "entries":
            table = sections[SECTION_INDEX[opts["table"]]]
            offsets, tail = parse_table(table)
            unit = opts.get("unit", 1)
            inflate = None if inflated else INFLATE.get(opts.get("compressed"))
            folder = os.path.join(tree, name)
            os.makedirs(folder, exist_ok=True)
            for k in range(len(offsets) - 1):
                piece = data[offsets[k] * unit:offsets[k + 1] * unit]
                if inflate is not None and len(piece) > 0:
                    piece = inflate(piece)
                with open(os.path.join(folder, "%04d.bin" % k), "wb") as f:
                    f.write(piece)
            entry.update({"table": opts["table"], "count": len(offsets) - 1, "unit": unit,
                          "table_words": len(table) // 4, "table_tail": ["0x%X" % w for w in tail]})
            if "compressed" in opts:
                entry["compressed"] = opts["compressed"]
        manifest["sections"].append(entry)
    with open(os.path.join(tree, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)
        f.write("\n")
    return manifest


def read_manifest(tree=TREE):
    path = os.path.join(tree, "manifest.json")
    if not os.path.exists(path):
        sys.exit(f"assets: {path} not found; run `python3 tools/assets.py unpack` first")
    with open(path) as f:
        manifest = json.load(f)
    if manifest.get("version") != MANIFEST_VERSION:
        sys.exit("assets: manifest version mismatch; re-run unpack")
    return manifest


def entry_files(folder):
    """The NNNN.bin files of a section folder, in index order, with no gaps."""
    names = sorted(f for f in os.listdir(folder) if f.endswith(".bin"))
    for k, f in enumerate(names):
        if f != "%04d.bin" % k:
            sys.exit(f"assets: {folder} must hold 0000.bin, 0001.bin, ... without gaps (found {f})")
    return [os.path.join(folder, f) for f in names]


def pack(tree=TREE):
    """The tree -> (lut, image). Entry sections are rebuilt from their files and
    their offset tables regenerated; nothing is compressed."""
    manifest = read_manifest(tree)
    by_name = {e["name"]: e for e in manifest["sections"]}
    sections = [None] * len(SECTIONS)
    for i, (name, kind, opts) in enumerate(SECTIONS):
        if kind == "blob":
            with open(os.path.join(tree, name + ".bin"), "rb") as f:
                sections[i] = f.read()
        elif kind == "entries":
            unit = opts.get("unit", 1)
            data = bytearray()
            offsets = []
            for path in entry_files(os.path.join(tree, name)):
                with open(path, "rb") as f:
                    piece = f.read()
                if unit != 1 and len(data) % unit:
                    sys.exit(f"assets: {path}: entries of {name} must be a multiple of {unit} bytes")
                offsets.append(len(data) // unit)
                data += piece
            offsets.append(len(data) // unit)
            sections[i] = bytes(data)
            meta = by_name[name]
            tail = [int(w, 16) for w in meta["table_tail"]]
            sections[SECTION_INDEX[opts["table"]]] = build_table(offsets, tail, meta["table_words"])
    return join_image(sections)


def write_image(lut, image):
    os.makedirs(os.path.dirname(ASSETS_BIN), exist_ok=True)
    with open(ASSETS_LUT, "wb") as f:
        f.write(lut)
    with open(ASSETS_BIN, "wb") as f:
        f.write(image)
    print(f"wrote assets/assets.lut.bin ({len(lut)} bytes) and assets/assets.bin ({len(image)} bytes)")


def tree_digest(tree):
    h = hashlib.sha1()
    for dirpath, dirnames, filenames in os.walk(tree):
        dirnames.sort()
        for fn in sorted(filenames):
            if fn == "manifest.json":
                continue
            p = os.path.join(dirpath, fn)
            h.update(os.path.relpath(p, tree).encode())
            with open(p, "rb") as f:
                h.update(f.read())
    return h.hexdigest()


# ---------------------------------------------------------------------------- commands

def cmd_unpack(args):
    _, lut, image = read_rom_image(args[0] if args else None)
    manifest = unpack(lut, image)
    files = sum(e.get("count", 1) for e in manifest["sections"] if e["kind"] != "table")
    print(f"unpacked {files} files into assets/tree/")
    return 0


def cmd_pack(args):
    write_image(*pack())
    return 0


def cmd_build(args):
    if not os.path.exists(os.path.join(TREE, "manifest.json")):
        cmd_unpack(args)
    return cmd_pack(args)


def cmd_verify(args):
    import tempfile
    lut, image = pack()
    with tempfile.TemporaryDirectory() as tmp:
        unpack(lut, image, tmp)
        a, b = tree_digest(TREE), tree_digest(tmp)
    if a != b:
        sys.exit("assets: verify FAILED: unpacking the packed image does not reproduce assets/tree/")
    print(f"verify OK: assets/tree/ round-trips through a {len(image)}-byte image")
    return 0


COMMANDS = {"unpack": cmd_unpack, "pack": cmd_pack, "build": cmd_build, "verify": cmd_verify}


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
        print(__doc__)
        return 2
    return COMMANDS[sys.argv[1]](sys.argv[2:])


if __name__ == "__main__":
    sys.exit(main())
