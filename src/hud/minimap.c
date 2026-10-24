#include "hud_internal.h"

/**
 * Loads the minimap sprite from the level data, then sets the global colours for the minimap.
 */
void minimap_init(LevelModel *model) {
    s32 sp2C;
    gMinimapRed = (model->minimapColor >> 16) & 0xFF;
    gMinimapGreen = (model->minimapColor >> 8) & 0xFF;
    gMinimapBlue = model->minimapColor & 0xFF;
    load_sprite_info(model->minimapSpriteIndex, &gMinimapDotOffsetX, &gMinimapDotOffsetY, &sp2C, &sp2C, &sp2C);
    sprite_get_size(model->minimapSpriteIndex, 0, &gMinimapSpriteWidth, &gMinimapSpriteHeight, &sp2C);
    model->minimapSpriteIndex = (ptrint_t) tex_load_sprite(model->minimapSpriteIndex, 1);
}

/**
 * Set the position of the minimap marker based on object position.
 * Some markers can rotate, so account for that, too.
 */
void minimap_marker_pos(f32 x, f32 z, f32 angleSin, f32 angleCos, f32 modelAspectRatio) {
    LevelModel *lvlMdl;
    UNUSED s32 pad24[2];
    f32 scaledX;
    f32 scaledY;
    s32 a;
    s32 b;

    lvlMdl = get_current_level_model();

    a = lvlMdl->upperXBounds - lvlMdl->lowerXBounds;
    b = lvlMdl->upperZBounds - lvlMdl->lowerZBounds;
    scaledX = (60.0f * modelAspectRatio * lvlMdl->minimapXScale * (x - lvlMdl->lowerXBounds)) / a;
    scaledY = (lvlMdl->minimapYScale * -60.0f * (z - lvlMdl->lowerZBounds)) / b;

    if (get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) { // Is adventure 2?
        gCurrentHud->entry[HUD_MINIMAP_MARKER].pos.x =
            (gMinimapScreenX - ((scaledX * angleCos) + (scaledY * angleSin))) + lvlMdl->minimapOffsetXAdv2 -
            gMinimapDotOffsetX;
        gCurrentHud->entry[HUD_MINIMAP_MARKER].pos.y = (lvlMdl->minimapOffsetYAdv2 + gMinimapScreenY) -
                                                       ((scaledX * angleSin) - (scaledY * angleCos)) +
                                                       gMinimapDotOffsetY;
        return;
    }
    // Final x position on minimap
    gCurrentHud->entry[HUD_MINIMAP_MARKER].pos.x = gMinimapScreenX + ((scaledX * angleCos) + (scaledY * angleSin)) +
                                                   (f32) lvlMdl->minimapOffsetXAdv1 - gMinimapDotOffsetX;
    // Final y position on minimap
    gCurrentHud->entry[HUD_MINIMAP_MARKER].pos.y = (lvlMdl->minimapOffsetYAdv1 + gMinimapScreenY) -
                                                   ((scaledX * angleSin) - (scaledY * angleCos)) + gMinimapDotOffsetY;
}

/**
 * Begin fading the minimap which will slowly set it to whatever the opacity target is.
 * Fades in or out based on what the setting is.
 */
void minimap_fade(s32 setting) {
    gMinimapFade = TRUE;
    gMinimapXlu = setting;
}

/**
 * Snaps the minimap opacity to the target level.
 * Fades in or out based on what the setting is.
 */
void minimap_opacity_set(s32 setting) {
    gMinimapOpacity = gMinimapOpacityTarget;
    gMinimapXlu = setting;
}
