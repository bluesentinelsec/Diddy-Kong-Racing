# Diddy Kong Racing - PC port

A native port of Diddy Kong Racing (Nintendo 64, 1997) to Windows, macOS and
Linux, built from the [community decompilation](https://github.com/DavidSM64/Diddy-Kong-Racing)
by way of [Bruceleeto's PC/Dreamcast port](https://github.com/Bruceleeto/Diddy-Kong-Racing).
The game logic is the decompiled C; this repository adds the host layer: an
interpreter for the F3DDKR display lists, a shader renderer (OpenGL 3.3 core or
OpenGL ES 3.0), an interpreter for the RSP audio microcode, and SDL3 for the
window, input and sound.

It runs as a single self-contained executable on 64-bit and 32-bit hosts,
including the Raspberry Pi 5.

**No game data is in this repository.** You need your own copy of the US 1.1
ROM to produce the asset files the executable loads (see *Assets*).

## Building

Requirements: CMake 3.24+, a C compiler (clang, GCC or MinGW-w64), git, and on
Linux the usual SDL build dependencies (X11, Wayland, ALSA/PulseAudio
development packages; see `.github/workflows/pc.yml` for the exact list). SDL3
itself is fetched and built automatically and linked statically.

```
make            # debug build  -> build/debug/dkracing
make release    # optimized    -> build/release/dkracing
```

Other targets:

| Target        | What it does                                                          |
| ------------- | --------------------------------------------------------------------- |
| `make run`    | debug build, then run windowed (`RUN_ARGS` to change the flags)       |
| `make run-debugger` | the same under lldb (macOS) or gdb (Linux); a crash prints a backtrace and leaves you at the debugger prompt |
| `make test`   | build and run the unit tests with ctest                               |
| `make fmt`    | clang-format the port's own sources (the decomp keeps its formatting) |
| `make doc`    | Doxygen HTML documentation into `docs/html`                           |
| `make assets` | extract and pack the game assets from the ROM in `baseroms/`          |
| `make rom`    | build the N64 ROM from this tree and check it is byte-identical       |
| `make golden` | run the game deterministically and compare it with the golden trace   |
| `make clean`  | remove all build output                                               |

The Makefile is a thin wrapper; the build itself is CMake, so this works too:

```
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
```

CMake options: `DKR_VERSION` (`us_v80`, the default, or `us_v77`),
`DKR_SDL3_TAG` (the SDL release to build), `DKR_SANITIZE=ON` for an
AddressSanitizer build, `DKR_BUILD_TESTS=ON` for the test executables,
`DKR_EEPROM_PRESET_100=ON` to start new save files 100% complete.

Cross-building a 32-bit binary on a 64-bit Raspberry Pi:
`cmake -S . -B build/pi -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armhf.cmake`.

## Assets

The executable loads two files, `assets/assets.bin` and `assets/assets.lut.bin`,
from the working directory or from the directory it lives in. They are produced
from your ROM by the decompilation's tooling, which needs Python 3 and the MIPS
binutils (`binutils-mips-linux-gnu` on Debian, see `Makefile.n64`):

```
cp /path/to/your/rom.z64 baseroms/
make assets
```

This writes the two asset files and regenerates `include/asset_enums.h`.
Supported ROMs are identified by SHA-1; US 1.1 is the one this port is tested
with.

## Correctness

The game code in `src/`, `include/` and `libultra/` is the decompilation, and
every place the port diverges is an `#ifdef TARGET_PC` region whose `#else`
branch is the decompilation's own line. Two checks keep that honest:

- `make rom` (Linux, needs the IDO toolchain that `make assets` sets up) builds
  the N64 ROM from this tree with the decompilation's own build and fails unless
  the result is byte-identical to the retail US 1.1 ROM
  (SHA-1 `6d96743d46f8c0cd0edb0ec5600b003c89b93755`). This proves the N64 side
  of every `#ifdef` is still exactly the matching code.
- `make golden` runs the port for a fixed number of frames with no input, an
  erased save and a frame-counted clock, hashing everything the renderer is
  handed (state changes, vertices, texture pixels) and every audio buffer the
  mixer produces, one line per frame, and compares the result with
  `tests/golden/us_v80.trace`. The hashes do not involve the GPU or the audio
  device, so a 32-bit Raspberry Pi and a 64-bit Mac must produce the same file.
  A second trace, `tests/golden/us_v80_drive.trace`, replays the scripted
  controller in `tests/golden/drive.txt` for 7000 frames: through the player
  select, the caution screen and the adventure intro, then driving around the
  overworld, so it covers racer physics and the engine and skid sounds.
  `make golden-update` regenerates both; `tests/golden.sh` takes any binary and
  an optional input script. `DKR_TRACE_DETAIL=1` adds a line per renderer and
  audio event to locate a divergence within a frame, `DKR_TRACE_DRAW=<frame>:<n>`
  prints the vertices of one draw call, and `DKR_DUMP_FRAME` can capture the
  frame itself.

## Running

```
./build/release/dkracing [options]
```

| Flag                | Effect                                                         |
| ------------------- | -------------------------------------------------------------- |
| `--windowed`, `-w`  | start in a window instead of borderless fullscreen            |
| `--fullscreen`      | start fullscreen (the default)                                 |
| `--scale N`         | window size as a multiple of the 320x240 frame (default 3)     |
| `--no-vsync`        | present as fast as the GPU allows instead of once per refresh  |
| `--no-interp`       | no interpolated frames: draw only the 30Hz logic frames        |
| `--no-scanlines`    | start with the scanline effect off                             |
| `--no-widescreen`   | keep the 4:3 image letterboxed instead of widening it          |
| `--trace FILE`      | write the per-frame hash trace to FILE, then exit (see above)  |
| `--trace-frames N`  | frames to trace (default 1800)                                 |
| `--help`, `-h`      | print the options                                              |

Keys while running: Tab toggles fullscreen, 1 toggles the scanline effect, 2
toggles widescreen, Escape quits. The window is resizable.

### Widescreen

On by default: the image is widened to the window's (or display's) aspect
instead of being letterboxed, and the extra width shows more of the scene at
the sides. The game still projects for 4:3 and its matrices are untouched; the
host moves each viewport's centre out to the centre of the wider image while
keeping its pixel scale, so everything that was on screen before is exactly
where it was and the margins fill in. The HUD, menus and text stay at their
original size, centred; full-width fades and letterbox bars span the full
width; the background sky image is zoomed to cover the corners. The field of
view is clamped at 2.4:1 for very wide windows. The game culls objects by
distance and draws every level segment, so nothing pops in at the edges.
Traces (`make golden`) always run at 4:3.

### Effects

The scene is rendered into an offscreen framebuffer at the window's resolution
and copied to the screen through a post-processing shader
(`linux/shaders/post.frag`) that applies whichever effects are switched on.
Effects are runtime toggles; `--no-*` flags set their starting state.

| Effect    | Key | Default | What it does                                              |
| --------- | --- | ------- | --------------------------------------------------------- |
| Scanlines | 1   | on      | one dark line per N64 scanline, CRT style                 |

All shaders are plain OpenGL (GLSL 3.30 core / ES 3.00) and are embedded in the
executable as byte arrays at build time (`cmake/EmbedFile.cmake`), so nothing
ships beside the binary.

### Frame rate

The game's logic runs at the N64's 30Hz with its original fixed timestep, which
is what keeps the physics identical to the console. The frame rate is not tied
to it: between two logic frames the host redraws the last frame it was given
with every matrix interpolated between its previous and current values, as often
as the display will take a frame (vsync) or as fast as the GPU can draw one
(`--no-vsync`). Camera and object motion is smooth at any refresh rate, and so
is the racers' vertex animation (each vertex batch is paired with its
previous-frame self and interpolated too); the 2D layer steps at 30Hz. A camera
cut, a respawn or an object teleport is drawn where it lands rather than
blended across the jump: the game's transform is snapshotted with each matrix
tag, and one that moved implausibly far in a single tick is treated as new.

