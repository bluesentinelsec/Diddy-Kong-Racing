# Building for iOS

The iOS app builds the game through the root `CMakeLists.txt` (its `IOS`
branch) as an `.app` bundle, with SDL3 linked statically like the desktop
builds. The renderer uses its OpenGL ES 3.0 path, the on-screen touch controls
(`linux/vcontrols.c`) are always on, and a paired game controller works
alongside them. The app runs in landscape only, like the Android app.

The iOS-only host code is `ios/app.c`; `ios/Info.plist.in` is the bundle's
Info.plist template and `ios/Assets.xcassets` holds the app icon and the launch
screen colour (regenerate the icon with `tools/make-icons.py`). The app's
version is the repo's `VERSION` file and its build number is the commit count,
so a fresh build always supersedes the one installed before it.

## Requirements

- macOS with Xcode 15 or newer, including its iOS platform component (Xcode >
  Settings > Components, or `xcodebuild -downloadPlatform iOS`).
- CMake 3.24 or newer.
- To run on a device: an Apple Developer account signed into Xcode (Xcode >
  Settings > Accounts). A free account works for your own devices.
- On the iPhone or iPad: Developer Mode on (Settings > Privacy & Security >
  Developer Mode), plugged in over USB, unlocked and trusted.

## Build

```
make assets   # once, needs your ROM; copied into the app bundle
make ios      # unsigned -> build/ios/Release-iphoneos/Diddy Kong Racing.app
```

`make ios` generates an Xcode project in `build/ios` and builds it with
`xcodebuild` without signing. That is what CI does and enough to check the
code; an unsigned app cannot be installed on a device.

## Install and run on a device

```
make run-ios
```

This builds the app signed for the connected device with automatic signing,
installs it and launches it. The team is taken from the account Xcode is
signed into; set `IOS_TEAM=XXXXXXXXXX` (the ten-character Team ID) to choose
one, and `IOS_DEVICE=<udid>` to pick a device when several are connected
(`ios/device.sh` lists the first one it finds). The first run registers the
device with your team and creates a development certificate and provisioning
profile, which can take a minute.

The bundle identifier defaults to `com.bluesentinelsec.dkracing`; override
it with `IOS_BUNDLE=...` if it clashes with an identifier in another team.

To see the game's log while it runs:

```
xcrun devicectl device process launch --console --terminate-existing \
    --device "$(ios/device.sh)" com.bluesentinelsec.dkracing
```

Saves live in the app's own data container (SDL's preference path), so they
survive reinstalls of the same bundle identifier and go away when the app is
deleted.

## Opening in Xcode

`open build/ios/dkracing.xcodeproj` opens the generated project; select the
`dkracing` scheme and your device and press Run. Xcode's debugger and GPU frame
capture work as usual. Regenerate the project with CMake rather than editing it
in Xcode, since it is overwritten on the next configure.

## CI

The `iOS arm64 (unsigned)` job in `.github/workflows/pc.yml` runs `make ios` on
a macOS runner, checks that the bundle has an arm64 executable and the landscape
orientation keys, and uploads it as the `dkracing-ios-arm64-unsigned` artifact.
CI has no signing identity and no game data, so the artifact shows the code
builds; to play, build locally with `make run-ios` after `make assets`.
