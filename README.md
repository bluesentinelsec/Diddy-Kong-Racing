# Diddy Kong Racing - PC port

A native port of Diddy Kong Racing (Nintendo 64, 1997) to Windows, macOS,
Linux, the browser, Android and iOS. The game logic is the decompiled C of the
original; this repository adds everything that makes it run on a modern host:
an interpreter for the game's F3DDKR display lists, a shader renderer (OpenGL
3.3 core or OpenGL ES 3.0), an interpreter for the RSP audio microcode, SDL3
for the window, input and sound, frame interpolation for an uncapped frame
rate, widescreen, and on-screen touch controls for phones and tablets.

It runs as a single self-contained executable on 64-bit and 32-bit hosts,
including the Raspberry Pi 5, and as an app on Android and iOS.

**No game data is in this repository.** You need your own copy of the US 1.1
ROM to produce the two asset files the game loads (see *Assets*).

## Building

Step-by-step guides for each platform (Linux, Windows, macOS, web, Android
and iOS) are in [`docs/`](docs/README.md). The port is versioned
([semantic](https://semver.org), the `VERSION` file at the root): `dkracing
--version` prints it with the commit it was built from, and the Android and iOS
apps carry it as their app version with the commit count as the build number.
The short version:

Requirements: CMake 3.24+, a C compiler (clang, GCC or MinGW-w64), git, and on
Linux the usual SDL build dependencies (X11, Wayland, ALSA/PulseAudio
development packages; see `.github/workflows/pc.yml` for the exact list). SDL3
itself is fetched and built automatically and linked statically.

```
make            # debug build  -> build/debug/dkracing
make release    # optimized    -> build/release/dkracing
```

From Command Prompt or PowerShell on Windows, the equivalent wrapper is:

```
build-windows.bat          # debug build  -> build\debug\dkracing.exe
build-windows.bat release  # optimized    -> build\release\dkracing.exe
build-windows.bat run      # debug build, then run windowed
```

Other targets:

| Target        | What it does                                                          |
| ------------- | --------------------------------------------------------------------- |
| `make run`    | debug build, then run windowed (`RUN_ARGS` to change the flags)       |
| `make run-debugger` | the same under lldb (macOS) or gdb (Linux); a crash prints a backtrace and leaves you at the debugger prompt |
| `make test`   | build and run the unit tests with ctest                               |
| `make fmt`    | clang-format the host layer's sources                                 |
| `make doc`    | Doxygen HTML documentation into `docs/html`                           |
| `make assets` | extract and pack the game assets from the ROM in `baseroms/`          |
| `make golden` | run the game deterministically and compare it with the golden trace   |
| `make web`    | browser build through Docker (Emscripten) into `build/web`            |
| `make run-web`| build it and serve it on http://localhost:8000                        |
| `make clean`  | remove all build output                                               |

`make package` (or `make windows-package`, `linux-package`, `macos-package`,
`web-package`, `android-package`, `ios-package`) builds a distributable for that
platform into `build/dist/`: a zip with a versioned, icon-bearing `.exe`, an
AppImage, a signed `.app` bundle, a zip of the web page, a release APK or
an `.ipa`. Desktop executables embed the game data at build time, so a release
is one file. See [`docs/packaging.md`](docs/packaging.md).

The Makefile is a thin wrapper; the build itself is CMake, so this works too:

```
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
```

CMake options: `DKR_VERSION` (`us_v80`, the default, or `us_v77`),
`DKR_SDL3_TAG` (the SDL release to build), `DKR_EMBED_ASSETS` (on when
`assets/` exists: the executable carries the game data and needs no folder
beside it), `DKR_SANITIZE=ON` for an AddressSanitizer build,
`DKR_BUILD_TESTS=ON` for the test executables, `DKR_EEPROM_PRESET_100=ON` to
start new save files 100% complete.

Cross-building a 32-bit binary on a 64-bit Raspberry Pi:
`cmake -S . -B build/pi -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armhf.cmake`.

### Browser build (Emscripten)

The port also builds for the browser. The toolchain lives in a Docker image
(`docker/web.Dockerfile`, a pinned `emscripten/emsdk` release), so Docker is
the only thing to install:

```
make web       # -> build/web/dkracing.html, .js, .wasm and .data (the assets)
make run-web   # builds, then serves build/web on http://localhost:8000
```

Open http://localhost:8000/dkracing.html and click the page once (browsers only
allow audio after a click). Keys and gamepads work as on the desktop; Tab
toggles fullscreen. Saves are kept in the browser's IndexedDB. The page needs
WebGL 2.

How it fits: the game loop blocks waiting for the next tick, which a page
cannot do, so the build uses Emscripten's ASYNCIFY to turn those waits into
yields to the browser; the renderer's OpenGL ES 3.0 path becomes WebGL 2
unchanged; `assets/` is packed into `dkracing.data` and served beside the
page. CI builds the page without assets to check the code still compiles for
wasm (it has no game data), so the CI artifact is not playable.

### Android (Kindle Fire / mobile)

An Android app lives in `android/`. It builds the game as `libmain.so` through
the same root `CMakeLists.txt` (which has an `ANDROID` branch), loaded by SDL's
activity; the renderer's OpenGL ES 3.0 path is used as-is. Because a gamepad or
keyboard is not guaranteed, an on-screen control layer (`linux/vcontrols.c`) is
drawn above the game: a left analog stick for steering and the N64's four face
buttons in a diamond at the bottom-right, labelled as on the pad: A
(accelerate), B (brake / reverse), Z (use item), R (hop / powerslide); a Start
pill at the bottom-centre and a C-up (camera zoom) button beside it. While
driving, a double tap on A latches it (cruise control: the button turns solid
and pulses); a single tap or the brake releases it, and it never engages in a
menu. The overlay appears only on mobile: Android (Kindle Fire included), iOS,
and the browser build on a phone or tablet (a touch-first, `pointer: coarse`
device). `DKR_VCONTROLS=1` forces it on anywhere (the mouse then acts as one
finger, for testing) and `DKR_VCONTROLS=0` forces it off.

Requirements: the Android SDK and NDK, an SDK CMake 3.24+ (`sdkmanager
"cmake;3.31.5"`), and a JDK 17+ for Gradle. Then:

```
make assets                 # once, to produce assets/ from your ROM (see Assets)
cd android
./fetch-sdl.sh              # clones the SDL3 source used by the app into android/SDL
./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

`fetch-sdl.sh` keeps SDL out of git (as the desktop build does) while giving
Gradle the SDL Java activity and the native build one shared SDL tree. The
assets are copied into the APK automatically from `assets/`. The build targets
`arm64-v8a`; add ABIs in `android/app/build.gradle` if you need them. CI
(`.github/workflows/android.yml`) builds the APK, without assets, on every push
and pull request.

## Assets

The game needs two files, `assets/assets.bin` and `assets/assets.lut.bin`.
Desktop builds embed them in the executable when they exist at build time
(otherwise the executable looks for `assets/` in the working directory or
beside itself); mobile and web builds pack them in. They are byte ranges of
the ROM, so producing them is a copy:

```
cp /path/to/your/rom.z64 baseroms/
make assets        # python3 tools/extract-assets.py
```

The ROM is identified by SHA-1, never modified, and never copied anywhere
else; `.v64` and `.n64` dumps are converted on the fly. US 1.1 (SHA-1
`6d96743d46f8c0cd0edb0ec5600b003c89b93755`) is the revision the port is built
and tested for; US 1.0 is also recognised (build with `-DDKR_VERSION=us_v77`).
On Windows, `build-windows.bat assets [ROM]` does the same with the system
Python.

## Correctness

The game code in `src/`, `include/` and `libultra/` is the decompiled
original, and every place the port changes it is an `#ifdef TARGET_PC` region
whose `#else` branch keeps the original line, so the console's behaviour is
always there to compare against. The check that keeps the port honest:

- `make golden` runs the game for a fixed number of frames with no input, an
  erased save and a frame-counted clock, hashing everything the renderer is
  handed (state changes, vertices, texture pixels) and every audio buffer the
  mixer produces, one line per frame, and compares the result with
  `tests/golden/us_v80.trace`. The hashes do not involve the GPU or the audio
  device, so a 32-bit Raspberry Pi and a 64-bit Mac must produce the same file.
  A second trace, `tests/golden/us_v80_drive.trace`, replays the scripted
  controller in `tests/golden/drive.txt` for 7000 frames: through the player
  select, the caution screen and the adventure intro, then driving around the
  overworld, so it covers racer physics and the engine and skid sounds.
  Then the **scene traces** in `tests/golden/scenes/`, one scripted path into
  each part of the game so that a change to its logic has a trace that
  exercises it: a car race, a hovercraft race and a plane race on Ancient Lake
  with the AI, a time trial against the ghost, and a race paused and resumed
  from the pause menu. Each script's header gives its length (`# frames N`)
  and, where the fresh save would not unlock what it needs, `# save 100` to
  start from the 100%-completion save image. `tests/golden/scenes.sh` runs
  them all; adding a scene is adding a script and recording it.
  `make golden-update` regenerates everything; `tests/golden.sh` takes any
  binary and an optional input script. `DKR_TRACE_DETAIL=1` adds a line per
  renderer and audio event (blend colours included) to locate a divergence
  within a frame, `DKR_TRACE_DRAW=<frame>:<n>[-<m>]` prints the vertices of a
  range of draw calls, and `DKR_DUMP_FRAME` can capture the frame itself.
  Traces run headless too, so the check works over ssh.

  The traces are the safety net for rewriting the game code into readable C:
  a rewrite is correct when every trace is identical. They already caught one
  real bug while being written (a pointer table allocated at the console's
  4 bytes per entry, which on a 64-bit host overran into the level names).

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
| `--audio-rate HZ`   | mix audio at HZ (default 44100; 22050 is what the console did) |
| `--resampler R`     | voice resampling filter: `sinc` (default) or `n64` (the console's) |
| `--trace FILE`      | write the per-frame hash trace to FILE, then exit (see above)  |
| `--trace-frames N`  | frames to trace (default 1800)                                 |
| `--help`, `-h`      | print the options                                              |

Keys while running: Tab toggles fullscreen, 1 toggles the scanline effect, 2
toggles widescreen, Escape quits. The window is resizable.


### iOS (iPhone / iPad)

The iOS app is built from the same root `CMakeLists.txt` (its `IOS` branch) as
an `.app` bundle with SDL3 linked statically; `ios/app.c` holds the iOS-only
code and `ios/Info.plist.in` the bundle's Info.plist. The game runs landscape
only with the same on-screen touch controls as Android, and a paired game
controller works alongside them. The assets are copied into the bundle at
build time.

```
make assets      # once, needs your ROM
make ios         # unsigned app (what CI builds) -> build/ios/Release-iphoneos/
make run-ios     # sign for the connected iPhone/iPad, install and launch
```

`make run-ios` uses the Apple team Xcode is signed into (`IOS_TEAM=` overrides
it) and the first connected device (`IOS_DEVICE=` picks one). Details, including
Developer Mode and what the first run sets up, are in
[`docs/building-ios.md`](docs/building-ios.md). CI builds the unsigned app on
every push.

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

### Audio

The port runs the game's own synthesizer (the libultra sequence and sound
players and a host implementation of the RSP audio microcode), so the mix is
the console's. One thing is different by default: it mixes at 44.1 kHz instead
of the console's 22.05 kHz. The samples and the reverb are authored for 22.05
kHz, so the host tells the synthesizer to scale every voice pitch and the
reverb's delay lines by the ratio (`include/pc_audio.h`); the result is the same
notes and the same timing, with the resampler and reverb working at the higher
rate and no extra upsampling of a 22 kHz stream by the sound device.
`--audio-rate 22050` gives the console's rate for comparison.

The other departure is the voice resampler. The RSP microcode pitch-shifts
each voice through a 4-tap, 64-phase filter, which the port reproduces
exactly (`--resampler n64`). The default is instead a 14-tap windowed-sinc
interpolator with 256 phases (`linux/audio_hle.c`): a flatter passband, so
pitched-up samples keep more of their top end, and much better rejection of
the images above it, which is where the 4-tap's warble on sustained notes
comes from. It works within the same per-voice state block the game
allocates, so the two filters can even be swapped mid-stream. Measured on the
same scripted run, the sinc mix carries about twice the 4-8 kHz energy of the
4-tap mix and a seventh of the energy above 12 kHz.

Traces always mix at 22.05 kHz through the console's filter, so `make golden`
is unaffected by either setting.

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
| `DKR_DUMP_PCM=<file>` | write the mixed audio as raw stereo 16-bit PCM at the mix rate, for listening tests and comparisons |
| `DKR_INPUT_SCRIPT=<file>` | replay a controller script instead of reading the keyboard and gamepad; each line is `<frame> <buttons> <stickX> <stickY>` (buttons: comma-separated `A B Z START L R DU DD DL DR CU CD CL CR`, or `-`), held until the next line. Works with `--trace`; see `tests/golden/drive.txt` |
| `DKR_FPS_LOG=1`       | print logic frames, logic steps consumed, frames presented and camera cuts detected, once a second |
| `DKR_TRACE=<file>`, `DKR_TRACE_FRAMES=<n>` | same as `--trace` / `--trace-frames`      |
| `DKR_TRACE_SAVE=100`  | a trace starts from the 100%-completion save instead of an erased one (scene scripts say `# save 100`) |
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

On phones and tablets (or with `DKR_VCONTROLS=1`) an on-screen layer adds touch
controls: the left stick steers, the lettered A, B, Z and R buttons at the
bottom-right are the N64's face buttons, START pauses and C-up changes the
camera. Touching the stick overrides a physical controller; the buttons are
additive, so a real gamepad still works too.

## Layout

- `linux/` - the host layer: `main.c` (display-list interpreter, frame
  recording and interpolation, frame loop),
  `gfx.c` (SDL3 window and shader renderer), `audio_hle.c` (RSP audio
  interpreter), `audio.c`, `input.c`, `vcontrols.c` (on-screen touch controls),
  `reimpl.c` (libultra replacements, asset loading, saves), `pc_asset.c`
  (64-bit-safe asset unpacking), `shaders/`.
- `src/`, `libultra/`, `include/` - the decompiled game and its SDK, with
  `TARGET_PC` sections where the port diverges. `include/pc_*.h` are the host
  interfaces those sections call; `include/asset_enums.h` names every asset.
- `android/` - the Android app (Gradle + SDL activity) that builds the game as
  `libmain.so` through the root CMakeLists and packages the assets into the APK.
- `ios/` - the iOS app's own source, Info.plist template, asset catalog (app
  icon, launch colour) and device helper; the app itself is generated into
  `build/ios` by CMake's Xcode generator.
- `tools/extract-assets.py` - produces `assets/` from your ROM (`make assets`);
  `tools/make-icons.py` regenerates the app icons (the PNGs are committed).
- `tests/` - the golden traces and the unit tests (`make golden`, `make test`).
- `docs/` - the per-platform build guides and the asset format reference
  (`docs/asset-format.md`).

## Credits

The game code is the work of the
[Diddy Kong Racing decompilation](https://github.com/DavidSM64/Diddy-Kong-Racing)
project, and this port began from
[Bruceleeto's PC/Dreamcast port](https://github.com/Bruceleeto/Diddy-Kong-Racing)
of it. The asset format reference in `docs/asset-format.md` is theirs too.

## License

The host layer in this repository is provided under the terms in `LICENSE.md`.
Diddy Kong Racing is the property of its rights holders; this project contains
no game assets and requires a legitimately obtained ROM.