This is done entirely on the host side of the display list: the game tags each
matrix it pushes with the object or camera it belongs to (`pc_gfx_mtx_tag`, see
`include/pc_gfx.h`), the interpreter records the frame with those tags, and the
replay matches each matrix with last frame's by tag. The logic frame itself is
still rendered the moment it is submitted, exactly as before, so `make golden`
is unaffected (a trace never interpolates). Interpolation adds about half a
logic frame of display latency; `--no-interp` restores the plain 30Hz path.

Environment variables:

| Variable              | Effect                                                   |
| --------------------- | -------------------------------------------------------- |
| `DKR_DUMP_FRAME=<n>[,<m>...]` | write those frames to `dkr_frame_<n>.ppm` in the current directory (for testing without a screen) |
| `DKR_DUMP_EVERY=<k>`  | also write every *k*-th frame                            |
| `DKR_DUMP_PRESENT=<a>-<b>` | write every presented frame (logic and interpolated) numbered *a* to *b* as `dkr_present_<n>.ppm` |
| `DKR_INPUT_SCRIPT=<file>` | replay a controller script instead of reading the keyboard and gamepad; each line is `<frame> <buttons> <stickX> <stickY>` (buttons: comma-separated `A B Z START L R DU DD DL DR CU CD CL CR`, or `-`), held until the next line. Works with `--trace`; see `tests/golden/drive.txt` |
| `DKR_FPS_LOG=1`       | print logic frames, logic steps consumed, frames presented and camera cuts detected, once a second |
| `DKR_TRACE=<file>`, `DKR_TRACE_FRAMES=<n>` | same as `--trace` / `--trace-frames`      |
| `DKR_NO_CRASH_HANDLER=1` | do not catch fatal signals (`make run-debugger` sets this so the debugger sees the fault) |

