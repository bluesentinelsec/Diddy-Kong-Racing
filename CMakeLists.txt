cmake_minimum_required(VERSION 3.24)

# The port's own version (semantic, in VERSION) and the commit it was built
# from; shown by --version and at startup, and stamped into the app bundles.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION DKR_PORT_VERSION LIMIT_COUNT 1)
string(STRIP "${DKR_PORT_VERSION}" DKR_PORT_VERSION)
project(dkracing VERSION ${DKR_PORT_VERSION} LANGUAGES C)
find_package(Git QUIET)
set(DKR_PORT_COMMIT "unknown")
set(DKR_PORT_BUILD 1) # monotonic build number for the app stores' CFBundleVersion / versionCode
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE commit
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE rc)
    if(rc EQUAL 0)
        set(DKR_PORT_COMMIT ${commit})
    endif()
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE count
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE rc)
    if(rc EQUAL 0)
        set(DKR_PORT_BUILD ${count})
    endif()
endif()
message(STATUS "Diddy Kong Racing PC port ${DKR_PORT_VERSION} (${DKR_PORT_COMMIT}, build ${DKR_PORT_BUILD})")
string(REPLACE "." ";" DKR_VERSION_PARTS "${DKR_PORT_VERSION}")
list(GET DKR_VERSION_PARTS 0 DKR_VERSION_MAJOR)
list(GET DKR_VERSION_PARTS 1 DKR_VERSION_MINOR)
list(GET DKR_VERSION_PARTS 2 DKR_VERSION_PATCH)
string(TIMESTAMP DKR_PORT_DATE "%Y-%m-%d" UTC)
set(DKR_BUNDLE_ID "com.bluesentinelsec.dkracing" CACHE STRING "Bundle / application identifier on the platforms that have one")
set(DKR_PORT_COPYRIGHT "Port by bluesentinelsec. Diddy Kong Racing is a trademark of Nintendo; the game content belongs to Rare and Nintendo. Not affiliated with or endorsed by either.")
set(DKR_HAS_ASSETS FALSE)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.bin AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.lut.bin)
    set(DKR_HAS_ASSETS TRUE)
endif()

# ---------------------------------------------------------------------------
# Diddy Kong Racing - native PC build.
#
# This builds the host executable only. The N64 ROM build (Makefile) and the
# asset pipeline (`make extract` / `make build_assets`) are unchanged; they
# must be run first because the game loads assets/assets.bin and
# assets/assets.lut.bin at runtime and the generated include/asset_enums.h at
# compile time. No Nintendo data is in this repository.
#
#   cmake -S . -B build/pc -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/pc -j
#   ./build/pc/dkracing            (run from the repo root)
# ---------------------------------------------------------------------------

set(DKR_VERSION "us_v80" CACHE STRING "ROM version the assets were extracted from (us_v77, us_v80)")
set_property(CACHE DKR_VERSION PROPERTY STRINGS us_v77 us_v80)
option(DKR_EEPROM_PRESET_100 "Seed a fresh save file with a 100%-completion image instead of an erased one" OFF)
option(DKR_SANITIZE "Build with AddressSanitizer" OFF)
option(DKR_BUILD_TESTS "Build the unit tests (run with ctest)" OFF)
set(DKR_MAIN_POOL_MB 16 CACHE STRING "Size of the game's main memory pool, in MiB")

if(DKR_VERSION STREQUAL "us_v77")
    set(DKR_VERSION_DEFINE VERSION_US_V77)
elseif(DKR_VERSION STREQUAL "us_v80")
    set(DKR_VERSION_DEFINE VERSION_US_V80)
else()
    message(FATAL_ERROR "Unsupported DKR_VERSION '${DKR_VERSION}'")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# SDL3: fetched from the official repository at a pinned stable release and
