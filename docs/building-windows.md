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

Windows needs no other SDL dependencies: SDL uses the system's Win32, Direct3D
and WASAPI APIs.

## Build

From the MINGW64 shell, in the repository:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The output is `build/dkracing.exe`. The Makefile wrappers (`make`,
`make release`, `make test`) also work in this shell.

## Run

Put the `assets/` folder (see [README.md](README.md#game-data-every-platform))
next to `dkracing.exe` or in the folder you start it from, then:

```
./build/dkracing.exe --windowed
```

or double-click the executable. You can't run `make assets` natively on
Windows. Produce the files in WSL (Ubuntu) with the Linux steps and copy
`assets/` across.

## Test

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDKR_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
