#ifndef _JOYPAD_H_
#define _JOYPAD_H_

#include "types.h"
#include "host.h"

// --- Controller buttons and status, as the game reads them (the N64 bit layout) -----
#define MAXCONTROLLERS 4

#define CONT_NO_RESPONSE_ERROR 0x8
#define CONT_OVERRUN_ERROR 0x4
#define CONT_RANGE_ERROR -1
#define CONT_FRAME_ERROR 0x2
#define CONT_COLLISION_ERROR 0x1

#define CONT_ABSOLUTE 0x0001
#define CONT_RELATIVE 0x0002
#define CONT_JOYPORT 0x0004
#define CONT_EEPROM 0x8000
#define CONT_EEP16K 0x4000

#define CONT_A 0x8000
#define CONT_B 0x4000
#define CONT_G 0x2000
#define CONT_START 0x1000
#define CONT_UP 0x0800
#define CONT_DOWN 0x0400
#define CONT_LEFT 0x0200
#define CONT_RIGHT 0x0100
#define CONT_L 0x0020
#define CONT_R 0x0010
#define CONT_E 0x0008
#define CONT_D 0x0004
#define CONT_C 0x0002
#define CONT_F 0x0001

#define A_BUTTON CONT_A
#define B_BUTTON CONT_B
#define L_TRIG CONT_L
#define R_TRIG CONT_R
#define Z_TRIG CONT_G
#define START_BUTTON CONT_START
#define U_JPAD CONT_UP
#define L_JPAD CONT_LEFT
#define R_JPAD CONT_RIGHT
#define D_JPAD CONT_DOWN
#define U_CBUTTONS CONT_E
#define L_CBUTTONS CONT_C
#define R_CBUTTONS CONT_F
#define D_CBUTTONS CONT_D

#define CONTROLLER_MISSING -1
#define CONTROLLER_EXISTS   0

#define JOYSTICK_DEADZONE 8
#define JOYSTICK_MAX_RANGE 70

s32 input_init(void);
s32 input_update(s32 saveDataFlags, s32 updateRate);
void input_assign_players(void);
void charselect_assign_players(s8 *activePlayers);
u8 input_player_id(s32 player);
void input_swap_id(void);
u16 input_held(s32 player);
u32 input_pressed(s32 player);
u16 input_released(s32 player);
s32 input_clamp_stick_x(s32 player);
s32 input_clamp_stick_y(s32 player);
s8 input_clamp_stick_mag(s8 stickMag);
void drm_disable_input(void);

#endif
