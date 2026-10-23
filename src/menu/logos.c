#include "menu_internal.h"

static f32 sBootScreenTimer;

// Fade transition from the logo screen to the title screen.
static FadeTransition gFadeLogoToTitleScreen =
    FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_NONE, FADE_COLOR_BLACK, 120, FADE_STAY);

static char gRareCopyrightString[24] = "(C) COPYRIGHT RARE 1997";

/**
 * Initialises the viewport and background settings for the screen with the rotating N64 and Rareware Logo.
 * It then sets a 16 second timer that will count down.
 */
void menu_logos_screen_init(void) {
    gMenuInput.delay = 0;
    sBootScreenTimer = 16.0f;
    bgdraw_fillcolour(0, 0, 0);
    if (osTvType == OS_TV_TYPE_PAL) {
        viewport_menu_set(0, 0, 38, SCREEN_WIDTH, SCREEN_HEIGHT - 16);
        set_viewport_properties(0, VIEWPORT_AUTO, VIEWPORT_AUTO, SCREEN_WIDTH, SCREEN_HEIGHT_PAL);
    } else {
        viewport_menu_set(0, 0, 40, SCREEN_WIDTH, SCREEN_HEIGHT - 44);
        set_viewport_properties(0, VIEWPORT_AUTO, VIEWPORT_AUTO, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    copy_viewports_to_stack(); // Init viewports
    camEnableUserView(0, TRUE);
}

/**
 * Handler of everything that happens in the space of those 16 seconds.
 */
s32 menu_logo_screen_loop(s32 updateRate) {
    s32 opacity;
    s32 yOffset;
    s32 yOffsetShadow;

    if (osTvType == OS_TV_TYPE_PAL) {
        yOffset = 26;
        if (sBootScreenTimer < 2.6f && gMenuInput.delay == 0) {
            transition_begin(&gFadeLogoToTitleScreen);
            gMenuInput.delay = 1;
        }
        sBootScreenTimer -= updateRate / 50.0f;
    } else {
        yOffset = 0;
        if (sBootScreenTimer < 2.17f && gMenuInput.delay == 0) {
            transition_begin(&gFadeLogoToTitleScreen);
            gMenuInput.delay = 1;
        }
        sBootScreenTimer -= updateRate / 60.0f;
    }
    if (sBootScreenTimer <= 0.0f) {
        camDisableUserView(0, FALSE);
        set_viewport_properties(0, VIEWPORT_AUTO, VIEWPORT_AUTO, VIEWPORT_AUTO, VIEWPORT_AUTO);
        init_title_screen_variables();
        menu_init(MENU_TITLE);
    }
    // This uses doubles instead of floats for precision in the counting.
    if (sBootScreenTimer < 8.5) {
        set_text_font(ASSET_FONTS_SMALLFONT);
        set_text_background_colour(0, 0, 0, 0);
        if ((sBootScreenTimer < 8.0) && (sBootScreenTimer >= 7.5)) {
            opacity = (8.0 - sBootScreenTimer) * 510.0;
        } else {
            if ((sBootScreenTimer < 7.5) && (sBootScreenTimer >= 7.0)) {
                opacity = (sBootScreenTimer - 7.0) * 510.0;
            } else {
                opacity = 0;
            }
        }
        if (opacity != 0) {
            set_text_colour(255, 255, 0, 255, opacity);
            draw_text(&gMenuFrame.displayList, 159, yOffset + 212, gRareCopyrightString, ALIGN_MIDDLE_CENTER);
            draw_text(&gMenuFrame.displayList, 161, yOffset + 212, gRareCopyrightString, ALIGN_MIDDLE_CENTER);
            draw_text(&gMenuFrame.displayList, POS_CENTRED, yOffset + 211, gRareCopyrightString, ALIGN_MIDDLE_CENTER);
            draw_text(&gMenuFrame.displayList, POS_CENTRED, yOffset + 213, gRareCopyrightString, ALIGN_MIDDLE_CENTER);
        }
        yOffsetShadow = yOffset + 212;
        if (sBootScreenTimer > 8.0) {
            opacity = (8.5 - sBootScreenTimer) * 510.0;
        } else {
            opacity = 0xFF;
        }
        set_text_colour(0xFF, 0xFF, 0xFF, 0xFF, opacity);
        draw_text(&gMenuFrame.displayList, POS_CENTRED, yOffsetShadow, gRareCopyrightString, ALIGN_MIDDLE_CENTER);
    }
    return MENU_RESULT_CONTINUE;
}
