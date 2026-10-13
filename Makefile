# Diddy Kong Racing PC port - developer Makefile.
#
# This wraps the CMake build in the targets people expect. The N64 ROM build and
# the asset pipeline from the decompilation live in Makefile.n64 (see `assets`).
#
#   make            debug build        -> build/debug/dkracing
#   make release    optimized build    -> build/release/dkracing
#   make run        build (debug) and run, windowed
#   make run-debugger  same, under lldb (macOS) or gdb (Linux); prints a backtrace on a crash
#   make test       build and run the unit tests (ctest)
#   make fmt        clang-format the port's own sources
#   make doc        Doxygen HTML into docs/html
#   make assets     extract and pack the game assets from baseroms/ (needs the ROM)
#   make rom        build the N64 ROM with the IDO toolchain and verify it is byte-identical
#   make golden     run the game deterministically and compare per-frame hashes with tests/golden/
#   make clean      remove every build directory

CMAKE      ?= cmake
JOBS       ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CMAKE_ARGS ?=
RUN_ARGS   ?= --windowed

DEBUG_DIR   := build/debug
RELEASE_DIR := build/release

# Everything the port adds on top of the decompilation; the decomp's own sources
# keep their upstream formatting so merges stay easy.
FORMAT_SOURCES := $(wildcard linux/*.c linux/*.h include/pc_asset.h tests/*.c tests/*.h)

.PHONY: rom golden golden-update all debug release run run-debugger test fmt doc assets clean

all: debug

debug: $(DEBUG_DIR)/CMakeCache.txt
	$(CMAKE) --build $(DEBUG_DIR) -j $(JOBS)

release: $(RELEASE_DIR)/CMakeCache.txt
	$(CMAKE) --build $(RELEASE_DIR) -j $(JOBS)

$(DEBUG_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(DEBUG_DIR) -DCMAKE_BUILD_TYPE=Debug -DDKR_BUILD_TESTS=ON $(CMAKE_ARGS)

$(RELEASE_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(RELEASE_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_ARGS)

run: debug
	./$(DEBUG_DIR)/dkracing $(RUN_ARGS)

# Run the debug build under the platform's debugger. The process starts at once;
# on a crash the debugger prints a backtrace and leaves you at its prompt (type
# `quit` to leave). DEBUGGER overrides the choice, e.g. DEBUGGER=gdb on macOS.
# The game's own crash handler is disabled so the debugger sees the raw fault.
UNAME_S  := $(shell uname -s)
DEBUGGER ?= $(if $(filter Darwin,$(UNAME_S)),lldb,gdb)

run-debugger: debug
ifeq ($(DEBUGGER),lldb)
	DKR_NO_CRASH_HANDLER=1 lldb -o run -o 'bt all' -- ./$(DEBUG_DIR)/dkracing $(RUN_ARGS)
else
	DKR_NO_CRASH_HANDLER=1 $(DEBUGGER) -q -ex run -ex bt --args ./$(DEBUG_DIR)/dkracing $(RUN_ARGS)
endif

GOLDEN ?= tests/golden/us_v80.trace
GOLDEN_FRAMES ?= 1800
# Second trace: scripted input through the menus into the adventure overworld.
GOLDEN_DRIVE ?= tests/golden/us_v80_drive.trace
GOLDEN_DRIVE_FRAMES ?= 7000
GOLDEN_DRIVE_INPUT ?= tests/golden/drive.txt

test: debug
	ctest --test-dir $(DEBUG_DIR) --output-on-failure

# Golden-trace check: run the game deterministically and compare per-frame hashes
# of the renderer input and audio output with the committed trace (tests/golden.sh).
golden: debug
	tests/golden.sh $(DEBUG_DIR)/dkracing $(GOLDEN) $(GOLDEN_FRAMES)
	tests/golden.sh $(DEBUG_DIR)/dkracing $(GOLDEN_DRIVE) $(GOLDEN_DRIVE_FRAMES) $(GOLDEN_DRIVE_INPUT)

golden-update: debug
	tests/golden.sh --update $(DEBUG_DIR)/dkracing $(GOLDEN) $(GOLDEN_FRAMES)
	tests/golden.sh --update $(DEBUG_DIR)/dkracing $(GOLDEN_DRIVE) $(GOLDEN_DRIVE_FRAMES) $(GOLDEN_DRIVE_INPUT)

fmt:
	clang-format -i $(FORMAT_SOURCES)

doc:
	doxygen Doxyfile
	@echo "Documentation written to docs/html/index.html"

assets:
	$(MAKE) -f Makefile.n64 setup
	$(MAKE) -f Makefile.n64 extract REGION=us VERSION=v80
	$(MAKE) -f Makefile.n64 build_assets REGION=us VERSION=v80

# The correctness gate: the decompilation's own build (IDO 5.3 via the recomp
# toolchain, Linux only) must reproduce the retail ROM bit for bit from this tree.
# Makefile.n64 checks the SHA-1 itself and fails the build on a mismatch.
rom:
	$(MAKE) -f Makefile.n64 REGION=us VERSION=v80 -j$(JOBS)
	cmp build/dkr.us.v80.z64 baseroms/baserom.us.v80.z64 && echo "rom: build/dkr.us.v80.z64 is byte-identical to the retail ROM"

clean:
	rm -rf build docs/html