# built as a static library, so the executable is self-contained (no SDL
# shared library to ship or find at runtime). OpenGL itself is not linked: the
# renderer resolves its GL 3.3 core / GL ES 3.0 entry points through SDL.
set(DKR_SDL3_TAG "release-3.4.18" CACHE STRING "SDL3 git tag to build against")
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
if(ANDROID)
    # Android loads libSDL3.so from SDLActivity, so SDL is a shared library here.
    # It is taken from the checkout at android/SDL (created by android/fetch-sdl.sh)
    # so this native build and the Gradle build share one SDL tree — Gradle also
    # compiles SDL's Java activity classes out of that same directory.
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(DKR_SDL_TARGET SDL3::SDL3-shared)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/android/SDL/CMakeLists.txt)
        message(FATAL_ERROR "android/SDL not found. Run android/fetch-sdl.sh first.")
    endif()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/android/SDL ${CMAKE_BINARY_DIR}/SDL)
    message(STATUS "SDL3 from android/SDL (shared) for Android")
else()
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(DKR_SDL_TARGET SDL3::SDL3-static)
    include(FetchContent)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG ${DKR_SDL3_TAG}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)
    FetchContent_MakeAvailable(SDL3)
    message(STATUS "SDL3 ${DKR_SDL3_TAG} via FetchContent, static")
endif()

# Shaders are embedded as byte arrays so the executable is self-contained.
set(DKR_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linux/shaders)
set(DKR_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${DKR_GENERATED_DIR})
set(DKR_SHADER_HEADERS)
foreach(shader dkr.vert dkr.frag post.vert post.frag)
    string(REPLACE "." "_" symbol "shader_${shader}")
    set(header ${DKR_GENERATED_DIR}/${symbol}.h)
    add_custom_command(
        OUTPUT ${header}
        COMMAND ${CMAKE_COMMAND} -DINPUT=${DKR_SHADER_DIR}/${shader} -DOUTPUT=${header} -DSYMBOL=${symbol}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFile.cmake
        DEPENDS ${DKR_SHADER_DIR}/${shader} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFile.cmake
        COMMENT "Embedding ${shader}"
        VERBATIM)
    list(APPEND DKR_SHADER_HEADERS ${header})
endforeach()
add_custom_target(dkr_shaders DEPENDS ${DKR_SHADER_HEADERS})

# --- Sources -----------------------------------------------------------------

set(DKR_PLATFORM_SOURCES
    linux/main.c
    linux/audio.c
    linux/audio_hle.c
    linux/gfx.c
    linux/input.c
    linux/vcontrols.c
    linux/reimpl.c
    linux/pc_asset.c
    linux/trace.c
    linux/crash.c
    linux/android_compat.c
)

set(DKR_GAME_SOURCES
    src/asset_loading.c src/audio.c src/audiomgr.c src/audiosfx.c src/audio_spatial.c
    src/audio_vehicle.c src/borders.c src/camera.c src/fade_transition.c src/font.c
    src/game.c src/game_text.c src/game_ui.c src/get_stack_pointer.c src/gzip.c
    src/joypad.c src/lights.c src/main.c src/memory.c src/menu.c src/object_functions.c
    src/object_models.c src/objects.c src/os_yield_data.c src/particles.c src/printf.c
    src/racer.c src/rcp_dkr.c src/save_data.c src/screen_asset.c src/set_rsp_segment.c
    src/textures_sprites.c src/thread0_epc.c src/thread30_bgload.c src/thread3_main.c
    src/tracks.c src/unused_string.c src/vehicle_bluey.c src/vehicle_bubbler.c
    src/vehicle_rocket.c src/vehicle_smokey.c src/vehicle_tricky.c src/vehicle_wizpig.c
    src/video.c src/waves.c src/weather.c
    src/hasm/collision.c src/hasm/math_util.c src/hasm/obj_animate.c src/hasm/obj_shade_fast.c
)

