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
 * Wizpig ship initilisation.
 * Does nothing.
 */
void obj_init_wizpigship(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

/**
 * Wizpig ship loop behaviour.
 * Periodically shoots laserbeams in front of it.
 */
void obj_loop_wizpigship(Object *wizShipObj, s32 updateRate) {
    s32 i;
    s32 index;
    f32 posX;
    f32 posY;
    f32 posZ;
    Object *newObj;
    ObjectModel *wizShipModel;
    MtxF shipMtx;
    MtxF laserMtx;
    LevelObjectEntryCommon newObject;
    ObjectTransform trans;

    anim_object_follow_path(wizShipObj, updateRate, wizShipObj);
    if (wizShipObj->modelInstances[0] != NULL) {
        wizShipModel = wizShipObj->modelInstances[0]->objModel;
        if (wizShipObj->properties.wizpigship.timer > 0) {
            wizShipObj->properties.wizpigship.timer -= updateRate;
        } else {
            wizShipObj->properties.wizpigship.timer = 0;
        }
        if (wizShipObj->attachPoints != NULL && wizShipObj->properties.wizpigship.timer == 0) {
            if (wizShipObj->particleEmittersEnabled & OBJ_EMIT_1) {
                wizShipObj->properties.wizpigship.timer = 20;
                mtxf_from_transform(&shipMtx, &wizShipObj->trans);
                trans.x_position = 0.0f;
                trans.y_position = 0.0f;
                trans.z_position = 0.0f;
                trans.scale = 1.0f;
                trans.rotation.y_rotation = wizShipObj->trans.rotation.y_rotation;
                trans.rotation.x_rotation = wizShipObj->trans.rotation.x_rotation;
                trans.rotation.z_rotation = 0;
                mtxf_from_transform(&laserMtx, &trans);

                for (i = 0; i < wizShipObj->attachPoints->count; i++) {
                    index = wizShipObj->attachPoints->unk2C[i];
                    if ((index >= 0) && (index < wizShipModel->numberOfAttachPoints)) {
                        if (wizShipObj->curVertData != NULL) {
                            posX = wizShipObj->curVertData[wizShipModel->attachPoints[index]].x;
                            posY = wizShipObj->curVertData[wizShipModel->attachPoints[index]].y;
                            posZ = wizShipObj->curVertData[wizShipModel->attachPoints[index]].z;
                            mtxf_transform_point(shipMtx, posX, posY, posZ, &posX, &posY, &posZ);
                            newObject.x = posX;
                            newObject.y = posY;
                            newObject.z = posZ;
                            newObject.size = sizeof(LevelObjectEntryCommon);
                            newObject.objectID = ASSET_OBJECT_ID_LASERBOLT;
                            newObj = spawn_object(&newObject, OBJECT_SPAWN_UNK01);
                            if (newObj != NULL) {
                                newObj->level_entry = NULL;
                                newObj->trans.rotation.y_rotation = wizShipObj->trans.rotation.y_rotation + 0x8000;
                                newObj->trans.rotation.x_rotation = -wizShipObj->trans.rotation.x_rotation;
                                newObj->properties.laserbolt.timer = 60;
                                mtxf_transform_point(laserMtx, 0.0f, 0.0f, -30.0f, &newObj->x_velocity,
                                                     &newObj->y_velocity, &newObj->z_velocity);
                                audspat_play_sound_at_position(
                                    SOUND_LASER_GUN, wizShipObj->trans.x_position, wizShipObj->trans.y_position,
                                    wizShipObj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                            }
                        }
                    }
                }
            }
        }
    }
}
