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
#   make web        browser build (Emscripten, in Docker) -> build/web/dkracing.html
#   make run-web    build it and serve it on http://localhost:8000
#   make ios        iOS app (Xcode, macOS only) -> build/ios/Release-iphoneos/Diddy Kong Racing.app
#   make run-ios    build it signed for the connected iPhone/iPad, install and launch it
#   make clean      remove every build directory

CMAKE      ?= cmake
JOBS       ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CMAKE_ARGS ?=
RUN_ARGS   ?= --windowed

DEBUG_DIR   := build/debug
RELEASE_DIR := build/release

# Everything the port adds on top of the decompilation; the decomp's own sources
# keep their upstream formatting so merges stay easy.
FORMAT_SOURCES := $(wildcard linux/*.c linux/*.h ios/*.c include/pc_asset.h include/pc_gfx.h tests/*.c tests/*.h)

.PHONY: rom golden golden-update all debug release run run-debugger test fmt doc assets clean web web-image run-web ios run-ios

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

ifeq ($(OS),Windows_NT)
assets:
	powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/extract-pc-assets-windows.ps1
else
assets:
	$(MAKE) -f Makefile.n64 setup
	$(MAKE) -f Makefile.n64 extract REGION=us VERSION=v80
	$(MAKE) -f Makefile.n64 build_assets REGION=us VERSION=v80
endif

# The correctness gate: the decompilation's own build (IDO 5.3 via the recomp
# toolchain, Linux only) must reproduce the retail ROM bit for bit from this tree.
# Makefile.n64 checks the SHA-1 itself and fails the build on a mismatch.
rom:
	$(MAKE) -f Makefile.n64 REGION=us VERSION=v80 -j$(JOBS)
	cmp build/dkr.us.v80.z64 baseroms/baserom.us.v80.z64 && echo "rom: build/dkr.us.v80.z64 is byte-identical to the retail ROM"

# Browser build. The Emscripten toolchain lives in a Docker image
# (docker/web.Dockerfile) so nothing has to be installed on the host; the
# repository is bind-mounted into it and the output lands in build/web like any
# other configuration. Needs the assets (make assets) for a playable page.
WEB_DIR   := build/web
WEB_IMAGE ?= dkracing-web
WEB_PORT  ?= 8000
DOCKER    ?= docker

web-image:
	$(DOCKER) build -t $(WEB_IMAGE) -f docker/web.Dockerfile docker

web: web-image
	$(DOCKER) run --rm -v "$(CURDIR)":/src -w /src $(WEB_IMAGE) sh -c \
	  'emcmake cmake -S . -B $(WEB_DIR) -G Ninja -DCMAKE_BUILD_TYPE=Release $(CMAKE_ARGS) && cmake --build $(WEB_DIR) -j $(JOBS)'
	@ls -la $(WEB_DIR)/dkracing.*

run-web: web
	@echo "Serving $(WEB_DIR) on http://localhost:$(WEB_PORT)/dkracing.html (Ctrl-C to stop)"
	$(DOCKER) run --rm -p $(WEB_PORT):8000 -v "$(CURDIR)/$(WEB_DIR)":/srv:ro -w /srv $(WEB_IMAGE) python3 -m http.server 8000

# iOS app. CMake generates an Xcode project (the only way to sign) and
# xcodebuild builds it. `make ios` builds unsigned, which is what CI does and
# enough to check the code; `make run-ios` signs for the connected device with
# the team Xcode is signed into (or IOS_TEAM=XXXXXXXXXX), installs and launches.
# Needs the assets (make assets) for a playable app; see docs/building-ios.md.
IOS_DIR    := build/ios
IOS_APP    := $(IOS_DIR)/Release-iphoneos/Diddy Kong Racing.app
IOS_BUNDLE ?= com.bluesentinelsec.dkracing
IOS_TEAM   ?= $(shell defaults read com.apple.dt.Xcode IDEProvisioningTeams 2>/dev/null | sed -n 's/.*teamID = \([A-Z0-9]*\);.*/\1/p' | head -1)
IOS_DEVICE ?= $(shell ios/device.sh 2>/dev/null)
XCODEBUILD := xcodebuild -project $(IOS_DIR)/dkracing.xcodeproj -scheme dkracing -configuration Release -quiet

$(IOS_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(IOS_DIR) -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
	  -DCMAKE_OSX_ARCHITECTURES=arm64 -DDKR_IOS_BUNDLE_ID=$(IOS_BUNDLE) -DDKR_IOS_TEAM=$(IOS_TEAM) $(CMAKE_ARGS)

ios: $(IOS_DIR)/CMakeCache.txt
	$(XCODEBUILD) -destination generic/platform=iOS CODE_SIGNING_ALLOWED=NO build
	@ls -d "$(IOS_APP)"

run-ios: $(IOS_DIR)/CMakeCache.txt
	@test -n "$(IOS_DEVICE)" || { echo "run-ios: no iOS device connected (or set IOS_DEVICE=<udid>)"; exit 1; }
	@test -n "$(IOS_TEAM)" || { echo "run-ios: no Apple team; sign into Xcode or set IOS_TEAM=<team id>"; exit 1; }
	$(XCODEBUILD) -destination 'platform=iOS,id=$(IOS_DEVICE)' -allowProvisioningUpdates -allowProvisioningDeviceRegistration build
	xcrun devicectl device install app --device $(IOS_DEVICE) "$(IOS_APP)"
	xcrun devicectl device process launch --device $(IOS_DEVICE) --terminate-existing $(IOS_BUNDLE)

clean:
	rm -rf build docs/html
