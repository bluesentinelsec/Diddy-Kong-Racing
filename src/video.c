#include "video.h"
#include "host.h"
#include "macros.h"
#include "memory.h"
#include "types.h"

/************ .data ************/

u16 *gVideoDepthBuffer = NULL;

VideoModeResolution gVideoModeResolutions[] = {
    { SCREEN_WIDTH, SCREEN_HEIGHT },                   // 320x240
    { SCREEN_WIDTH, SCREEN_HEIGHT },                   // 320x240
    { HIGH_RES_SCREEN_WIDTH, SCREEN_HEIGHT },          // 640x240
    { HIGH_RES_SCREEN_WIDTH, SCREEN_HEIGHT },          // 640x240
    { HIGH_RES_SCREEN_WIDTH, HIGH_RES_SCREEN_HEIGHT }, // 640x480
    { HIGH_RES_SCREEN_WIDTH, HIGH_RES_SCREEN_HEIGHT }, // 640x480
    { HIGH_RES_SCREEN_WIDTH, HIGH_RES_SCREEN_HEIGHT }, // 640x480
    { HIGH_RES_SCREEN_WIDTH, HIGH_RES_SCREEN_HEIGHT }, // 640x480
};

// This value exists in order to make sure there are no out of bounds accesses of gVideoModeResolutions
#define NUM_RESOLUTION_MODES ((s32) (sizeof(gVideoModeResolutions) / sizeof(VideoModeResolution)) - 1)

/*******************************/

/************ .bss ************/

s32 gVideoRefreshRate; // Official Name: viFramesPerSecond
f32 gVideoAspectRatio;
f32 gVideoHeightRatio;
s32 gVideoFbWidths[2];
s32 gVideoFbHeights[2];
u16 *gVideoFramebuffers[2];
s32 gVideoCurrFbIndex;
s32 gVideoModeIndex;
s32 sBlackScreenTimer;
u16 *gVideoCurrFramebuffer; // Official Name: currentScreen
u16 *gVideoLastFramebuffer; // Official Name: otherScreen
u16 *gVideoCurrDepthBuffer;
u16 *gVideoLastDepthBuffer;
u8 gVideoMaxUpdateRate;
u8 gVideoDeltaCounter;
u8 gVideoDeltaTime;

/******************************/

/**
 * Set up the framebuffers and the VI.
 * Framebuffers are allocated at runtime.
 * Official Name: viInit
 */
void video_init(s32 videoModeIndex) {
    if (osTvType == OS_TV_TYPE_PAL) {
        gVideoRefreshRate = REFRESH_50HZ;
        gVideoAspectRatio = ASPECT_RATIO_PAL;
        gVideoHeightRatio = HEIGHT_RATIO_PAL;
    } else if (osTvType == OS_TV_TYPE_MPAL) {
        gVideoRefreshRate = REFRESH_60HZ;
        gVideoAspectRatio = ASPECT_RATIO_MPAL;
        gVideoHeightRatio = HEIGHT_RATIO_MPAL;
    } else {
        gVideoRefreshRate = REFRESH_60HZ;
        gVideoAspectRatio = ASPECT_RATIO_NTSC;
        gVideoHeightRatio = HEIGHT_RATIO_NTSC;
    }

    if (osTvType == OS_TV_TYPE_PAL) {
        s32 i;
        for (i = 0; i <= NUM_RESOLUTION_MODES; i++) {
            gVideoModeResolutions[i].height += PAL_HEIGHT_DIFFERENCE;
        }
    }

    video_delta_reset();
    fb_mode_set(videoModeIndex);
    gVideoFramebuffers[0] = NULL;
    gVideoFramebuffers[1] = NULL;
    fb_alloc(0);
    fb_alloc(1);
    gVideoCurrFbIndex = 1;
    fb_swap();
    fb_init_vi();
    sBlackScreenTimer = 12;
    gVideoDeltaCounter = 0;
    gVideoMaxUpdateRate = 3;
}

/**
 * Set the current video mode to the id specified.
 */
void fb_mode_set(s32 videoModeIndex) {
    gVideoModeIndex = videoModeIndex;
}

/**
 * Unused function that would return the current video mode index.
 */
UNUSED s32 fb_mode(void) {
    return gVideoModeIndex;
}

/**
 * Unused function that would change the framebuffer dimensions.
 * Since only one kind of video mode is ever used, this function is never called.
 */
UNUSED void fb_mode_size(s32 fbIndex) {
    gVideoFbWidths[fbIndex] = gVideoModeResolutions[gVideoModeIndex & NUM_RESOLUTION_MODES].width;
    gVideoFbHeights[fbIndex] = gVideoModeResolutions[gVideoModeIndex & NUM_RESOLUTION_MODES].height;
}

/**
 * Return the current framebuffer dimensions as a single s32 value.
 * The high 16 bits are the height of the frame, and the low 16 bits are the width.
 * Official Name: viGetCurrentSize
 */
s32 fb_size(void) {
    return (gVideoFbHeights[gVideoCurrFbIndex] << 16) | gVideoFbWidths[gVideoCurrFbIndex];
}

/**
 * Initialise the VI settings.
 * It first checks the TV type ad then will set the properties of the VI
 * depending on the gVideoModeIndex value.
 * Most of these go unused, as the value is always 1.
 * Official Name: viSetTiming
 */
