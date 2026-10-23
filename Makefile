# Diddy Kong Racing PC port - developer Makefile.
#
# This wraps the CMake build in the targets people expect; `make assets`
# produces the game data from your ROM (tools/assets.py).
#
#   make            debug build        -> build/debug/dkracing
#   make release    optimized build    -> build/release/dkracing
#   make run        build (debug) and run, windowed
#   make run-debugger  same, under lldb (macOS) or gdb (Linux); prints a backtrace on a crash
#   make test       build and run the unit tests (ctest)
#   make fmt        clang-format the port's own sources
#   make doc        Doxygen HTML into docs/html
#   make assets     extract and pack the game assets from baseroms/ (needs the ROM)
#   make golden     run the game deterministically and compare per-frame hashes with tests/golden/
#   make coverage   run every golden trace on a coverage build; report what they never execute
#   make web        browser build (Emscripten, in Docker) -> build/web/dkracing.html
#   make run-web    build it and serve it on http://localhost:8000
#   make ios        iOS app (Xcode, macOS only) -> build/ios/Release-iphoneos/Diddy Kong Racing.app
#   make run-ios    build it signed for the connected iPhone/iPad, install and launch it
#   make package    distributable package for this host (one of the below)
#   make windows-package / linux-package / macos-package / web-package /
#        android-package / ios-package   -> build/dist/DiddyKongRacing-<version>-<platform>.*
#   make clean      remove every build directory

CMAKE      ?= cmake
JOBS       ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CMAKE_ARGS ?=
RUN_ARGS   ?= --windowed

DEBUG_DIR   := build/debug
RELEASE_DIR := build/release
COVERAGE_DIR := build/coverage

