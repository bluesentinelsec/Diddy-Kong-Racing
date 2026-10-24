#ifndef DKR_RACER_INTERNAL_H
#define DKR_RACER_INTERNAL_H

/*
 * Shared between the files of src/racer/: the includes they all use, the racer state they
 * share (defined in racer.c) and the functions one file calls in another.
 */
#include "asset_enums.h"
#include "asset_loading.h"
#include "audio.h"
#include "audio_spatial.h"
#include "audio_vehicle.h"
#include "audiosfx.h"
#include "behaviours.h"
#include "collision.h"
#include "common.h"
#include "fade_transition.h"
#include "game.h"
#include "game_ui.h"
#include "host.h"
#include "joypad.h"
#include "libc/string.h"
#include "macros.h"
#include "math_util.h"
#include "memory.h"
#include "menu.h"
#include "object_models.h"
#include "objects.h"
#include "particles.h"
#include "printf.h"
#include "racer.h"
#include "save_data.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"
#include "vehicle_misc.h"
#include "video.h"

extern s32 gNumViewports;
extern f32 gSurfaceTractionTable[19];
extern f32 gSurfaceSlideTable[19];
extern s32 gSurfaceBobbingTable[19];
extern s32 gSurfaceSoundTable[19];
extern f32 gCurrentCourseHeight;
extern Vec3f gCurrentRacerWaterPos;
extern s8 gRacerWaveType;
extern Camera *gCameraObject;
extern ObjectTransform gCurrentRacerTransform;
extern u32 gCurrentRacerInput;
extern u32 gCurrentButtonsPressed;
extern u32 gCurrentButtonsReleased;
extern s32 gCurrentStickX;
extern s32 gCurrentStickY;
extern s32 unused_8011D53C;
extern s32 gRaceStartTimer;
extern f32 gAiStartDelay;
extern f32 gRacerExternalShiftX;
extern f32 gRacerExternalShiftZ;
extern u16 gRacerSurfaceSoundId;
extern u16 gRacerSurfaceSoundVolume;
extern s32 gCurrentCarSteerVel;
extern s32 gRacerTargetRoll;
extern s32 gCurrentPlayerIndex;
extern f32 *gCurrentRacerMiscAssetPtr;
extern f32 *gRacerWheelTable;
extern f32 gCurrentRacerWeightStat;
extern f32 gCurrentRacerHandlingStat;
extern f32 gCurrentRacerUnusedMiscAsset11;
extern f32 gRacerMagnetVelX;
extern f32 gRacerMagnetVelZ;
extern s8 gCurrentSurfaceType;
extern s8 gTajInteractStatus;
extern s8 gRacerDialogueCamera;
extern s8 gRacerInputBlocked;
extern s8 gStartBoostTime;
extern s16 gDialogueCameraAngle;
extern s8 gEggChallengeFlags[4];
extern s8 gAiEggCounts[4];
extern GhostNode *gGhostData[3];
extern s8 gCurrentGhostIndex;
extern s8 gPrevGhostNodeIndex;
extern s16 gGhostNodeDelay;
extern s16 gGhostNodeCount[3];
extern s16 gGhostNodeFull[2];
extern s16 gGhostMapID;
extern s8 gRacerWaveCount;
extern WaterProperties **gRacerCurrentWave;
extern s8 gAiRacerElevations[4];
extern s16 gAiCatchupTimer;

