#ifndef DKR_BEHAVIOURS_H
#define DKR_BEHAVIOURS_H

#include "level_object_entries.h"
#include "structs.h"
#include "types.h"

#define WORLD_KEY_GRAB_CHECK_RADIUS 50

#define TIME_JIFFIES_PER_SECOND 60
#define TIME_SECONDS(sec) sec *TIME_JIFFIES_PER_SECOND
#define U8_ANGLE_TO_U16(x) (x << 6 << 4)
#define TEX_INDEX_NO_TEXTURE 0xFF

enum BananaBehaviour { BANANA_COLLECTED = -1, BANANA_IDLE, BANANA_DROPPED };

enum WeaponBehaviour {
    WEAPON_DROPPED,
    WEAPON_ARMED,
    WEAPON_TRIGGERED,
    WEAPON_DESTROY,
    WEAPON_STATUS_4,
    WEAPON_STATUS_5
};

enum DoorStatus { DOOR_CLOSING = -1, DOOR_CLOSED, DOOR_OPENING };

enum BossRaceWarps {
    WARP_STANDARD = -1,
    WARP_BOSS_FIRST,
    WARP_BOSS_REMATCH,
};

enum TajBehaviours {
    TAJ_MODE_ROAM,
    TAJ_MODE_APPROACH_PLAYER,
    TAJ_MODE_TURN_TOWARDS_PLAYER,
    TAJ_MODE_GREET_PLAYER,
    TAJ_MODE_DIALOGUE,
    TAJ_MODE_TRANSFORM_BEGIN,
    TAJ_MODE_TRANSFORM_END,
    TAJ_MODE_END_DIALOGUE,
    TAJ_MODE_END_DIALOGUE_CHALLENGE,
    TAJ_MODE_TELEPORT_TO_PLAYER_BEGIN = 10,
    TAJ_MODE_TELEPORT_TO_PLAYER_END,
    TAJ_MODE_SET_CHALLENGE = 15,
    TAJ_MODE_TELEPORT_AWAY_BEGIN = 20,
    TAJ_MODE_TELEPORT_AWAY_END,
    TAJ_MODE_RACE = 30,
    TAJ_MODE_UNK1F
};

enum TTBehaviours {
    TT_MODE_ROAM,
    TT_MODE_APPROACH_PLAYER,
    TT_MODE_TURN_TOWARDS_PLAYER,
    TT_MODE_DIALOGUE,
    TT_MODE_DIALOGUE_END
};

enum SilvereCoinBehaviours {
    SILVER_COIN_ACTIVE,
    SILVER_COIN_COLLECTED,
    SILVER_COIN_COLLECTED_PLAYER_2,
    SILVER_COIN_INACTIVE
};

enum EggPickupStatus { EGG_SPAWNED, EGG_COLLECTED, EGG_MOVING, EGG_IN_BASE, EGG_HATCHED };

enum FrogActions { FROG_IDLE, FROG_HOP, FROG_SQUISH, FROG_FLAT, FROG_UNSQUISH };

typedef struct VertexPosition {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
} VertexPosition;

/**
 * One entry per behaviour id: what an object of that kind does at each point in its life.
 * Every hook is optional; a NULL hook is a no-op. `flags` are the OBJECT_BEHAVIOUR_* bits
 * set on spawn, `dataSize` is the size of the behaviour's own data block (0 for none) and
 * `dataAlign16` asks for that block to start on a 16-byte boundary.
 */
typedef struct ObjectBehaviour {
    const char *name;
    void (*init)(Object *obj, LevelObjectEntry *entry, s32 param);
    void (*loop)(Object *obj, s32 updateRate);
    void (*destroy)(Object *obj, s32 keepAnimTarget);
    void (*render)(Object *obj);
    void (*fade)(Object *obj, s32 alpha);
    s32 flags;
    s32 dataSize;
    s32 dataAlign16;
} ObjectBehaviour;

extern const ObjectBehaviour gObjectBehaviours[];

// The registry row for a behaviour id; ids outside the table get an empty row.
const ObjectBehaviour *obj_behaviour(s32 behaviorId);

extern f32 gNPCPosY;