# Everything the port adds on top of the decompilation; the decomp's own sources
# keep their upstream formatting so merges stay easy.
FORMAT_SOURCES := $(wildcard linux/*.c linux/*.h ios/*.c include/pc_asset.h include/pc_gfx.h tests/*.c tests/*.h)

.PHONY: golden golden-update coverage all debug release run run-debugger test fmt doc assets assets-unpack clean web web-image run-web ios run-ios package windows-package linux-package macos-package web-package android-package ios-package

all: debug

debug: $(DEBUG_DIR)/CMakeCache.txt
	$(CMAKE) --build $(DEBUG_DIR) -j $(JOBS)

release: $(RELEASE_DIR)/CMakeCache.txt
	$(CMAKE) --build $(RELEASE_DIR) -j $(JOBS)

$(DEBUG_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(DEBUG_DIR) -DCMAKE_BUILD_TYPE=Debug -DDKR_BUILD_TESTS=ON $(CMAKE_ARGS)

$(RELEASE_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(RELEASE_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_ARGS)

$(COVERAGE_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(COVERAGE_DIR) -DCMAKE_BUILD_TYPE=Debug -DDKR_COVERAGE=ON $(CMAKE_ARGS)

# Which game code the golden traces exercise: builds with clang coverage, runs every
# trace (they still have to match), and writes build/coverage/report/ (tests/coverage.sh).
coverage: $(COVERAGE_DIR)/CMakeCache.txt
	$(CMAKE) --build $(COVERAGE_DIR) -j $(JOBS)
	tests/coverage.sh $(COVERAGE_DIR)/dkracing

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
# The scene traces (tests/golden/scenes/*.txt, one per part of the game) run
# after the two base traces; tests/golden/scenes.sh reads each script's length.
golden: debug
	tests/golden.sh $(DEBUG_DIR)/dkracing $(GOLDEN) $(GOLDEN_FRAMES)
	tests/golden.sh $(DEBUG_DIR)/dkracing $(GOLDEN_DRIVE) $(GOLDEN_DRIVE_FRAMES) $(GOLDEN_DRIVE_INPUT)
	tests/golden/scenes.sh $(DEBUG_DIR)/dkracing

golden-update: debug
	tests/golden.sh --update $(DEBUG_DIR)/dkracing $(GOLDEN) $(GOLDEN_FRAMES)
	tests/golden.sh --update $(DEBUG_DIR)/dkracing $(GOLDEN_DRIVE) $(GOLDEN_DRIVE_FRAMES) $(GOLDEN_DRIVE_INPUT)
	tests/golden/scenes.sh --update $(DEBUG_DIR)/dkracing

fmt:
	clang-format -i $(FORMAT_SOURCES)

doc:
	doxygen Doxyfile
	@echo "Documentation written to docs/html/index.html"

# The game data. tools/assets.py verifies the ROM in baseroms/ (any file
# name; it is identified by SHA-1), unpacks it into assets/tree/ (one native
# file per asset, deflated ones inflated) and packs that tree into the image
# the game loads, assets/assets.bin + assets/assets.lut.bin. Python 3 only.
# `make assets-unpack` redoes the tree from the ROM; `make assets` alone
# repacks an existing tree, so edits to it reach the game.
ifeq ($(OS),Windows_NT)
PYTHON ?= python
else
PYTHON ?= python3
endif
assets:
	$(PYTHON) tools/assets.py build
assets-unpack:
	$(PYTHON) tools/assets.py unpack

# Browser build. The Emscripten toolchain lives in a Docker image
# (docker/web.Dockerfile) so nothing has to be installed on the host; the
# repository is bind-mounted into it and the output lands in build/web like any
# other configuration. Needs the assets (make assets) for a playable page.
WEB_DIR   := build/web
DIST_DIR  := build/dist
WEB_IMAGE ?= dkracing-web
WEB_PORT  ?= 8000
DOCKER    ?= docker

web-image:
	$(DOCKER) build -t $(WEB_IMAGE) -f docker/web.Dockerfile docker

web: web-image
	@mkdir -p $(WEB_DIR) $(DIST_DIR) # created here as the user: on Linux, files the container makes are root's
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

# --- Packaging -----------------------------------------------------------------
# Each target builds the release configuration for its platform and wraps it the
# way that platform distributes software, named
# build/dist/DiddyKongRacing-<version>-<platform>[-<arch>]. The game data is
# embedded in desktop executables and packed into the mobile and web builds at
# build time, so run `make assets` first; a package built without it is not
# playable (that is what CI produces). See docs/packaging.md.
PORT_VERSION := $(shell cat VERSION)
HOST_ARCH    := $(shell uname -m | sed 's/aarch64/arm64/;s/amd64/x86_64/')
PKG          := DiddyKongRacing-$(PORT_VERSION)

package:
ifeq ($(OS),Windows_NT)
	$(MAKE) windows-package
else ifeq ($(UNAME_S),Darwin)
	$(MAKE) macos-package
else
	$(MAKE) linux-package
endif

# Windows: a zip with the executable (icon and version resource built in).
windows-package: release
	rm -rf $(DIST_DIR)/$(PKG)-windows-x86_64 && mkdir -p $(DIST_DIR)/$(PKG)-windows-x86_64
	cp $(RELEASE_DIR)/dkracing.exe README.md LICENSE.md $(DIST_DIR)/$(PKG)-windows-x86_64/
	cd $(DIST_DIR) && rm -f $(PKG)-windows-x86_64.zip && \
	  powershell.exe -NoProfile -Command "Compress-Archive -Force -Path '$(PKG)-windows-x86_64' -DestinationPath '$(PKG)-windows-x86_64.zip'"
	@ls -la $(DIST_DIR)/$(PKG)-windows-x86_64.zip

# Linux: an AppImage (desktop entry, icon, AppStream metadata, one file).
# appimagetool is fetched on first use into build/tools.
APPIMAGE_ARCH := $(shell uname -m | sed 's/arm64/aarch64/;s/amd64/x86_64/')
APPIMAGETOOL  := build/tools/appimagetool-$(APPIMAGE_ARCH).AppImage
$(APPIMAGETOOL):
	mkdir -p build/tools
	curl -sSL -o $@ https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$(APPIMAGE_ARCH).AppImage
	chmod +x $@

linux-package: release $(APPIMAGETOOL)
	rm -rf $(DIST_DIR)/AppDir && mkdir -p $(DIST_DIR)/AppDir/usr/bin $(DIST_DIR)/AppDir/usr/share/applications \
	  $(DIST_DIR)/AppDir/usr/share/icons/hicolor/256x256/apps $(DIST_DIR)/AppDir/usr/share/metainfo
	cp $(RELEASE_DIR)/dkracing $(DIST_DIR)/AppDir/usr/bin/
	cp linux/packaging/com.bluesentinelsec.dkracing.desktop $(DIST_DIR)/AppDir/ 
	cp linux/packaging/com.bluesentinelsec.dkracing.desktop $(DIST_DIR)/AppDir/usr/share/applications/
	cp linux/packaging/dkracing.png $(DIST_DIR)/AppDir/com.bluesentinelsec.dkracing.png
	cp linux/packaging/dkracing.png $(DIST_DIR)/AppDir/usr/share/icons/hicolor/256x256/apps/com.bluesentinelsec.dkracing.png
	cp $(RELEASE_DIR)/com.bluesentinelsec.dkracing.metainfo.xml $(DIST_DIR)/AppDir/usr/share/metainfo/
	ln -sf usr/bin/dkracing $(DIST_DIR)/AppDir/AppRun
	cd $(DIST_DIR) && ARCH=$(APPIMAGE_ARCH) ../../$(APPIMAGETOOL) --appimage-extract-and-run AppDir $(PKG)-linux-$(HOST_ARCH).AppImage
	@ls -la $(DIST_DIR)/$(PKG)-linux-$(HOST_ARCH).AppImage

# macOS: the app bundle itself, signed with a Developer ID certificate when the
# keychain has one (MACOS_IDENTITY overrides the choice), otherwise ad hoc. It is
# also zipped with ditto, the one archiver that keeps a bundle and its signature
# intact, for transport.
MACOS_IDENTITY ?= $(shell security find-identity -v -p codesigning 2>/dev/null | sed -n 's/.*"\(Developer ID Application: [^"]*\)".*/\1/p' | head -1)
MACOS_APP := $(DIST_DIR)/Diddy Kong Racing.app
macos-package: release
	rm -rf "$(MACOS_APP)" && mkdir -p "$(MACOS_APP)/Contents/MacOS" "$(MACOS_APP)/Contents/Resources"
	cp $(RELEASE_DIR)/dkracing "$(MACOS_APP)/Contents/MacOS/dkracing"
	cp $(RELEASE_DIR)/Info.plist "$(MACOS_APP)/Contents/Info.plist"
	cp macos/dkracing.icns "$(MACOS_APP)/Contents/Resources/dkracing.icns"
	printf 'APPL????' > "$(MACOS_APP)/Contents/PkgInfo"
	@if [ -n "$(MACOS_IDENTITY)" ] && codesign --force --options runtime --timestamp -s "$(MACOS_IDENTITY)" "$(MACOS_APP)" 2>/dev/null; then \
	  echo "signed with: $(MACOS_IDENTITY)"; \
	else \
	  codesign --force -s - "$(MACOS_APP)" && echo "signed ad hoc (no Developer ID certificate usable)"; \
	fi
	codesign --verify --strict "$(MACOS_APP)"
	rm -f $(DIST_DIR)/$(PKG)-macos-$(HOST_ARCH).app.zip
	ditto -c -k --keepParent "$(MACOS_APP)" $(DIST_DIR)/$(PKG)-macos-$(HOST_ARCH).app.zip
	@ls -lad "$(MACOS_APP)" $(DIST_DIR)/$(PKG)-macos-$(HOST_ARCH).app.zip

# Web: the page, its script, the wasm, the packed assets and the icons, zipped
# for any static host.
web-package: web
	mkdir -p $(DIST_DIR) && rm -f $(DIST_DIR)/$(PKG)-web.zip
	cd $(WEB_DIR) && python3 -m zipfile -c ../../$(DIST_DIR)/$(PKG)-web.zip dkracing.html dkracing.js dkracing.wasm \
	  $$(ls dkracing.data 2>/dev/null) favicon.png icon-192.png icon-512.png manifest.webmanifest
	@ls -la $(DIST_DIR)/$(PKG)-web.zip

# Android: a release APK signed with a local key (android/release.keystore,
# generated on first use and kept out of git; ANDROID_KEYSTORE_PASSWORD sets
# its password). Needs the SDK as described in docs/building-android.md.
ANDROID_KEYSTORE_PASSWORD ?= dkracing
android/release.keystore:
	keytool -genkeypair -keystore $@ -alias dkracing -keyalg RSA -keysize 2048 -validity 10000 \
	  -storepass $(ANDROID_KEYSTORE_PASSWORD) -keypass $(ANDROID_KEYSTORE_PASSWORD) -dname "CN=bluesentinelsec"

android-package: android/release.keystore
	test -d android/SDL || android/fetch-sdl.sh
	cd android && ./gradlew :app:assembleRelease -Pdkr.keystore=$(CURDIR)/android/release.keystore \
	  -Pdkr.keystorePassword=$(ANDROID_KEYSTORE_PASSWORD)
	mkdir -p $(DIST_DIR) && cp android/app/build/outputs/apk/release/app-release.apk $(DIST_DIR)/$(PKG)-android-arm64.apk
	@ls -la $(DIST_DIR)/$(PKG)-android-arm64.apk

# iOS: an .ipa of the app signed with the team Xcode is signed into, for
# installing on your own devices (sideloading); there is no store listing.
ios-package: $(IOS_DIR)/CMakeCache.txt
	@test -n "$(IOS_TEAM)" || { echo "ios-package: no Apple team; sign into Xcode or set IOS_TEAM=<team id>"; exit 1; }
	$(XCODEBUILD) -destination generic/platform=iOS -allowProvisioningUpdates build
	rm -rf $(DIST_DIR)/Payload && mkdir -p $(DIST_DIR)/Payload && cp -R "$(IOS_APP)" $(DIST_DIR)/Payload/
	cd $(DIST_DIR) && rm -f $(PKG)-ios.ipa && python3 -m zipfile -c $(PKG)-ios.ipa Payload && rm -rf Payload
	@ls -la $(DIST_DIR)/$(PKG)-ios.ipa

clean:
	rm -rf build docs/html
