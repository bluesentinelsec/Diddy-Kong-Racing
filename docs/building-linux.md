# Building on Linux

Tested on Ubuntu 24.04 (x86_64 and arm64) in CI, and on Fedora and Raspberry Pi
OS. The result is one executable with SDL3 linked statically.

## Requirements

- A C compiler (GCC or clang), CMake 3.24+, Ninja or make, git, pkg-config.
- SDL's development packages for the window, input and audio backends.

Debian / Ubuntu (the same list CI installs):

```
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build pkg-config git \
  libasound2-dev libpulse-dev libpipewire-0.3-dev libudev-dev libdbus-1-dev \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev \
  libxss-dev libxtst-dev libxinerama-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols libdecor-0-dev \
  libegl-dev libgl-dev libdrm-dev libgbm-dev libibus-1.0-dev
```

Fedora:

```
sudo dnf install -y gcc cmake ninja-build pkgconf-pkg-config git \
  alsa-lib-devel pulseaudio-libs-devel pipewire-devel systemd-devel dbus-devel \
  libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXi-devel \
  libXfixes-devel libXScrnSaver-devel libXtst-devel libXinerama-devel \
  libxkbcommon-devel wayland-devel wayland-protocols-devel libdecor-devel \
  mesa-libEGL-devel mesa-libGL-devel libdrm-devel mesa-libgbm-devel ibus-devel
```

SDL silently leaves out any backend whose headers it can't find. If the game
builds but gets no sound or no Wayland window, install the missing package,
then reconfigure from scratch (`make clean`).

## Build

```
make            # debug build  -> build/debug/dkracing (unit tests included)
make release    # optimized    -> build/release/dkracing
```

Or with CMake directly:

```
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

## Run

```
make assets                       # once, needs your ROM (see README.md in this folder)
./build/release/dkracing --windowed
```

`make run` builds the debug binary and runs it windowed. `make run-debugger`
does the same under gdb, so a crash leaves you at a backtrace.

## Test

```
make test       # unit tests (ctest)
make golden     # deterministic golden-trace check; needs the assets
```

## Raspberry Pi

A 64-bit Pi OS builds natively with the steps above. To cross-build a 32-bit
(armhf) binary on a 64-bit Pi:

```
cmake -S . -B build/pi -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armhf.cmake
cmake --build build/pi
```