// Hooks defined beside the behaviours they belong to.
void obj_destroy_racer(Object *obj, s32 keepAnimTarget);
void obj_fade_racer(Object *obj, s32 alpha);
void obj_destroy_weapon(Object *obj, s32 keepAnimTarget);
void obj_destroy_fireball(Object *obj, s32 keepAnimTarget);
void obj_destroy_snowball(Object *obj, s32 keepAnimTarget);
void obj_destroy_wavegenerator(Object *obj, s32 keepAnimTarget);
void obj_destroy_rgbalight(Object *obj, s32 keepAnimTarget);
void obj_destroy_animation(Object *obj, s32 keepAnimTarget);
void obj_destroy_overridepos(Object *obj, s32 keepAnimTarget);
void obj_destroy_log(Object *obj, s32 keepAnimTarget);
void obj_destroy_lensflare(Object *obj, s32 keepAnimTarget);
void obj_destroy_lensflareswitch(Object *obj, s32 keepAnimTarget);
void obj_destroy_animated_object(Object *obj, s32 keepAnimTarget);
void obj_destroy_tracked_animated_object(Object *obj, s32 keepAnimTarget);
void obj_fade_animated_object(Object *obj, s32 alpha);
void obj_fade_ghost(Object *obj, s32 alpha);
void obj_fade_keep(Object *obj, s32 alpha);
void obj_render_characterflag(Object *obj);
void obj_render_butterfly(Object *obj);
void obj_render_fish(Object *obj);
void obj_render_boost(Object *obj);
void butterfly_forget_target(Object *obj);

