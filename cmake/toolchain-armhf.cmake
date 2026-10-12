# CMake toolchain: 32-bit ARM (armhf) build on a 64-bit Raspberry Pi OS host with
# Debian multiarch (gcc-arm-linux-gnueabihf, libsdl2-dev:armhf, libgl1-mesa-dev:armhf).
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(ENV{PKG_CONFIG_PATH} "/usr/lib/arm-linux-gnueabihf/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/arm-linux-gnueabihf/pkgconfig:/usr/share/pkgconfig")
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)
set(OPENGL_gl_LIBRARY /usr/lib/arm-linux-gnueabihf/libGL.so)
set(OPENGL_INCLUDE_DIR /usr/include)
