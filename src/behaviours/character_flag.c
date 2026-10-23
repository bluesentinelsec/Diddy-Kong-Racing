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
#include "PR/os_cont.h"
#include "PRinternal/viint.h"
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

Vertex gCharacterFlagVertices[4] = {
    { -256, 256, 0, 255, 255, 255, 255 },
    { 256, 256, 0, 255, 255, 255, 255 },
    { 256, -256, 0, 255, 255, 255, 255 },
    { -256, -256, 0, 255, 255, 255, 255 },
};

/**
 * Character World Portrait init func.
 * Sets the scale, but the actual geometry data is built during the update loop.
 * These are seen in the challenge maps.
 */
void obj_init_characterflag(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_CharacterFlag *entry = &levelEntry->characterFlag;
    f32 radius;
    obj->properties.characterFlag.playerID = entry->playerIndex;
    obj->properties.characterFlag.characterID = -1; // Set to -1 so the loop func builds the gfx data.
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    radius = entry->radius & 0xFF;
    if (radius < 10.0f) {
        radius = 10.0f;
    }
    radius /= 64;
    obj->trans.scale = obj->header->scale * radius;
}

/**
 * Character World Portrait loop func.
 * If there's no associated character ID, build the portrait geometry
 * and set the texture based on the character ID, then write the character ID.
 */
void obj_loop_characterflag(Object *obj, UNUSED s32 updateRate) {
    s32 temp_t4;
    s32 temp_t5;
    Object *racerObj;
    CharacterFlagModel *flagModel;
    Object_Racer *racer;

    if (obj->properties.characterFlag.characterID < 0) {
        racerObj = get_racer_object(obj->properties.characterFlag.playerID);
        if (racerObj != NULL) {
            flagModel = obj->characterFlagModel;
            racer = racerObj->racer;
            obj->properties.characterFlag.characterID = racer->characterId;
            if (obj->properties.characterFlag.characterID < 0 ||
                obj->properties.characterFlag.characterID >= NUMBER_OF_CHARACTERS) {
                obj->properties.characterFlag.characterID = 0;
            }
            flagModel->vertices = gCharacterFlagVertices;
            flagModel->texture = obj->textures[obj->properties.characterFlag.characterID];
            temp_t4 = (flagModel->texture->width - 1) << 21;
            temp_t5 = (flagModel->texture->height - 1) << 5;
            // 0x40 = Draw backface
            flagModel->triangles[0].vertices = DKR_TRIANGLE(BACKFACE_DRAW, 0, 1, 3);
            flagModel->triangles[0].uv0.texCoords = 0;
            flagModel->triangles[0].uv1.texCoords = temp_t4;
            flagModel->triangles[0].uv2.texCoords = temp_t5;
            flagModel->triangles[1].vertices = DKR_TRIANGLE(BACKFACE_DRAW, 1, 2, 3);
            flagModel->triangles[1].uv0.texCoords = temp_t4;
            flagModel->triangles[1].uv1.texCoords = (temp_t4 | temp_t5);
            flagModel->triangles[1].uv2.texCoords = temp_t5;
        }
    }
}

void obj_render_characterflag(Object *obj) {
    CharacterFlagModel *characterFlagModel;

    if (obj->properties.characterFlag.characterID >= 0) {
        characterFlagModel = obj->characterFlagModel;
        render_misc_model(obj, characterFlagModel->vertices, 4, characterFlagModel->triangles, 2,
                          characterFlagModel->texture, RENDER_ANTI_ALIASING | RENDER_Z_COMPARE | RENDER_FOG_ACTIVE, 0,
                          1.0f);
    }
}
