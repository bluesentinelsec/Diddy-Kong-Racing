# Building on macOS

CI builds on macOS 15 (Apple silicon). Intel Macs build the same way. The
result is a single `dkracing` executable with SDL3 linked statically.

## Requirements

- Xcode Command Line Tools: `xcode-select --install`
- CMake and Ninja, for example from [Homebrew](https://brew.sh/):

  ```
  brew install cmake ninja
  ```

macOS needs no other SDL dependencies.

## Build

```
make            # debug build  -> build/debug/dkracing
make release    # optimized    -> build/release/dkracing
```

Or with CMake directly:

```
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

## Run

```
./build/release/dkracing --windowed
```

Put `assets/` (see [README.md](README.md#game-data-every-platform)) in the
working directory or next to the executable; `make assets` produces it from
the ROM in `baseroms/` with the system Python 3.

`make run-debugger` runs the debug build under lldb.

## Test

```
make test
```
