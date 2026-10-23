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
 * AI node init func.
 * Adds itself to the node list, and then tells the game AI nodes exist.
 */
void obj_init_ainode(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_AiNode *entry = &levelEntry->aiNode;
    if (entry->nodeID == NODE_NONE) {
        entry->nodeID = ainode_register(obj) & NODE_NONE;
    }
    ainode_tail_set(entry->nodeID);
    ainode_enable();
}

/**
 * AI node loop func.
 * Does nothing. All the relevant behaviour is handled globally.
 */
void obj_loop_ainode(UNUSED Object *obj, UNUSED s32 updateRate) {
}
