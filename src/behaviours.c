/**
 * The behaviour registry: one row per behaviour id, naming the hooks an object of that
 * kind runs when it is spawned, ticked, drawn, faded and destroyed, the flags it starts
 * with and the size of its private data. Behaviours left out of the table do nothing.
 */
#include "behaviours.h"

#include "objects.h"
#include "racer.h"
#include "tracks.h"
#include "waves.h"

#define ANIMATED(...) .destroy = obj_destroy_animated_object, .dataSize = sizeof(Object_AnimatedObject), __VA_ARGS__

const ObjectBehaviour *obj_behaviour(s32 behaviorId) {
    static const ObjectBehaviour sNoBehaviour = { "none" };
    if (behaviorId < 0 || behaviorId >= BHV_COUNT) {
        return &sNoBehaviour;
    }
    return &gObjectBehaviours[behaviorId];
}

/** The object keeps whatever opacity it already had. */
void obj_fade_keep(UNUSED Object *obj, UNUSED s32 alpha) {
}

const ObjectBehaviour gObjectBehaviours[BHV_COUNT] = {
    [BHV_RACER] = { "racer", .init = obj_init_racer, .destroy = obj_destroy_racer, .fade = obj_fade_racer,
                    .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_WATER_EFFECT |
                             OBJECT_BEHAVIOUR_ANIMATION | OBJECT_BEHAVIOUR_INTERACTIVE,
                    .dataSize = sizeof(Object_Racer) },
    [BHV_SCENERY] = { "scenery", .init = obj_init_scenery, .loop = obj_loop_scenery,
                      .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_FISH] = { "fish", .init = obj_init_fish, .loop = obj_loop_fish, .render = obj_render_fish,
                   .dataSize = sizeof(Object_Fish), .dataAlign16 = TRUE },
    [BHV_ANIMATOR] = { "animator", .init = obj_init_animator, .loop = obj_loop_animator,
                       .dataSize = sizeof(Object_Animator) },
    [BHV_WEAPON] = { "weapon", .init = obj_init_weapon, .loop = obj_loop_weapon, .destroy = obj_destroy_weapon,
                     .flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_WATER_EFFECT | OBJECT_BEHAVIOUR_INTERACTIVE,
                     .dataSize = sizeof(Object_Weapon) },
    [BHV_SMOKE] = { "smoke", .init = obj_init_smoke, .loop = obj_loop_smoke },
    [BHV_EXIT] = { "exit", .init = obj_init_exit, .loop = obj_loop_exit, .flags = OBJECT_BEHAVIOUR_INTERACTIVE,
                   .dataSize = sizeof(Object_Exit) },
    [BHV_AUDIO] = { "audio", .init = obj_init_audio, .dataSize = sizeof(Object_Audio) },
    [BHV_AUDIO_LINE] = { "audio_line", .init = obj_init_audioline, .dataSize = sizeof(Object_AudioLine) },
    [BHV_CAMERA_CONTROL] = { "camera_control", .init = obj_init_cameracontrol, .loop = obj_loop_cameracontrol },
    [BHV_SETUP_POINT] = { "setup_point", .init = obj_init_setuppoint, .loop = obj_loop_setuppoint },
    [BHV_DINO_WHALE] = { "dino_whale", .init = obj_init_dino_whale, .loop = obj_loop_dino_whale,
                         ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                           OBJECT_BEHAVIOUR_ANIMATION | OBJECT_BEHAVIOUR_INTERACTIVE) },
    [BHV_CHECKPOINT] = { "checkpoint", .init = obj_init_checkpoint, .loop = obj_loop_checkpoint,
                         .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_DOOR] = { "door", .init = obj_init_door, .loop = obj_loop_door,
                   .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE,
                   .dataSize = sizeof(Object_Door) },
    [BHV_FOG_CHANGER] = { "fog_changer", .init = obj_init_fogchanger, .loop = obj_loop_fogchanger },
    [BHV_AINODE] = { "ai_node", .init = obj_init_ainode, .loop = obj_loop_ainode, .dataSize = sizeof(Object_AiNode) },
    [BHV_WEAPON_BALLOON] = { "weapon_balloon", .init = obj_init_weaponballoon, .loop = obj_loop_weaponballoon,
                             .flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_INTERACTIVE,
                             .dataSize = sizeof(Object_WeaponBalloon) },
    [BHV_WEAPON_2] = { "weapon_2", .init = obj_init_weapon, .loop = obj_loop_weapon, .destroy = obj_destroy_weapon,
                       .flags = OBJECT_BEHAVIOUR_INTERACTIVE, .dataSize = sizeof(Object_Weapon) },
    [BHV_AUDIO_LINE_2] = { "audio_line_2", .init = obj_init_audioline, .dataSize = sizeof(Object_AudioLine) },
    [BHV_UNK_18] = { "unk_18", .flags = OBJECT_BEHAVIOUR_WATER_EFFECT },
    [BHV_UNK_19] = { "unk_19", .init = obj_init_unknown25, .loop = obj_loop_unknown25 },
    [BHV_BOMB_EXPLOSION] = { "bomb_explosion", .init = obj_init_bombexplosion, .loop = obj_loop_bombexplosion },
    [BHV_BALLOON_POP] = { "balloon_pop", .init = obj_init_wballoonpop, .loop = obj_loop_wballoonpop },
    [BHV_SKY_CONTROL] = { "sky_control", .init = obj_init_skycontrol, .loop = obj_loop_skycontrol,
                          .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_AUDIO_REVERB] = { "audio_reverb", .init = obj_init_audioreverb, .dataSize = sizeof(Object_AudioReverb) },
    [BHV_TORCH_MIST] = { "torch_mist", .init = obj_init_torch_mist, .loop = obj_loop_torch_mist },
    [BHV_TEXTURE_SCROLL] = { "texture_scroll", .init = obj_init_texscroll, .loop = obj_loop_texscroll,
                             .dataSize = sizeof(Object_TexScroll) },
    [BHV_MODECHANGE] = { "mode_change", .init = obj_init_modechange, .loop = obj_loop_modechange,
                         .flags = OBJECT_BEHAVIOUR_INTERACTIVE, .dataSize = sizeof(Object_Trigger) },
    [BHV_STOPWATCH_MAN] = { "stopwatch_man", .init = obj_init_stopwatchman, .loop = obj_loop_stopwatchman,
                            .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION |
                                     OBJECT_BEHAVIOUR_INTERACTIVE,
                            .dataSize = sizeof(Object_NPC) },
    [BHV_BANANA] = { "banana", .init = obj_init_banana, .loop = obj_loop_banana,
                     .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_SHADOW,
                     .dataSize = sizeof(Object_Banana) },
    [BHV_LIGHT_RGBA] = { "light_rgba", .init = obj_init_rgbalight, .destroy = obj_destroy_rgbalight },
    [BHV_UNK_30] = { "unk_30", .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_ANIMATION },
    [BHV_BUOY_PIRATE_SHIP] = { "buoy_pirate_ship", .init = obj_init_buoy_pirateship, .loop = obj_loop_buoy_pirateship,
                               .destroy = obj_destroy_log, .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_WEATHER] = { "weather", .init = obj_init_weather, .loop = obj_loop_weather },
    [BHV_BRIDGE_WHALE_RAMP] = { "bridge_whale_ramp", .init = obj_init_bridge_whaleramp,
                                .loop = obj_loop_bridge_whaleramp,
                                .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_ANIMATION |
                                         OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE,
                                .dataSize = sizeof(Object_Bridge_WhaleRamp) },
    [BHV_RAMP_SWITCH] = { "ramp_switch", .init = obj_init_rampswitch, .loop = obj_loop_rampswitch,
                          .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_SHADOW },
    [BHV_SEA_MONSTER] = { "sea_monster", .init = obj_init_seamonster, .loop = obj_loop_seamonster,
                          .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_ANIMATION, .dataSize = 0x18 },
    [BHV_BONUS] = { "bonus", .init = obj_init_bonus, .loop = obj_loop_bonus, .flags = OBJECT_BEHAVIOUR_INTERACTIVE,
                    .dataSize = sizeof(Object_Trigger) },
    [BHV_LENS_FLARE] = { "lens_flare", .init = obj_init_lensflare, .destroy = obj_destroy_lensflare },
    [BHV_LENS_FLARE_SWITCH] = { "lens_flare_switch", .init = obj_init_lensflareswitch,
                                .destroy = obj_destroy_lensflareswitch },
    [BHV_COLLECT_EGG] = { "collect_egg", .init = obj_init_collectegg, .loop = obj_loop_collectegg,
                          .flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_INTERACTIVE,
                          .dataSize = sizeof(Object_CollectEgg) },
    [BHV_EGG_CREATOR] = { "egg_creator", .init = obj_init_eggcreator, .loop = obj_loop_eggcreator },
    [BHV_CHARACTER_FLAG] = { "character_flag", .init = obj_init_characterflag, .loop = obj_loop_characterflag,
                             .render = obj_render_characterflag, .dataSize = sizeof(CharacterFlagModel),
                             .dataAlign16 = TRUE },
    [BHV_ANIMATION] = { "animation", .init = obj_init_animation, .destroy = obj_destroy_animation,
                        .flags = OBJECT_BEHAVIOUR_SHADOW },
    [BHV_ANIMATED_OBJECT] = { "animated_object", .loop = obj_loop_animobject, .fade = obj_fade_animated_object,
                              ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                                OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_CAMERA_ANIMATION] = { "camera_animation", .loop = obj_loop_animcamera, .fade = obj_fade_animated_object,
                               ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADOW) },
    [BHV_INFO_POINT] = { "info_point", .init = obj_init_infopoint, .loop = obj_loop_infopoint,
                         .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_CAR_ANIMATION] = { "car_animation", .loop = obj_loop_animcar, .fade = obj_fade_animated_object,
                            .destroy = obj_destroy_animated_object, .dataSize = sizeof(Object_AnimatedObject) },
    [BHV_CHARACTER_SELECT] = { "character_select", .loop = obj_loop_char_select, .fade = obj_fade_animated_object,
                               ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                                 OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_TRIGGER] = { "trigger", .init = obj_init_trigger, .loop = obj_loop_trigger,
                      .flags = OBJECT_BEHAVIOUR_INTERACTIVE, .dataSize = sizeof(Object_Trigger) },
    [BHV_VEHICLE_ANIMATION] = { "vehicle_animation", .loop = obj_loop_vehicleanim, .fade = obj_fade_animated_object,
                                ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                                  OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_ZIPPER_WATER] = { "zipper_water", .init = obj_init_airzippers_waterzippers,
                           .loop = obj_loop_airzippers_waterzippers, .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_TIMETRIAL_GHOST] = { "timetrial_ghost", .init = obj_init_timetrialghost, .loop = obj_loop_timetrialghost,
                              .fade = obj_fade_ghost },
    [BHV_WAVE_GENERATOR] = { "wave_generator", .init = obj_init_wavegenerator, .destroy = obj_destroy_wavegenerator },
    [BHV_WAVE_POWER] = { "wave_power", .loop = obj_loop_wavepower },
    [BHV_BUTTERFLY] = { "butterfly", .init = obj_init_butterfly, .loop = obj_loop_butterfly,
                        .render = obj_render_butterfly, .flags = OBJECT_BEHAVIOUR_SHADOW,
                        .dataSize = sizeof(Object_Butterfly), .dataAlign16 = TRUE },
    [BHV_PARK_WARDEN] = { "park_warden", .init = obj_init_parkwarden, .loop = obj_loop_parkwarden,
                          .fade = obj_fade_keep,
                          .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION |
                                   OBJECT_BEHAVIOUR_INTERACTIVE,
                          .dataSize = sizeof(Object_NPC) },
    [BHV_UNK_3F] = { "unk_3f", .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_ANIMATION },
    [BHV_WORLD_KEY] = { "world_key", .init = obj_init_worldkey, .loop = obj_loop_worldkey,
                        .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_SHADOW },
    [BHV_BANANA_SPAWNER] = { "banana_spawner", .init = obj_init_bananacreator, .loop = obj_loop_bananacreator },
    [BHV_TREASURE_SUCKER] = { "treasure_sucker", .init = obj_init_treasuresucker, .loop = obj_loop_treasuresucker },
    [BHV_LOG] = { "log", .init = obj_init_log, .loop = obj_loop_log, .destroy = obj_destroy_log,
                  .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE },
    [BHV_LAVA_SPURT] = { "lava_spurt", .init = obj_init_lavaspurt, .loop = obj_loop_lavaspurt,
                         .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_POS_ARROW] = { "pos_arrow", .init = obj_init_posarrow, .loop = obj_loop_posarrow },
    [BHV_HIT_TESTER] = { "hit_tester", .init = obj_init_hittester, .loop = obj_loop_hittester,
                         .fade = obj_fade_animated_object,
                         ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                           OBJECT_BEHAVIOUR_ANIMATION | OBJECT_BEHAVIOUR_INTERACTIVE |
                                           OBJECT_BEHAVIOUR_COLLIDABLE) },
    [BHV_MIDI_FADE] = { "midi_fade", .init = obj_init_midifade, .dataSize = sizeof(Object_MidiFade) },
    [BHV_HIT_TESTER_2] = { "hit_tester_2", .init = obj_init_dynamic_lighting_object, .loop = obj_loop_hittester,
                           .fade = obj_fade_animated_object,
                           ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                             OBJECT_BEHAVIOUR_ANIMATION | OBJECT_BEHAVIOUR_INTERACTIVE |
                                             OBJECT_BEHAVIOUR_COLLIDABLE) },
    [BHV_EFFECT_BOX] = { "effect_box", .init = obj_init_effectbox, .loop = obj_loop_effectbox },
    [BHV_TROPHY_CABINET] = { "trophy_cabinet", .init = obj_init_trophycab, .loop = obj_loop_trophycab,
                             .flags =
                                 OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE,
                             .dataSize = sizeof(JingleState) },
    [BHV_BUBBLER] = { "bubbler", .init = obj_init_bubbler, .loop = obj_loop_bubbler },
    [BHV_FLY_COIN] = { "fly_coin", .init = obj_init_flycoin, .loop = obj_loop_flycoin },
    [BHV_GOLDEN_BALLOON] = { "golden_balloon", .init = obj_init_goldenballoon, .loop = obj_loop_goldenballoon,
                             .fade = obj_fade_keep, .flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_INTERACTIVE,
                             .dataSize = sizeof(Object_NPC) },
    [BHV_LASER_BOLT] = { "laser_bolt", .init = obj_init_laserbolt, .loop = obj_loop_laserbolt,
                         .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_LASER_GUN] = { "laser_gun", .init = obj_init_lasergun, .loop = obj_loop_lasergun,
                        .flags = OBJECT_BEHAVIOUR_INTERACTIVE, .dataSize = sizeof(Object_LaserGun) },
    [BHV_PARK_WARDEN_2] = { "park_warden_2", .loop = obj_loop_gbparkwarden, .fade = obj_fade_keep,
                            ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                              OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_ANIMATED_OBJECT_2] = { "animated_object_2", .loop = obj_loop_animobject, .fade = obj_fade_animated_object,
                                ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_ZIPPER_GROUND] = { "zipper_ground", .init = obj_init_groundzipper, .loop = obj_loop_groundzipper,
                            .flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_OVERRIDE_POS] = { "override_pos", .init = obj_init_overridepos, .destroy = obj_destroy_overridepos,
                           .dataSize = sizeof(Object_OverridePos) },
    [BHV_WIZPIG_SHIP] = { "wizpig_ship", .init = obj_init_wizpigship, .loop = obj_loop_wizpigship,
                          ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                            OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_ANIMATED_OBJECT_3] = { "animated_object_3", .loop = obj_loop_animobject,
                                .destroy = obj_destroy_tracked_animated_object,
                                .dataSize = sizeof(Object_AnimatedObject) },
    [BHV_ANIMATED_OBJECT_4] = { "animated_object_4", .loop = obj_loop_animobject,
                                ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                                  OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_TAJ_TELEPOINT] = { "taj_telepoint" },
    [BHV_SILVER_COIN] = { "silver_coin", .init = obj_init_silvercoin, .loop = obj_loop_silvercoin,
                          .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_SHADOW },
    [BHV_BOOST] = { "boost", .init = obj_init_boost, .render = obj_render_boost },
    [BHV_WARDEN_SMOKE] = { "warden_smoke", .init = obj_init_wardensmoke, .loop = obj_loop_wardensmoke },
    [BHV_UNK_5B] = { "unk_5b", .flags = OBJECT_BEHAVIOUR_SHADED },
    [BHV_UNK_5C] = { "unk_5c" },
    [BHV_ZIPPER_AIR] = { "zipper_air", .init = obj_init_airzippers_waterzippers,
                         .loop = obj_loop_airzippers_waterzippers, .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_UNK_5E] = { "unk_5e", .init = obj_init_unknown94, .loop = obj_loop_unknown94, .dataSize = 0x60 },
    [BHV_DYNAMIC_LIGHT_OBJECT_2] = { "dynamic_light_object_2", .init = obj_init_dynamic_lighting_object,
                                     .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_INTERACTIVE |
                                              OBJECT_BEHAVIOUR_COLLIDABLE },
    [BHV_SNOWBALL] = { "snowball", .init = obj_init_unknown96, .loop = obj_loop_snowball,
                       .destroy = obj_destroy_snowball,
                       .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION |
                                OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE,
                       .dataSize = sizeof(Object_AnimatedObject) },
    [BHV_SNOWBALL_2] = { "snowball_2", .init = obj_init_snowball, .loop = obj_loop_snowball,
                         .destroy = obj_destroy_snowball,
                         .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION |
                                  OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE,
                         .dataSize = sizeof(Object_AnimatedObject) },
    [BHV_TELEPORT] = { "teleport", .init = obj_init_teleport, .loop = obj_loop_teleport,
                       .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_ROCKET_SIGNPOST] = { "rocket_signpost", .init = obj_init_lighthouse_rocketsignpost,
                              .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_INTERACTIVE |
                                       OBJECT_BEHAVIOUR_COLLIDABLE },
    [BHV_ROCKET_SIGNPOST_2] = { "rocket_signpost_2", .init = obj_init_lighthouse_rocketsignpost,
                                .loop = obj_loop_rocketsignpost,
                                .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_INTERACTIVE |
                                         OBJECT_BEHAVIOUR_COLLIDABLE },
    [BHV_SNOWBALL_3] = { "snowball_3", .init = obj_init_unknown96, .loop = obj_loop_snowball,
                         .destroy = obj_destroy_snowball,
                         .flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION | OBJECT_BEHAVIOUR_INTERACTIVE |
                                  OBJECT_BEHAVIOUR_COLLIDABLE,
                         .dataSize = sizeof(Object_AnimatedObject) },
    [BHV_SNOWBALL_4] = { "snowball_4", .init = obj_init_snowball, .loop = obj_loop_snowball,
                         .destroy = obj_destroy_snowball,
                         .flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION | OBJECT_BEHAVIOUR_INTERACTIVE |
                                  OBJECT_BEHAVIOUR_COLLIDABLE,
                         .dataSize = sizeof(Object_AnimatedObject) },
    [BHV_HIT_TESTER_3] = { "hit_tester_3", .init = obj_init_hittester, .loop = obj_loop_hittester,
                           ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION |
                                             OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE) },
    [BHV_HIT_TESTER_4] = { "hit_tester_4", .init = obj_init_dynamic_lighting_object, .loop = obj_loop_hittester,
                           ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION |
                                             OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE) },
    [BHV_RANGE_TRIGGER] = { "range_trigger", .init = obj_init_rangetrigger, .loop = obj_loop_rangetrigger },
    [BHV_UNK_6A] = { "unk_6a" },
    [BHV_BOSS_HAZARD_TRIGGER] = { "boss_hazard_trigger" },
    [BHV_FIREBALL_OCTOWEAPON] = { "fireball_octoweapon", .init = obj_init_fireball_octoweapon,
                                  .loop = obj_loop_fireball_octoweapon, .flags = OBJECT_BEHAVIOUR_INTERACTIVE },
    [BHV_FROG] = { "frog", .init = obj_init_frog, .loop = obj_loop_frog,
                   .flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW | OBJECT_BEHAVIOUR_ANIMATION,
                   .dataSize = sizeof(Object_Frog) },
    [BHV_SILVER_COIN_2] = { "silver_coin_2", .init = obj_init_silvercoin_adv2, .loop = obj_loop_silvercoin,
                            .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_SHADOW },
    [BHV_TT_DOOR] = { "tt_door", .init = obj_init_ttdoor, .loop = obj_loop_ttdoor,
                      .flags = OBJECT_BEHAVIOUR_INTERACTIVE | OBJECT_BEHAVIOUR_COLLIDABLE,
                      .dataSize = sizeof(Object_Door) },
    [BHV_MIDI_FADE_POINT] = { "midi_fade_point", .init = obj_init_midifadepoint,
                              .dataSize = sizeof(Object_MidiFadePoint) },
    [BHV_DOOR_OPENER] = { "door_opener", .loop = obj_loop_dooropener, .destroy = obj_destroy_animated_object,
                          .dataSize = sizeof(Object_AnimatedObject) },
    [BHV_UNK_72] = { "unk_72", .flags = OBJECT_BEHAVIOUR_SHADED },
    [BHV_PIG_ROCKETEER] = { "pig_rocketeer", .loop = obj_loop_pigrocketeer,
                            ANIMATED(.flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_SHADOW |
                                              OBJECT_BEHAVIOUR_ANIMATION) },
    [BHV_FIREBALL_OCTOWEAPON_2] = { "fireball_octoweapon_2", .init = obj_init_fireball_octoweapon,
                                    .loop = obj_loop_fireball_octoweapon, .destroy = obj_destroy_fireball,
                                    .flags = OBJECT_BEHAVIOUR_INTERACTIVE, .dataSize = sizeof(Object_Weapon) },
    [BHV_LEVEL_NAME] = { "level_name", .init = obj_init_levelname, .loop = obj_loop_levelname },
    [BHV_MIDI_CHANNEL_SET] = { "midi_channel_set", .init = obj_init_midichset,
                               .dataSize = sizeof(Object_MidiChannelSet) },
    [BHV_WIZPIG_GHOSTS] = { "wizpig_ghosts", .loop = obj_loop_wizghosts, .destroy = obj_destroy_animated_object,
                            .dataSize = sizeof(Object_AnimatedObject) },
};