void pc_swap_racer_tables(ObjectHeader *header);
void increment_ai_behaviour_chances(Object *obj, Object_Racer *racer, s32 updateRate);
void racer_AI_pathing_inputs(Object *obj, Object_Racer *racer, s32 updateRate);
s32 ease_step(s32 diff, f32 rate, f32 divisor);
void racer_ease_throttle(Object_Racer *racer, f32 updateRateF, f64 step);
s32 racer_ease_brake(Object_Racer *racer, f32 updateRateF, f64 step);
void racer_zipper_align(Object *obj, Object_Racer *racer, s32 updateRate);
s32 racer_dominant_surface(Object_Racer *racer, s32 *stoneWheels);
void racer_check_taj_pad(Object_Racer *racer, s32 surface);
void racer_start_zip_pad_boost(Object *obj, Object_Racer *racer);
f32 racer_speed_curve(f32 speed);
void racer_enable_boost_emitter(Object *obj, Object_Racer *racer, s32 baseBit);
void racer_velocity_to_local(Object *obj, Object_Racer *racer, s32 includeRoll);
void racer_collide_keeping_bump(Object *obj, Object_Racer *racer, s32 updateRate);
void update_hovercraft(s32 updateRate, f32 updateRateF, Object *obj, Object_Racer *racer);
void update_camera_hovercraft(f32 updateRate, Object *obj, Object_Racer *racer);
f32 rotate_racer_in_water(Object *obj, Object_Racer *racer, Vec3f *pos, s8 waveType, s32 updateRate, s32 steerX,
                          f32 driftScale);
void update_plane(s32 updateRate, f32 updateRateF, Object *obj, Object_Racer *racer);
void update_camera_plane(f32 updateRate, Object *obj, Object_Racer *racer);
void update_loop_de_loop(s32 updateRate, f32 updateRateF, Object *racerObj, Object_Racer *racer);
void update_camera_loop(f32 updateRateF, Object *obj, Object_Racer *racer);
void update_carpet(s32 updateRate, f32 updateRateF, Object *obj, Object_Racer *racer);
void set_racer_tail_lights(Object_Racer *racer);
void update_car(s32 updateRate, f32 updateRateF, Object *racerObj, Object_Racer *racer);
void racer_approach_object(Object *obj, Object_Racer *racer, f32 divisor);
void apply_vehicle_rotation_offset(Object_Racer *obj, s32 max, s16 yRotation, s16 xRotation, s16 zRotation);
void handle_racer_head_turning(Object *obj, Object_Racer *racer, s32 updateRate);
void slowly_reset_head_angle(Object_Racer *racer);
void racer_spinout_car(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF);
void update_car_velocity_offground(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF);
void racer_update_tilt_vector(Object *obj, Object_Racer *racer);
void handle_car_velocity_control(Object_Racer *racer);
void update_car_wheels(Object *objRacer, Object_Racer *racer, f32 updateRateF);
void racer_attack_handler_car(Object *obj, Object_Racer *racer, s32 updateRate);
void update_onscreen_AI_racer(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF);
void update_car_velocity_ground(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF);
void update_racer_collision(Object *racerObj, Object_Racer *racer, s32 updateRate);
void onscreen_ai_racer_physics(Object *obj, Object_Racer *racer, s32 updateRate);
void handle_racer_items(Object *obj, Object_Racer *racer, s32 updateRate);
void racer_activate_magnet(Object *obj, Object_Racer *racer, s32 updateRate);
void racer_play_sound(Object *obj, s32 soundID);
void play_random_character_voice(Object *obj, s32 soundID, s32 range, s32 flags);
f32 handle_racer_top_speed(Object *obj, Object_Racer *racer);
void racer_update_basis_vectors(Object *obj, Object_Racer *racer);
void drop_bananas(Object *obj, Object_Racer *racer, s32 number);
void handle_base_steering(Object_Racer *racer, s32 updateRate, f32 updateRateF);
void update_player_camera(Object *obj, Object_Racer *racer, f32 updateRateF);
void second_racer_camera_update(Object *obj, Object_Racer *racer, s32 mode, f32 updateRateF);
void set_position_goal_from_path(Object *obj, Object_Racer *racer, f32 *x, f32 *y, f32 *z);
void timetrial_ghost_write(Object *obj, s32 updateRate);
void update_AI_racer(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF);
void racer_update_progress(Object_Racer *racer);

#endif
