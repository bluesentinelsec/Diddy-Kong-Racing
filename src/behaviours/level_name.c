#include "behaviours.h"

#include "audio.h"
#include "audio_spatial.h"
#include "audio_vehicle.h"
#include "audiosfx.h"
#include "camera.h"
#include "collision.h"
#include "common.h"
#include "fade_transition.h"
#include "font.h"
#include "game.h"
#include "game_text.h"
#include "game_ui.h"
#include "host.h"
#include "joypad.h"
#include "lights.h"
#include "macros.h"
#include "math_util.h"
#include "menu.h"
#include "object_models.h"
#include "objects.h"
#include "particles.h"
#include "printf.h"
#include "racer.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"
#include "video.h"
#include "waves.h"
#include "weather.h"

/**
 * Overworld level name init behaviour.
 * Sets level name and radius based off spawn info.
 */
void obj_init_levelname(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_LevelName *entry = &levelEntry->levelName;
    obj->properties.levelName.radius = entry->radius * 8.0f;
    obj->properties.levelName.radius *= obj->properties.levelName.radius;
    obj->properties.levelName.levelID = entry->levelID;
    obj->properties.levelName.opacity = 0;
    if (is_in_tracks_mode()) {
        free_object(obj);
    }
    clear_dialogue_box_open_flag(4);
}

/**
 * Overworld level name loop behaviour.
 * Placed at a level entrance, when a player gets near, display the level name on screen.
 */
void obj_loop_levelname(Object *obj, s32 updateRate) {
    s32 textWidth;
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;
    UNUSED s32 pad[5];
    char *levelName;
    Object *racerObj;
    f32 diffX;
    f32 diffZ;
    ObjPropertyName *properties;

    racerObj = get_racer_object_by_port(PLAYER_ONE);
    if (racerObj != NULL) {
        diffX = obj->trans.x_position - racerObj->trans.x_position;
        diffZ = obj->trans.z_position - racerObj->trans.z_position;
        properties = &obj->properties.levelName;
        if ((diffX * diffX) + (diffZ * diffZ) < properties->radius) {
            properties->opacity += updateRate * 16;
            if (properties->opacity > 256) {
                properties->opacity = 256;
            }
        } else {
            properties->opacity -= updateRate * 16;
            if (properties->opacity < 0) {
                properties->opacity = 0;
            }
        }
        if (properties->opacity > 0) {
            levelName = level_name(properties->levelID);
            textWidth = (get_text_width(levelName, 0, 0) + 24) >> 1;
            x1 = SCREEN_WIDTH_HALF - textWidth;
            x2 = textWidth + SCREEN_WIDTH_HALF;
            if (osTvType == OS_TV_TYPE_PAL) {
                y1 = SCREEN_HEIGHT - 16;
                y2 = SCREEN_HEIGHT - 16 + 24;
            } else {
                y1 = SCREEN_HEIGHT - 38;
                y2 = SCREEN_HEIGHT - 38 + 20;
            }
            dialogue_clear(4);
            set_current_dialogue_box_coords(4, x1, y1, x2, y2);
            set_current_dialogue_background_colour(4, 128, 64, 128, (properties->opacity * 160) >> 8);
            set_current_text_background_colour(4, 0, 0, 0, 0);
#if REGION == REGION_JP
            set_dialogue_font(4, FONT_SMALL);
            set_current_text_colour(4, 255, 255, 64, 255, (properties->opacity * 255) >> 8);
            render_dialogue_text(4, (x2 - x1) >> 1, ((y2 - y1) >> 1), levelName, 1, ALIGN_MIDDLE_CENTER);
#else
            set_dialogue_font(4, FONT_COLOURFUL);
            set_current_text_colour(4, 255, 255, 255, 0, (properties->opacity * 255) >> 8);
            render_dialogue_text(4, (x2 - x1) >> 1, ((y2 - y1) >> 1) + 2, levelName, 1, ALIGN_MIDDLE_CENTER);
#endif
            open_dialogue_box(4);
        }
    }
}