On a crash (segmentation fault, bus error, abort) the game prints the current frame
number and a backtrace to stderr and appends it to `dkracing-crash.log` in the
working directory, then re-raises the signal. Please include that file in bug
reports. For file and line numbers run the debug build under `make run-debugger`.

Saves are kept in the per-user data directory as `eeprom.bin`:
`~/Library/Application Support/bluesentinelsec/DiddyKongRacing/` on macOS,
`~/.local/share/bluesentinelsec/DiddyKongRacing/` on Linux, and
`%APPDATA%\bluesentinelsec\DiddyKongRacing\` on Windows.

## Controls

| N64        | Keyboard        | Gamepad (Xbox layout)          |
| ---------- | --------------- | ------------------------------ |
| Stick      | arrows or WASD  | left stick                     |
| A          | X               | A, or right trigger            |
| B          | C               | B, or left trigger             |
| Z          | Z               | X                              |
| R          | Space           | right bumper                   |
| L          | Q               | left bumper                    |
| Start      | Enter           | Menu                           |
| C buttons  | I J K L         | right stick (Y is also C-up)   |
| D-pad      |                 | D-pad                          |

Gamepads can be plugged in while the game is running.

## Layout

- `linux/` - the host layer: `main.c` (display-list interpreter, frame
  recording and interpolation, frame loop),
  `gfx.c` (SDL3 window and shader renderer), `audio_hle.c` (RSP audio
  interpreter), `audio.c`, `input.c`, `reimpl.c` (libultra replacements, asset
  loading, saves), `pc_asset.c` (64-bit-safe asset unpacking), `shaders/`.
- `src/`, `libultra/`, `include/` - the decompiled game and its SDK, with
  `TARGET_PC` sections where the port diverges. `include/pc_*.h` are the host
  interfaces those sections call.
- `tests/` - unit tests for the host layer (`make test`).
- `Makefile.n64` - the decompilation's own build and asset pipeline.

## License

The host layer in this repository is provided under the terms in `LICENSE.md`.
Diddy Kong Racing is the property of its rights holders; this project contains
no game assets and requires a legitimately obtained ROM.