set(DKR_LIBULTRA_SOURCES
    libultra/src/audio/bnkf.c libultra/src/audio/cents2ratio.c libultra/src/audio/copy.c
    libultra/src/audio/cseq.c libultra/src/audio/cspgetchlvol.c libultra/src/audio/cspgetstate.c
    libultra/src/audio/cspgettempo.c libultra/src/audio/cspplay.c libultra/src/audio/cspsetbank.c
    libultra/src/audio/cspsetchlvol.c libultra/src/audio/cspsetpan.c libultra/src/audio/cspsetseq.c
    libultra/src/audio/cspsettempo.c libultra/src/audio/cspsetvol.c libultra/src/audio/cspstop.c
    libultra/src/audio/env.c libultra/src/audio/event.c libultra/src/audio/filter.c
    libultra/src/audio/heapalloc.c libultra/src/audio/heapinit.c libultra/src/audio/load.c
    libultra/src/audio/mainbus.c libultra/src/audio/mips1/auxbus.c
    libultra/src/audio/mips1/cspgetfadein.c libultra/src/audio/mips1/csplayer.c
    libultra/src/audio/mips1/cspsetfadein.c libultra/src/audio/mips1/drvrnew.c
    libultra/src/audio/mips1/reverb.c libultra/src/audio/mips1/seqchannel.c
    libultra/src/audio/mips1/seqplayer.c libultra/src/audio/mips1/synallocfx.c
    libultra/src/audio/mips1/synsetpan.c libultra/src/audio/mips1/synstartvoiceparam.c
    libultra/src/audio/mips1/synthesizer.c libultra/src/audio/mips1/unknown_064690.c
    libultra/src/audio/resample.c libultra/src/audio/save.c libultra/src/audio/seqpgetfxmix.c
    libultra/src/audio/sl.c libultra/src/audio/synaddplayer.c libultra/src/audio/synallocvoice.c
    libultra/src/audio/syndelete.c libultra/src/audio/synfreevoice.c libultra/src/audio/synsetfxmix.c
    libultra/src/audio/synsetpitch.c libultra/src/audio/synsetpriority.c libultra/src/audio/synsetvol.c
    libultra/src/audio/synstartvoice.c libultra/src/audio/synstopvoice.c
    libultra/src/debug/assert.c libultra/src/debug/kdebugserver.c
    libultra/src/gu/mtxutil.c libultra/src/gu/perspective.c
    libultra/src/io/crc.c libultra/src/io/devmgr.c libultra/src/io/dp.c libultra/src/io/dpgetstat.c
    libultra/src/io/dpsetnextbuf.c libultra/src/io/dpsetstat.c libultra/src/io/epirawdma.c
    libultra/src/io/epirawwrite.c libultra/src/io/leointerrupt.c libultra/src/io/piacs.c
    libultra/src/io/pidma.c libultra/src/io/pigetcmdq.c libultra/src/io/pimgr.c
    libultra/src/io/pirawdma.c libultra/src/io/pirawread.c libultra/src/io/si.c
    libultra/src/io/siacs.c libultra/src/io/sirawread.c libultra/src/io/sirawwrite.c
    libultra/src/io/sp.c libultra/src/io/spgetstat.c libultra/src/io/sprawdma.c
    libultra/src/io/spsetpc.c libultra/src/io/spsetstat.c libultra/src/io/sptask.c
    libultra/src/io/sptaskyield.c libultra/src/io/sptaskyielded.c libultra/src/io/vi.c
    libultra/src/io/viblack.c libultra/src/io/vigetcurrcontext.c libultra/src/io/vigetcurrframebuf.c
    libultra/src/io/vigetnextframebuf.c libultra/src/io/vimgr.c libultra/src/io/visetevent.c
    libultra/src/io/visetmode.c libultra/src/io/visetspecial.c libultra/src/io/viswapbuf.c
    libultra/src/io/viswapcontext.c libultra/src/io/vitbl.c
    libultra/src/libc/ldiv.c libultra/src/libc/ll.c libultra/src/libc/llcvt.c
    libultra/src/libc/rmonPrintf.c libultra/src/libc/string.c libultra/src/libc/xldtob.c
    libultra/src/libc/xlitob.c libultra/src/libc/xprintf.c
    libultra/src/os/createmesgqueue.c libultra/src/os/createthread.c libultra/src/os/destroythread.c
    libultra/src/os/getactivequeue.c libultra/src/os/getthreadpri.c libultra/src/os/gettime.c
    libultra/src/os/initialize.c libultra/src/os/jammesg.c libultra/src/os/recvmesg.c
    libultra/src/os/resetglobalintmask.c libultra/src/os/sendmesg.c libultra/src/os/seteventmesg.c
    libultra/src/os/sethwinterrupt.c libultra/src/os/setthreadpri.c libultra/src/os/settime.c
    libultra/src/os/settimer.c libultra/src/os/startthread.c libultra/src/os/stopthread.c
    libultra/src/os/thread.c libultra/src/os/timerintr.c libultra/src/os/virtualtophysical.c
    libultra/src/os/yieldthread.c libultra/src/sc/sched.c
)

