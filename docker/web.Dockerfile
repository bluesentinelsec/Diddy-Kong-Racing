# Build environment for the browser (Emscripten) build of the PC port.
#
#   make web        builds build/web/dkracing.{html,js,wasm,data} through this image
#   make run-web    serves build/web on http://localhost:8000
#
# Pinned to a specific emsdk release so the toolchain is the same on every
# machine and in CI; the image is published for amd64 and arm64, so Apple
# silicon runs it natively. It ships cmake but not ninja, which comes from pip.
# The repository is bind-mounted at /src at run time; nothing from the game is
# baked into the image.
# Fully qualified so Podman (which refuses short names without a TTY) works too.
FROM docker.io/emscripten/emsdk:6.0.11

RUN python3 -m pip install --no-cache-dir --break-system-packages ninja

WORKDIR /src
