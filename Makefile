# Diddy Kong Racing PC port - developer Makefile.
#
# This wraps the CMake build in the targets people expect. The N64 ROM build and
# the asset pipeline from the decompilation live in Makefile.n64 (see `assets`).
#
#   make            debug build        -> build/debug/dkracing
#   make release    optimized build    -> build/release/dkracing
#   make run        build (debug) and run, windowed
#   make test       build and run the unit tests (ctest)
#   make fmt        clang-format the port's own sources
#   make doc        Doxygen HTML into docs/html
#   make assets     extract and pack the game assets from baseroms/ (needs the ROM)
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

.PHONY: all debug release run test fmt doc assets clean

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

test: debug
	ctest --test-dir $(DEBUG_DIR) --output-on-failure

fmt:
	clang-format -i $(FORMAT_SOURCES)

doc:
	doxygen Doxyfile
	@echo "Documentation written to docs/html/index.html"

assets:
	$(MAKE) -f Makefile.n64 setup
	$(MAKE) -f Makefile.n64 extract REGION=us VERSION=v80
	$(MAKE) -f Makefile.n64 build_assets REGION=us VERSION=v80

clean:
	rm -rf build docs/html
