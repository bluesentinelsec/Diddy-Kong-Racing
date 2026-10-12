cmake_minimum_required(VERSION 3.24)
project(dkracing C)

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
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG ${DKR_SDL3_TAG}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(SDL3)
message(STATUS "SDL3 ${DKR_SDL3_TAG} via FetchContent, static")

# Shaders are embedded as byte arrays so the executable is self-contained.
set(DKR_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linux/shaders)
set(DKR_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${DKR_GENERATED_DIR})
set(DKR_SHADER_HEADERS)
foreach(shader dkr.vert dkr.frag)
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
    linux/reimpl.c
    linux/pc_asset.c
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
    SDL_MAIN_HANDLED
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
    AUDIO_NO_REVERB
)
if(DKR_EEPROM_PRESET_100)
    list(APPEND DKR_DEFINES EEPROM_PRESET_100)
endif()

set(DKR_C_FLAGS
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
    target_link_libraries(${tgt} PRIVATE SDL3::SDL3-static)
endfunction()

add_library(dkr_game OBJECT ${DKR_GAME_SOURCES} ${DKR_LIBULTRA_SOURCES})
dkr_configure_target(dkr_game)
target_include_directories(dkr_game BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/libc)

add_library(dkr_platform OBJECT ${DKR_PLATFORM_SOURCES})
dkr_configure_target(dkr_platform)
target_include_directories(dkr_platform PRIVATE ${DKR_GENERATED_DIR})
add_dependencies(dkr_platform dkr_shaders)

add_executable(dkracing $<TARGET_OBJECTS:dkr_game> $<TARGET_OBJECTS:dkr_platform>)
target_link_libraries(dkracing PRIVATE SDL3::SDL3-static)
if(NOT WIN32)
    target_link_libraries(dkracing PRIVATE m)
endif()
if(DKR_LINK_FLAGS)
    target_link_options(dkracing PRIVATE ${DKR_LINK_FLAGS})
endif()
