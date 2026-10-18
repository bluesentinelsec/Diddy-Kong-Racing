# Building for iOS

**Not supported yet.** No iOS app project exists yet, and CI doesn't build
for iOS.

What already works with iOS in mind:

- The renderer has an OpenGL ES 3.0 path, the same one Android and the web
  build use.
- The on-screen touch controls (`linux/vcontrols.c`) turn on for
  `SDL_PLATFORM_IOS`.
- SDL3 supports iOS, including its own `main` entry point for an app bundle.

Until there's a native build, you can play on an iPhone or iPad through the
[web build](building-web.md) in Safari: the touch controls appear
automatically on touch-first devices.

When iOS support lands, this page will cover the Xcode and CMake setup,
signing, and how the assets get into the bundle.
