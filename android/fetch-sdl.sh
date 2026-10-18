#!/usr/bin/env bash
# Clone the SDL3 source used by the Android build into android/SDL.
#
# The desktop build fetches SDL3 through CMake FetchContent, but the Android
# build needs SDL's Java activity classes too, so both the native build
# (add_subdirectory in the root CMakeLists) and Gradle (java srcDirs) use this
# one checkout. Keep the tag in sync with DKR_SDL3_TAG in the root CMakeLists.
set -euo pipefail

TAG="${1:-release-3.4.18}"
HERE="$(cd "$(dirname "$0")" && pwd)"
DIR="$HERE/SDL"

if [ -d "$DIR/.git" ]; then
    echo "SDL already present at $DIR (delete it to re-fetch)"
    exit 0
fi

echo "Cloning SDL $TAG into $DIR ..."
git clone --depth 1 --branch "$TAG" https://github.com/libsdl-org/SDL.git "$DIR"
echo "Done: $DIR"
