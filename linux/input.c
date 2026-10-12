// Host input: SDL3 keyboard and gamepad, mapped onto one N64 controller.

#include "input.h"

#include <SDL3/SDL.h>

// The CONT_* button bits from include/PR/os_cont.h, repeated here because this
// file cannot include the N64 headers (see input.h).
#define BTN_A 0x8000
#define BTN_B 0x4000
#define BTN_Z 0x2000
#define BTN_START 0x1000
#define BTN_UP 0x0800
#define BTN_DOWN 0x0400
#define BTN_LEFT 0x0200
#define BTN_RIGHT 0x0100
#define BTN_L 0x0020
#define BTN_R 0x0010
#define BTN_CUP 0x0008
#define BTN_CDOWN 0x0004
#define BTN_CLEFT 0x0002
#define BTN_CRIGHT 0x0001

// Full deflection on a real stick, and its diagonal component: the N64 stick is
// round, so holding two axes at once cannot reach 80 on both.
#define STICK_MAX 80
#define STICK_DIAG 57

typedef struct {
    SDL_Scancode scancode;
    unsigned short button;
} KeyBinding;

static const KeyBinding sKeyBindings[] = {
    { SDL_SCANCODE_X, BTN_A },      // accelerate
    { SDL_SCANCODE_C, BTN_B },      // brake / reverse
    { SDL_SCANCODE_Z, BTN_Z },      // fire weapon
    { SDL_SCANCODE_SPACE, BTN_R },  // hop / powerslide
    { SDL_SCANCODE_Q, BTN_L },
    { SDL_SCANCODE_RETURN, BTN_START },
    { SDL_SCANCODE_I, BTN_CUP },
    { SDL_SCANCODE_K, BTN_CDOWN },
    { SDL_SCANCODE_J, BTN_CLEFT },
    { SDL_SCANCODE_L, BTN_CRIGHT },
};

#define NUM_KEY_BINDINGS (int) (sizeof(sKeyBindings) / sizeof(sKeyBindings[0]))

typedef struct {
    SDL_GamepadButton button;
    unsigned short n64;
} PadBinding;

static const PadBinding sPadBindings[] = {
    { SDL_GAMEPAD_BUTTON_SOUTH, BTN_A },          // accelerate
    { SDL_GAMEPAD_BUTTON_EAST, BTN_B },           // brake / reverse
    { SDL_GAMEPAD_BUTTON_WEST, BTN_Z },           // fire weapon
    { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, BTN_R }, // hop / powerslide
    { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, BTN_L },
    { SDL_GAMEPAD_BUTTON_START, BTN_START },
    { SDL_GAMEPAD_BUTTON_NORTH, BTN_CUP },
    { SDL_GAMEPAD_BUTTON_DPAD_UP, BTN_CUP },
    { SDL_GAMEPAD_BUTTON_DPAD_DOWN, BTN_CDOWN },
    { SDL_GAMEPAD_BUTTON_DPAD_LEFT, BTN_CLEFT },
    { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, BTN_CRIGHT },
};

#define NUM_PAD_BINDINGS (int) (sizeof(sPadBindings) / sizeof(sPadBindings[0]))

static SDL_Gamepad *sPad;
static int sPadInit;
static Uint64 sPadNextScan;

// Opens the first gamepad SDL knows about. Re-scanned once a second so a pad
// plugged in after launch is picked up without an event loop of our own.
static void gamepad_poll_devices(void) {
    SDL_JoystickID *ids;
    int count = 0;

    if (!sPadInit) {
        sPadInit = 1;
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
            return;
        }
    }
    if (sPad != NULL) {
        if (SDL_GamepadConnected(sPad)) {
            return;
        }
        SDL_CloseGamepad(sPad);
        sPad = NULL;
    }
    if (SDL_GetTicks() < sPadNextScan) {
        return;
    }
    sPadNextScan = SDL_GetTicks() + 1000;
    ids = SDL_GetGamepads(&count);
    if (ids != NULL) {
        if (count > 0) {
            sPad = SDL_OpenGamepad(ids[0]);
            if (sPad != NULL) {
                SDL_Log("INPUT: using gamepad '%s'", SDL_GetGamepadName(sPad));
            }
        }
        SDL_free(ids);
    }
}

// Steering: arrow keys and WASD both drive the analog stick.
static int axis_value(const bool *keys, int negA, int negB, int posA, int posB) {
    int value = 0;

    if (keys[negA] || keys[negB]) {
        value -= 1;
    }
    if (keys[posA] || keys[posB]) {
        value += 1;
    }
    return value;
}

// SDL axis (-32768..32767, y down) to an N64 stick coordinate (-80..80, y up),
// with a dead zone so a resting stick reads as centred.
static int stick_from_axis(Sint16 raw, int invert) {
    const int deadZone = 6000;
    int v = raw;

    if (v > -deadZone && v < deadZone) {
        return 0;
    }
    if (v < 0) {
        v = -((-v - deadZone) * STICK_MAX / (32768 - deadZone));
    } else {
        v = (v - deadZone) * STICK_MAX / (32767 - deadZone);
    }
    if (v > STICK_MAX) {
        v = STICK_MAX;
    }
    if (v < -STICK_MAX) {
        v = -STICK_MAX;
    }
    return invert ? -v : v;
}

void input_host_read(unsigned short *button, signed char *stickX, signed char *stickY) {
    const bool *keys;
    unsigned short buttons = 0;
    int x, y;
    int magnitude;
    int i;

    *button = 0;
    *stickX = 0;
    *stickY = 0;

    // The game polls the controller before the window opens, and the keyboard
    // state is only meaningful once the video subsystem is up.
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        return;
    }

    keys = SDL_GetKeyboardState(NULL);
    for (i = 0; i < NUM_KEY_BINDINGS; i++) {
        if (keys[sKeyBindings[i].scancode]) {
            buttons |= sKeyBindings[i].button;
        }
    }
    x = axis_value(keys, SDL_SCANCODE_LEFT, SDL_SCANCODE_A, SDL_SCANCODE_RIGHT, SDL_SCANCODE_D);
    y = axis_value(keys, SDL_SCANCODE_DOWN, SDL_SCANCODE_S, SDL_SCANCODE_UP, SDL_SCANCODE_W);
    magnitude = (x != 0 && y != 0) ? STICK_DIAG : STICK_MAX;
    x *= magnitude;
    y *= magnitude;

    gamepad_poll_devices();
    if (sPad != NULL) {
        for (i = 0; i < NUM_PAD_BINDINGS; i++) {
            if (SDL_GetGamepadButton(sPad, sPadBindings[i].button)) {
                buttons |= sPadBindings[i].n64;
            }
        }
        // The right trigger also accelerates, which is how most people hold a
        // racing game on a modern pad.
        if (SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 8000) {
            buttons |= BTN_A;
        }
        if (x == 0 && y == 0) {
            x = stick_from_axis(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTX), 0);
            y = stick_from_axis(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTY), 1);
        }
    }

    *button = buttons;
    *stickX = (signed char) x;
    *stickY = (signed char) y;
}