void fb_init_vi(void) {
    static const char *const sModeNames[] = {
        "320 by 240 Point sampled, Non interlaced",
        "320 by 240 Anti-aliased, Non interlaced",
        "640 by 240 Point sampled, Non interlaced",
        "640 by 240 Anti-aliased, Non interlaced",
        "640 by 480 Point sampled, Interlaced",
        "640 by 480 Anti-aliased, Interlaced",
        "640 by 480 Point sampled, Interlaced, De-flickered",
        "640 by 480 Anti-aliased, Interlaced, De-flickered",
    };

    // The N64 programmed the video interface's timing registers here. The host
    // draws the display list itself at whatever size the window has, so the
    // mode only decides the framebuffer size the game lays out.
    stubbed_printf("%s.\n", sModeNames[gVideoModeIndex & NUM_RESOLUTION_MODES]);
}

/**
 * Allocate the selected framebuffer index from the main pool.
 * Will also allocate the depthbuffer if it does not already exist.
 * Framebuffers should be 64 bit aligned, but since the memory allocator
 * already aligns by 16, it only needs 48 bits of alignment in addition.
 */
void fb_alloc(s32 index) {
    if (gVideoFramebuffers[index] != 0) {
        mempool_locked_unset((u8 *) gVideoFramebuffers[index]); // Effectively unused.
        mempool_free(gVideoFramebuffers[index]);
    }
    gVideoFbWidths[index] = gVideoModeResolutions[gVideoModeIndex & NUM_RESOLUTION_MODES].width;
    gVideoFbHeights[index] = gVideoModeResolutions[gVideoModeIndex & NUM_RESOLUTION_MODES].height;
    if (gVideoModeIndex >= VIDEO_MODE_MIDRES_MASK) {
        gVideoFramebuffers[index] =
            mempool_alloc_safe((HIGH_RES_SCREEN_WIDTH * HIGH_RES_SCREEN_HEIGHT * 2) + 0x30, COLOUR_TAG_WHITE);
        gVideoFramebuffers[index] = FBALIGN(gVideoFramebuffers[index]);
        if (gVideoDepthBuffer == NULL) {
            gVideoDepthBuffer =
                mempool_alloc_safe((HIGH_RES_SCREEN_WIDTH * HIGH_RES_SCREEN_HEIGHT * 2) + 0x30, COLOUR_TAG_WHITE);
            gVideoDepthBuffer = FBALIGN(gVideoDepthBuffer);
        }
    } else {
        gVideoFramebuffers[index] =
            mempool_alloc_safe((gVideoFbWidths[index] * gVideoFbHeights[index] * 2) + 0x30, COLOUR_TAG_WHITE);
        gVideoFramebuffers[index] = FBALIGN(gVideoFramebuffers[index]);
        if (gVideoDepthBuffer == NULL) {
            gVideoDepthBuffer =
                mempool_alloc_safe((gVideoFbWidths[index] * gVideoFbHeights[index] * 2) + 0x30, COLOUR_TAG_WHITE);
            gVideoDepthBuffer = FBALIGN(gVideoDepthBuffer);
        }
    }
}

/**
 * Sets the video counters to their default values.
 * Another renmant from an unused system.
 * Official Name: viFrameRateReset
 */
void video_delta_reset(void) {
    gVideoDeltaCounter = 0;
    gVideoDeltaTime = LOGIC_30FPS;
}

/**
 * Wait for the finished message from the scheduler while counting up a timer,
 * then update the current framebuffer index.
 * This function also has a section where it counts a timer that goes no higher
 * than an update magnitude of 2. It's only purpose is to be used as a divisor
 * in the unused function, vi_refresh_rate.
 * Official Name: viFrameSync
 */
s32 fb_update(s32 mesg) {
    u8 tempUpdateRate;

    tempUpdateRate = LOGIC_60FPS;
    if (sBlackScreenTimer) {
        sBlackScreenTimer--;
    }
    if (mesg != MESG_SKIP_BUFFER_SWAP) {
        fb_swap();
    }
    // The N64 blocked on the video interface's retrace messages here. The host
    // paces the frame to the 60 Hz clock and returns how many retrace periods
    // it covered, which is what the game used the message count for.
    tempUpdateRate = pc_retrace_wait();
    return tempUpdateRate;
}

/** Caps how many logic steps one frame may cover when the game runs slowly (3 by default). */
void vi_set_max_update_rate(u8 arg0) {
    gVideoMaxUpdateRate = arg0;
}

/**
 * Unused function that returns the refresh rate, after performance.
 * A fully performant game would return 60.
 * Perhaps may have been used originally to calculate the factor in which to handle frameskipping with.
 */
UNUSED s32 vi_refresh_rate(void) {
    return (s32) ((f32) gVideoRefreshRate / (f32) gVideoDeltaTime);
}

/**
 * Flips the current framebuffer index, swapping to the other framebuffer
 * for the next frame, then update the current and previous framebuffer pointers.
 */
void fb_swap(void) {
    gVideoLastFramebuffer = gVideoFramebuffers[gVideoCurrFbIndex];
    gVideoLastDepthBuffer = gVideoDepthBuffer;
    gVideoCurrFbIndex ^= 1;
    gVideoCurrFramebuffer = gVideoFramebuffers[gVideoCurrFbIndex];
    gVideoCurrDepthBuffer = gVideoDepthBuffer;
}

/**
 * Copy byte-by-byte a region from one address to another.
 */
void fb_memcpy(u8 *src, u8 *dest, s32 len) {
    s32 i;

    for (i = 0; i < len; i++) {
        *dest++ = *src++;
    }
}