# --- Common flags ---------------------------------------------------------------

set(DKR_DEFINES
    TARGET_PC
    _LANGUAGE_C
    _FINALROM
    NDEBUG
    F3DDKR_GBI
    ${DKR_VERSION_DEFINE}
    AVOID_UB
    NON_MATCHING
    BUILD_VERSION=VERSION_G
    BUILD_VERSION_STRING="2.0G"
    _MIPS_SZLONG=32
    PC_MAIN_POOL_BYTES=${DKR_MAIN_POOL_MB}*1024*1024
    GBI_FLOAT_MTX
    DKR_PORT_VERSION="${DKR_PORT_VERSION}"
    DKR_PORT_COMMIT="${DKR_PORT_COMMIT}"
)
if(DKR_EEPROM_PRESET_100)
    list(APPEND DKR_DEFINES EEPROM_PRESET_100)
endif()
# On Android and iOS, SDL owns the entry point (SDL_main, called from SDL's
# activity / UIApplication delegate); everywhere else we provide a plain main()
# and tell SDL so.
if(NOT ANDROID AND NOT IOS)
    list(APPEND DKR_DEFINES SDL_MAIN_HANDLED)
endif()

set(DKR_C_FLAGS
    $<$<NOT:$<BOOL:${EMSCRIPTEN}>>:-g> # debug info in every build so crash backtraces and debuggers can name lines (not wasm: huge, slow)
    -ffp-contract=off # no fused multiply-add: the N64 FPU has none, and hosts must agree bit for bit
    -fno-builtin
    -fno-common
    -fno-strict-aliasing
    -funsigned-char
    -fwrapv
    -Wno-unused-value
    -Wno-pointer-sign
    -Wno-unknown-attributes
    -Wno-unknown-warning-option
)
if(DKR_SANITIZE)
    list(APPEND DKR_C_FLAGS -fsanitize=address -fno-omit-frame-pointer)
    set(DKR_LINK_FLAGS -fsanitize=address)
endif()

# Include paths shared by the game, libultra and the platform layer. The game
# and libultra sources additionally see include/libc, the N64 libc stubs, ahead
# of the system headers; the platform layer uses the host libc directly.
set(DKR_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PR
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sys
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hasm
    ${CMAKE_CURRENT_SOURCE_DIR}/libultra
    ${CMAKE_CURRENT_SOURCE_DIR}/libultra/src/gu
    ${CMAKE_CURRENT_SOURCE_DIR}/libultra/src/libc
    ${CMAKE_CURRENT_SOURCE_DIR}/libultra/src/io
    ${CMAKE_CURRENT_SOURCE_DIR}/libultra/src/sc
    ${CMAKE_CURRENT_SOURCE_DIR}/libultra/src/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/libultra/src/os
)

function(dkr_configure_target tgt)
    target_compile_definitions(${tgt} PRIVATE ${DKR_DEFINES})
    target_compile_options(${tgt} PRIVATE ${DKR_C_FLAGS})
    target_compile_features(${tgt} PRIVATE c_std_23)
    target_include_directories(${tgt} PRIVATE ${DKR_INCLUDE_DIRS})
    target_link_libraries(${tgt} PRIVATE ${DKR_SDL_TARGET})
endfunction()

add_library(dkr_game OBJECT ${DKR_GAME_SOURCES} ${DKR_LIBULTRA_SOURCES})
dkr_configure_target(dkr_game)
target_include_directories(dkr_game BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/libc)

add_library(dkr_platform OBJECT ${DKR_PLATFORM_SOURCES})
dkr_configure_target(dkr_platform)
target_include_directories(dkr_platform PRIVATE ${DKR_GENERATED_DIR})
add_dependencies(dkr_platform dkr_shaders)

