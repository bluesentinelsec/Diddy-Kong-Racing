# Packaging

Each platform has a `make <platform>-package` target that builds the release
configuration and wraps it the way that platform distributes software. The
output lands in `build/dist/` as `DiddyKongRacing-<version>-<platform>[-<arch>]`
with the version from the `VERSION` file. `make package` picks the target for
the host it runs on.

**The game data goes in at build time.** Desktop executables embed
`assets/assets.bin` and `assets/assets.lut.bin` (CMake's `DKR_EMBED_ASSETS`,
on by default when the files exist); the mobile apps and the web page pack them
in. Run `make assets` with your ROM first. A package built without them is not
playable, and that is what CI produces, since it has no game data. Because the
packages contain the game's data, they are for your own devices; do not
redistribute them.

| Target | Host | Output | What it is |
| ------ | ---- | ------ | ---------- |
| `make windows-package` | Windows (MSYS2 MINGW64) | `...-windows-x86_64.zip` | `dkracing.exe` with icon and version resource, README, licence |
| `make linux-package` | Linux | `...-linux-<arch>.AppImage` | one-file AppImage with desktop entry, icon and AppStream metadata |
| `make macos-package` | macOS | `Diddy Kong Racing.app` and `...-macos-<arch>.app.zip` | the signed app bundle, plus a ditto zip of it for transport |
| `make web-package` | any with Docker | `...-web.zip` | the page, script, wasm, packed assets, favicon and web-app manifest |
| `make android-package` | any with the Android SDK | `...-android-arm64.apk` | release APK signed with a local key |
| `make ios-package` | macOS with Xcode | `...-ios.ipa` | the signed app, for installing on your own devices |

## Platform notes

**Windows.** The executable's icon and its version information (Properties >
Details: product name, company, file version, copyright) come from
`windows/dkracing.rc.in`, compiled in by MinGW's `windres`. The zip is made
with PowerShell's `Compress-Archive`, which every Windows has.

**Linux.** `appimagetool` is downloaded into `build/tools/` on first use and
run with `--appimage-extract-and-run`, so FUSE is not needed to build. The
AppImage runs on any distribution with glibc at least as new as the build
host's; build on the oldest system you want to support (CI uses Ubuntu
24.04). The desktop entry and icon are in `linux/packaging/`; the AppStream
metadata is configured by CMake with the version.

**macOS.** `make macos-package` assembles `Diddy Kong Racing.app` from the
release executable, `macos/Info.plist.in` (configured by CMake with the
version and build number) and `macos/dkracing.icns`, then signs it. The app
is the deliverable; the `.app.zip` beside it is made with `ditto`, the
archiver that keeps a bundle and its code signature intact, so it can be
sent or uploaded and unzipped with a double click. If the keychain holds
a Developer ID Application certificate it is used, with the hardened runtime
and a secure timestamp; otherwise the app is signed ad hoc, which runs on the
building Mac and needs `xattr -d com.apple.quarantine` on others. Set
`MACOS_IDENTITY="Developer ID Application: ..."` to choose a certificate.
The app is not notarized, so on another Mac the first launch is blocked by
Gatekeeper; the user opens System Settings > Privacy & Security and clicks
"Open Anyway" once (on macOS 14 and earlier, right-click > Open does it).
Notarizing would remove that step and is a small addition if wanted.

**Web.** `make web-package` runs the Docker-based build (`make web`) and zips
the result. Serve the zip's contents from any static host; the page needs
WebGL 2 and must be served over HTTP (not opened as a file). The manifest lets
phones and tablets add it to the home screen as a full-screen landscape app.

**Android.** `make android-package` creates `android/release.keystore` on
first use (`keytool`, password from `ANDROID_KEYSTORE_PASSWORD`, default
`dkracing`) and builds `assembleRelease` signed with it. The key is gitignored;
keep it, because updates must be signed with the same key to install over an
earlier version. Needs the SDK components from
[building-android.md](building-android.md).

**iOS.** `make ios-package` builds the app signed with the team Xcode is
signed into (`IOS_TEAM=` overrides) and zips it as an `.ipa`. Install it with
`xcrun devicectl device install app`, Apple Configurator, or a sideloading
tool; devices must be registered with the team, as with `make run-ios`.

## Icons

`tools/make-icons.py` generates every icon from one design: the iOS asset
catalog, the Android mipmaps, `windows/dkracing.ico`, `macos/dkracing.icns`,
`linux/packaging/dkracing.png` and the web favicon and manifest icons. The
files are committed; rerun the script (needs Pillow) after changing the
design.

## CI

`.github/workflows/package.yml` runs the Windows, Linux (x86_64 and arm64),
macOS and web package targets on every push and pull request and uploads the
results as artifacts. They are built without game data, so they show the
packaging works, not that the game runs; macOS is signed ad hoc there.