void obj_init_scenery(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_scenery(Object *obj, s32 updateRate);
void obj_init_fireball_octoweapon(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_fireball_octoweapon(Object *obj, s32 updateRate);
void obj_init_lasergun(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_lasergun(Object *obj, s32 updateRate);
void obj_init_laserbolt(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_laserbolt(Object *obj, s32 updateRate);
void obj_init_torch_mist(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_torch_mist(Object *obj, s32 updateRate);
void obj_init_effectbox(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_effectbox(Object *obj, s32 updateRate);
void obj_init_trophycab(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_trophycab(Object *obj, s32 updateRate);
void obj_init_collectegg(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_collectegg(Object *obj, s32 updateRate);
void obj_init_eggcreator(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_eggcreator(Object *obj, s32 updateRate);
void try_to_collect_egg(Object *obj, Object_CollectEgg *egg);
void obj_init_lighthouse_rocketsignpost(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_rocketsignpost(Object *obj, s32 updateRate);
void obj_init_airzippers_waterzippers(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_airzippers_waterzippers(Object *obj, s32 updateRate);
void obj_init_groundzipper(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_groundzipper(Object *obj, s32 updateRate);
void obj_init_timetrialghost(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_timetrialghost(Object *obj, s32 updateRate);
void obj_init_characterflag(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_characterflag(Object *obj, s32 updateRate);
void obj_init_stopwatchman(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_stopwatchman(Object *obj, s32 updateRate);
void play_tt_voice_clip(u16 soundID, s32 interrupt);
void obj_init_fish(Object *fishObj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_fish(Object *fishObj, s32 updateRate);
void obj_init_lavaspurt(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_lavaspurt(Object *obj, s32 updateRate);
void obj_init_posarrow(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_posarrow(Object *obj, s32 updateRate);
void obj_init_animator(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_animator(Object *obj, s32 updateRate);
void obj_init_animation(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_loop_animobject(Object *obj, s32 updateRate);
void obj_loop_dooropener(Object *obj, s32 updateRate);
void obj_loop_vehicleanim(Object *obj, s32 updateRate);
void obj_loop_animcamera(Object *obj, s32 updateRate);
void obj_init_animcar(Object *obj, s32 arg1);
void obj_loop_animcar(Object *obj, s32 updateRate);
void obj_init_overridepos(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_overridepos(Object *obj, s32 arg1);
void obj_init_wizpigship(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_wizpigship(Object *wizShipObj, s32 updateRate);
void obj_init_hittester(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_hittester(Object *obj, s32 updateRate);
void obj_init_dynamic_lighting_object(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_unknown96(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_snowball(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_snowball(Object *obj, s32 updateRate);
void obj_init_char_select(s32 arg0, s32 arg1);
void obj_loop_char_select(Object *charSelectObj, s32 updateRate);
void obj_init_infopoint(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_infopoint(Object *obj, s32 updateRate);
void obj_init_smoke(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_smoke(Object *obj, s32 updateRate);
void obj_init_unknown25(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_unknown25(Object *obj, s32 updateRate);
void obj_init_wardensmoke(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_wardensmoke(Object *obj, s32 updateRate);
void obj_init_bombexplosion(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_bombexplosion(Object *obj, s32 updateRate);
void obj_init_teleport(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_teleport(Object *obj, s32 updateRate);
void obj_init_exit(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_exit(Object *obj, s32 updateRate);
void obj_init_cameracontrol(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_cameracontrol(Object *obj, s32 updateRate);
void obj_init_setuppoint(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_setuppoint(Object *obj, s32 updateRate);
void obj_init_dino_whale(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_dino_whale(Object *obj, s32 updateRate);
void obj_init_parkwarden(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void set_taj_voice_line(s16 soundID);
void obj_loop_parkwarden(Object *obj, s32 updateRate);
void play_taj_voice_clip(u16 soundID, s32 interrupt);
void obj_loop_gbparkwarden(Object *obj, s32 updateRate);
f32 get_npc_pos_y(void);
void obj_init_checkpoint(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_loop_checkpoint(Object *obj, s32 updateRate);
void obj_init_modechange(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_modechange(Object *obj, s32 updateRate);
void obj_init_bonus(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_bonus(Object *obj, s32 updateRate);
void obj_init_goldenballoon(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_goldenballoon(Object *obj, s32 updateRate);
void obj_init_door(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_door(Object *doorObj, s32 updateRate);
void obj_init_ttdoor(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_ttdoor(Object *obj, s32 updateRate);
void obj_init_trigger(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_trigger(Object *obj, s32 updateRate);
void obj_init_rangetrigger(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_rangetrigger(Object *obj, s32 updateRate);
void obj_init_bridge_whaleramp(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_bridge_whaleramp(Object *obj, s32 updateRate);
void obj_init_rampswitch(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_rampswitch(Object *obj, s32 updateRate);
void obj_init_seamonster(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_seamonster(Object *obj, s32 updateRate);
void obj_init_fogchanger(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_skycontrol(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_skycontrol(Object *obj, s32 updateRate);
void obj_init_ainode(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_ainode(Object *obj, s32 updateRate);
void obj_init_treasuresucker(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_treasuresucker(Object *obj, s32 updateRate);
void obj_init_flycoin(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_flycoin(Object *obj, s32 updateRate);
void obj_init_bananacreator(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_bananacreator(Object *obj, s32 updateRate);
void obj_init_banana(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_banana(Object *obj, s32 updateRate);
void obj_init_silvercoin_adv2(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_silvercoin(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_silvercoin(Object *obj, s32 updateRate);
void obj_init_worldkey(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_worldkey(Object *worldKeyObj, s32 updateRate);
void obj_init_weaponballoon(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_weaponballoon(Object *weaponBalloonObj, s32 updateRate);
void obj_init_wballoonpop(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_wballoonpop(Object *obj, s32 updateRate);
void obj_init_weapon(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_weapon(Object *obj, s32 updateRate);
void weapon_projectile(Object *obj, s32 updateRate);
void rocket_prevent_overshoot(Object *obj, s32 updateRate, Object_Weapon *rocket);
void homing_rocket_prevent_overshoot(Object *obj, s32 updateRate, Object_Weapon *rocket);
void reset_rocket_sound_timer(void);
void decrease_rocket_sound_timer(void);
void play_rocket_trailing_sound(Object *obj, struct Object_Weapon *weapon, u16 soundID);
void weapon_trap(Object *weaponObj, s32 updateRate);
void obj_spawn_effect(f32 x, f32 y, f32 z, s32 objectID, s32 soundID, f32 scale, s32 arg6);
void obj_init_audio(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_audioline(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_audioreverb(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_texscroll(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_loop_texscroll(Object *obj, s32 updateRate);
void obj_init_rgbalight(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_init_buoy_pirateship(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_loop_buoy_pirateship(Object *obj, s32 updateRate);
void obj_init_log(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_loop_log(Object *obj, s32 updateRate);
void obj_init_weather(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_weather(Object *obj, s32 updateRate);
void obj_init_lensflare(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_lensflareswitch(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_init_wavegenerator(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_init_butterfly(Object *butterflyObj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_butterfly(Object *butterflyObj, s32 updateRate);
void obj_init_midifade(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_midifadepoint(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_midichset(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_bubbler(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_bubbler(Object *obj, s32 updateRate);
void obj_init_boost(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_init_unknown94(Object *obj, LevelObjectEntry *levelEntry, s32 arg2);
void obj_loop_unknown94(Object *obj, s32 updateRate);
void obj_init_frog(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_frog(Object *obj, s32 updateRate);
void obj_loop_pigrocketeer(Object *obj, s32 updateRate);
void obj_init_levelname(Object *obj, LevelObjectEntry *levelEntry, s32 param);
void obj_loop_levelname(Object *obj, s32 updateRate);
void obj_loop_wizghosts(Object *obj, s32 updateRate);

void func_80037D60(s32 arg0, s32 arg1);
void func_80038330(s32 arg0, s32 arg1);
void func_800387C0(s32 arg0, s32 arg1);
void anim_object_spawn_target(Object *obj, LevelObjectEntry_Animation *animEntry);
void anim_objects_spawn_targets(void);
void calc_env_mapping_for_object(ObjectModel *, s16, s16, s16);
s32 ainode_find_nearest(f32 diffX, f32 diffY, f32 diffZ, s32 useElevation);
void object_id_assign(Object *, s32);

s32 anim_count_actors(s32);
s32 anim_actor_is_active(s32);

#endif