if(ANDROID)
    # The whole game is one shared library, libmain.so, which SDLActivity loads
    # after libSDL3.so and enters through SDL_main. No executable is produced.
    add_library(main SHARED $<TARGET_OBJECTS:dkr_game> $<TARGET_OBJECTS:dkr_platform>)
    target_link_libraries(main PRIVATE ${DKR_SDL_TARGET} m)
elseif(IOS)
    # The iOS app: an .app bundle built by Xcode (`cmake -G Xcode
    # -DCMAKE_SYSTEM_NAME=iOS`, see `make ios`). SDL is static, as on desktop;
    # its UIKit/OpenGLES/CoreAudio frameworks come in through SDL3::SDL3-static.
    # The game data is copied into the bundle's Resources/assets when it exists,
    # where SDL_GetBasePath() points and the desktop asset loader already looks.
    set(DKR_IOS_BUNDLE_ID ${DKR_BUNDLE_ID} CACHE STRING "iOS bundle identifier")
    set(DKR_IOS_TEAM "" CACHE STRING "Apple Development Team ID for automatic signing (empty: unsigned)")
    set(DKR_IOS_ASSETS)
    foreach(f assets.bin assets.lut.bin)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/${f})
            list(APPEND DKR_IOS_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets/${f})
        endif()
    endforeach()
    if(NOT DKR_IOS_ASSETS)
        message(WARNING "assets/assets.bin not found: the iOS app will have no game data (run `make assets` first)")
    endif()
    # ios/app.c is compiled into the app itself: Xcode only links a target that
    # compiles at least one source, and it is where the iOS-only code lives.
    set(DKR_IOS_XCASSETS ${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets) # app icon, launch colour (tools/make-icons.py)
    add_executable(dkracing MACOSX_BUNDLE ios/app.c $<TARGET_OBJECTS:dkr_game> $<TARGET_OBJECTS:dkr_platform>
                   ${DKR_IOS_ASSETS} ${DKR_IOS_XCASSETS})
    dkr_configure_target(dkracing)
    target_link_libraries(dkracing PRIVATE m)
    set_source_files_properties(${DKR_IOS_ASSETS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/assets)
    set_source_files_properties(${DKR_IOS_XCASSETS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(dkracing PROPERTIES
        OUTPUT_NAME "Diddy Kong Racing"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "Diddy Kong Racing"
        MACOSX_BUNDLE_GUI_IDENTIFIER ${DKR_IOS_BUNDLE_ID}
        MACOSX_BUNDLE_BUNDLE_VERSION ${DKR_PORT_BUILD}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${DKR_PORT_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "${DKR_PORT_COPYRIGHT}"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${DKR_IOS_BUNDLE_ID}
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DKR_IOS_TEAM}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_ENABLE_BITCODE NO
        XCODE_ATTRIBUTE_SKIP_INSTALL YES
        XCODE_GENERATE_SCHEME YES)
else()
    add_executable(dkracing $<TARGET_OBJECTS:dkr_game> $<TARGET_OBJECTS:dkr_platform>)
    target_link_libraries(dkracing PRIVATE ${DKR_SDL_TARGET})
    if(MINGW)
        # The MSYS2 MinGW toolchain otherwise leaves a dependency on
        # libwinpthread-1.dll, despite SDL itself being linked statically.
        # Keep the documented Windows artifact usable as a single executable.
        target_link_options(dkracing PRIVATE -static)
    endif()
    if(NOT WIN32)
        target_link_libraries(dkracing PRIVATE m)
    endif()
    if(DKR_LINK_FLAGS)
        target_link_options(dkracing PRIVATE ${DKR_LINK_FLAGS})
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # glibc's backtrace_symbols resolves names through the dynamic symbol table.
        target_link_options(dkracing PRIVATE -rdynamic)
    endif()
endif()

# --- Embedded game data (desktop) -------------------------------------------------
# Mobile and web builds pack assets/ into the app or the page. Desktop builds
# embed the two files into the executable itself (linux/assets_embed.S, an
# .incbin) when they exist at configure time, so a release is one file. CI has
# no game data and builds an executable that looks for assets/ beside it.
if(NOT ANDROID AND NOT IOS AND NOT EMSCRIPTEN)
    option(DKR_EMBED_ASSETS "Embed assets/ into the executable (needs make assets first)" ${DKR_HAS_ASSETS})
    if(DKR_EMBED_ASSETS)
        if(NOT DKR_HAS_ASSETS)
            message(FATAL_ERROR "DKR_EMBED_ASSETS is on but assets/assets.bin is missing: run `make assets` first")
        endif()
        enable_language(ASM)
        set(DKR_EMBED_SRC ${CMAKE_CURRENT_SOURCE_DIR}/linux/assets_embed.S)
        target_sources(dkracing PRIVATE ${DKR_EMBED_SRC})
        set_source_files_properties(${DKR_EMBED_SRC} PROPERTIES
            COMPILE_DEFINITIONS "DKR_ASSETS_LUT_FILE=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.lut.bin\";DKR_ASSETS_BIN_FILE=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.bin\""
            OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.lut.bin;${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.bin")
        target_compile_definitions(dkr_platform PRIVATE DKR_EMBED_ASSETS)
        message(STATUS "Game data embedded in the executable")
    else()
        message(STATUS "Game data NOT embedded: the executable loads assets/ beside it")
    endif()
endif()

# --- Platform metadata (desktop) ----------------------------------------------------
if(WIN32)
    # Icon and version information for dkracing.exe (windows/dkracing.rc.in).
    enable_language(RC)
    file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/windows/dkracing.ico" DKR_WINDOWS_ICON)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/windows/dkracing.rc.in ${CMAKE_CURRENT_BINARY_DIR}/dkracing.rc @ONLY)
    target_sources(dkracing PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/dkracing.rc)
elseif(APPLE AND NOT IOS)
    # The app bundle's Info.plist, which `make macos-package` wraps around the executable.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # AppStream metadata for the AppImage (`make linux-package`).
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/linux/packaging/com.bluesentinelsec.dkracing.metainfo.xml.in
                   ${CMAKE_CURRENT_BINARY_DIR}/com.bluesentinelsec.dkracing.metainfo.xml @ONLY)
endif()

# --- Browser build (Emscripten) ------------------------------------------------
# Configure with `emcmake cmake ...` (see docker/web.Dockerfile and `make web`).
# The game loop blocks in pc_retrace_wait, so ASYNCIFY turns those waits into
# yields to the browser; the renderer's GL ES 3.0 path becomes WebGL 2. The
# assets are packed into dkracing.data next to the page when assets/ exists
# (CI has no game data, so it only checks that the code builds).
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(DKR_WEB_SHELL ${CMAKE_CURRENT_SOURCE_DIR}/linux/web/shell.html)
    target_link_options(dkracing PRIVATE
        -sASYNCIFY
        -sASYNCIFY_STACK_SIZE=1048576
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=128MB
        -sSTACK_SIZE=4MB
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sGL_ENABLE_GET_PROC_ADDRESS=1
        -sENVIRONMENT=web
        -sEXIT_RUNTIME=0
        -lidbfs.js
        --shell-file ${DKR_WEB_SHELL})
    set_property(TARGET dkracing APPEND PROPERTY LINK_DEPENDS ${DKR_WEB_SHELL})
    # The page's icon and web-app manifest, served beside it.
    foreach(f favicon.png icon-192.png icon-512.png manifest.webmanifest)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/linux/web/${f} ${CMAKE_CURRENT_BINARY_DIR}/${f} COPYONLY)
    endforeach()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.bin)
        target_link_options(dkracing PRIVATE --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets)
        set_property(TARGET dkracing APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.bin)
    else()
        message(WARNING "assets/assets.bin not found: the web build will have no game data (run `make assets` first)")
    endif()
endif()

# --- Unit tests ----------------------------------------------------------------
if(DKR_BUILD_TESTS)
    enable_testing()
    add_executable(test_pc_asset tests/test_pc_asset.c linux/pc_asset.c)
    target_include_directories(test_pc_asset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/PR
                               ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_compile_definitions(test_pc_asset PRIVATE TARGET_PC _LANGUAGE_C)
    add_test(NAME pc_asset COMMAND test_pc_asset)
endif()
