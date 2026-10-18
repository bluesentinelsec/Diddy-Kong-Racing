# Building for Android

The Android app in `android/` builds the game as `libmain.so` through the root
`CMakeLists.txt` and loads it from SDL's activity. The target device is the
Kindle Fire HD 8 (12th gen), but any arm64 Android 5.0+ device with OpenGL ES
3.0 works. The on-screen touch controls are always on, and a gamepad or
keyboard works alongside them.

## Requirements

- JDK 17 or newer (Android Studio's bundled JBR works).
- Android SDK with:
  - `platforms;android-35`
  - `ndk;29.0.14206865` (the version `android/app/build.gradle` pins)
  - `cmake;3.31.5`

With Android Studio, install these from the SDK Manager. Without it, use the
command-line tools:

```
sdkmanager "platforms;android-35" "ndk;29.0.14206865" "cmake;3.31.5"
```

Point Gradle at the SDK with `ANDROID_HOME` or with `android/local.properties`
(`sdk.dir=/path/to/Android/Sdk`). Point it at the JDK with `JAVA_HOME`, for
example `JAVA_HOME=~/.local/opt/android-studio/jbr`.

## Build

```
make assets                  # once, needs your ROM; copied into the APK
cd android
./fetch-sdl.sh               # clone SDL3 into android/SDL (once)
./gradlew :app:assembleDebug # -> app/build/outputs/apk/debug/app-debug.apk
```

On Windows, use `gradlew.bat :app:assembleDebug`. `fetch-sdl.sh` needs a bash
shell (Git Bash or WSL), or clone
`https://github.com/libsdl-org/SDL.git` at tag `release-3.4.18` into
`android/SDL` yourself.

Why SDL is a separate checkout: the desktop build gets SDL through CMake
FetchContent, but Android also needs SDL's Java activity classes, so Gradle
and the native build share the one tree in `android/SDL` (gitignored). Keep its
tag the same as `DKR_SDL3_TAG` in `CMakeLists.txt`.

The build targets `arm64-v8a`. To add ABIs, edit `abiFilters` in
`android/app/build.gradle`.

## Install and run

Turn on developer options and USB debugging on the device (on a Kindle:
Settings > Device Options > About Fire Tablet, tap the serial number seven
times, then turn on Developer Options > USB debugging), then:

```
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n org.dkr.port/.DKRActivity
```

The game runs in landscape. To read its log: `adb logcat | grep -i sdl`.

## CI

`.github/workflows/android.yml` installs the same SDK components, fetches SDL
at the tag from `CMakeLists.txt`, runs `./gradlew :app:assembleDebug`, checks
that the APK contains `libmain.so` and `libSDL3.so`, and uploads it as the
`dkracing-android-arm64` artifact. CI has no game data, so that APK starts but
has nothing to load. To play it, build locally after `make assets`.
