# Building on Windows

The supported toolchain is MinGW-w64 through [MSYS2](https://www.msys2.org/),
which is what CI builds with. The result is a single `dkracing.exe` with SDL3
linked statically, so you don't need to ship any DLLs. MSVC isn't tested.

## Requirements

1. Install MSYS2 from https://www.msys2.org/.
2. Open the **MSYS2 MINGW64** shell (not the plain "MSYS2" one) and install the
   toolchain:

   ```
   pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
     mingw-w64-x86_64-ninja git make
   ```

3. Install Python 3 if `python.exe` is not already available. It is needed only
   to extract the game data from your ROM.

Windows needs no other SDL dependencies: SDL uses the system's Win32, Direct3D
and WASAPI APIs.

## Build

From Command Prompt or PowerShell in the repository, use the Windows wrapper:

```
build-windows.bat          # Debug -> build\debug\dkracing.exe
build-windows.bat release  # Release -> build\release\dkracing.exe
build-windows.bat test     # Debug build plus CTest
build-windows.bat run      # Debug build, then run windowed
```

The script finds a standard MSYS2 MINGW64 installation, configures CMake with
Ninja, and stages existing runtime assets beside the executable. It has the
same default/release split as `make` and `make release` on POSIX.

To invoke CMake yourself from the MINGW64 shell:

```
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

The output is `build/release/dkracing.exe`. The Makefile wrappers (`make`,
`make release`, `make test`) also work in this shell.

## Game data

Put your US 1.1 ROM (`.z64`, `.v64`, or `.n64`) in the repository root or in
`baseroms/`, then run this from Command Prompt or PowerShell:

```
build-windows.bat assets
```

On Windows this creates a local Python virtual environment, installs the pinned
extraction tools, identifies the ROM by SHA-1, and writes
`assets/assets.bin` and `assets/assets.lut.bin`. The build wrapper stages those
two files beside each executable it builds, so it can also be launched from
Explorer. You can invoke the extraction script directly from PowerShell and
optionally give it an explicit ROM path:

```
powershell -ExecutionPolicy Bypass -File tools/extract-pc-assets-windows.ps1
powershell -ExecutionPolicy Bypass -File tools/extract-pc-assets-windows.ps1 C:\path\to\rom.z64
```

## Run

Put the `assets/` folder (see [README.md](README.md#game-data-every-platform))
next to `dkracing.exe` or in the folder you start it from, then:

```
build-windows.bat run
```

or double-click the executable. When running from the repository as shown
above, it can also use the `assets/` directory there.

## Test

`build-windows.bat test` configures the Debug test targets, builds them, and
runs `ctest --output-on-failure`.
