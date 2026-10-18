# Building for the web

The browser build compiles the game to WebAssembly with Emscripten and renders
through WebGL 2. The toolchain lives in a Docker image
(`docker/web.Dockerfile`, pinned to `emscripten/emsdk:6.0.11`), so **Docker is
the only thing to install**. This works the same on Linux, macOS and Windows.

## Build

```
make assets    # once, needs your ROM; without it the page has no game data
make web       # -> build/web/dkracing.html, .js, .wasm and .data
```

`make web` builds the image, then runs `emcmake cmake` and `cmake --build`
inside it with the repository bind-mounted, so the output lands in
`build/web/` on the host.

### Podman instead of Docker

```
make web DOCKER=podman
```

On SELinux hosts (Fedora), the container can't read the bind-mounted
repository by default. If the build fails reading `/src`, run the steps by hand
with `--security-opt label=disable`:

```
podman build -t dkracing-web -f docker/web.Dockerfile docker
podman run --rm --security-opt label=disable -v "$PWD":/src -w /src dkracing-web sh -c \
  'emcmake cmake -S . -B build/web -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build/web'
```

## Run

```
make run-web   # builds, then serves build/web on http://localhost:8000
```

Open http://localhost:8000/dkracing.html and click the page once (browsers only
allow audio after a click). The page needs WebGL 2. Saves are kept in the
browser's IndexedDB. On a phone or tablet (a touch-first, `pointer: coarse`
device) the on-screen touch controls appear. On desktop browsers they don't.

To host it elsewhere, serve the four files in `build/web/` from any static web
server. Opening the HTML file straight from disk (`file://`) won't work.
