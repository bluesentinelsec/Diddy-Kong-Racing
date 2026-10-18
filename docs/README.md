# Building Diddy Kong Racing

The port builds from one CMake project (`CMakeLists.txt`) for every target.
SDL3 is fetched and built with the game, so the only host requirements are a
compiler, CMake and git, plus a few per-platform extras.

| Platform | Guide | Output | Checked in CI |
| -------- | ----- | ------ | ------------- |
| Linux (x86_64, arm64, Raspberry Pi) | [building-linux.md](building-linux.md) | `build/release/dkracing` | yes (`pc.yml`) |
| Windows (x86_64, MinGW-w64) | [building-windows.md](building-windows.md) | `build\release\dkracing.exe` | yes (`pc.yml`) |
| macOS (Apple silicon, Intel) | [building-macos.md](building-macos.md) | `build/release/dkracing` | yes, arm64 (`pc.yml`) |
| Web (Emscripten / WebGL 2) | [building-web.md](building-web.md) | `build/web/dkracing.html` | yes (`pc.yml`) |
| Android (Kindle Fire and phones) | [building-android.md](building-android.md) | `app-debug.apk` | yes (`android.yml`) |
| iOS | [building-ios.md](building-ios.md) | not supported yet | no |

CI builds every supported platform on each push to `master` and on every pull
request, and uploads the binaries as workflow artifacts. CI has no game data,
so those artifacts don't include assets: they show the code builds, but you
can't play them as-is.

## Game data (every platform)

**No game data is in this repository.** Every build loads two files,
`assets/assets.bin` and `assets/assets.lut.bin`, that you produce from your own
US 1.1 ROM:

```
cp /path/to/your/rom.z64 baseroms/
make assets
```

This needs Python 3. The full decompilation asset rebuild on Linux and macOS
also uses the MIPS binutils (`binutils-mips-linux-gnu` on Debian/Ubuntu).
Windows uses the native PowerShell/Splat extraction path documented in the
[Windows guide](building-windows.md). The output is the same on every platform,
so produce it once and copy `assets/` wherever you need it:

- **Desktop:** the executable looks for `assets/` in the working directory and
  next to itself.
- **Web:** `assets/` is packed into `dkracing.data` at build time.
- **Android:** `assets/` is copied into the APK at build time.

Build the game first and add the assets later if you like. Desktop builds
pick them up at run time. Web and Android builds pack them in, so rebuild those
after running `make assets`.

## Common CMake options

| Option | Default | Meaning |
| ------ | ------- | ------- |
| `DKR_VERSION` | `us_v80` | ROM revision the code is built for (`us_v80` or `us_v77`) |
| `DKR_SDL3_TAG` | `release-3.4.18` | SDL3 release to build against |
| `DKR_BUILD_TESTS` | `OFF` | build the unit tests (run with `ctest`) |
| `DKR_SANITIZE` | `OFF` | AddressSanitizer build |
| `DKR_EEPROM_PRESET_100` | `OFF` | new save files start 100% complete |

Pass them with `-D` to CMake, or through the Makefile as
`make CMAKE_ARGS="-DDKR_BUILD_TESTS=ON"`. The Makefile only remembers options
from the first configure of each build directory, so run `make clean` after
changing them.
