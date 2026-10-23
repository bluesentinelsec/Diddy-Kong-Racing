// Host input: SDL3 keyboard and gamepad, mapped onto one N64 controller.

#include "input.h"

#include "gfx.h"
#include "trace.h"
#include "vcontrols.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    { SDL_SCANCODE_X, BTN_A },     // accelerate
    { SDL_SCANCODE_C, BTN_B },     // brake / reverse
    { SDL_SCANCODE_Z, BTN_Z },     // fire weapon
    { SDL_SCANCODE_SPACE, BTN_R }, // hop / powerslide
    { SDL_SCANCODE_Q, BTN_L },     { SDL_SCANCODE_RETURN, BTN_START }, { SDL_SCANCODE_I, BTN_CUP },
    { SDL_SCANCODE_K, BTN_CDOWN }, { SDL_SCANCODE_J, BTN_CLEFT },      { SDL_SCANCODE_L, BTN_CRIGHT },
};

#define NUM_KEY_BINDINGS (int) (sizeof(sKeyBindings) / sizeof(sKeyBindings[0]))

// ---------------------------------------------------------------------------
// Scripted input: DKR_INPUT_SCRIPT=FILE replays controller state by frame, for
// reproducing a report or driving a trace past the title screen. Each line is
//
//     <frame> <buttons> <stickX> <stickY> [p<N>]
//
// and holds from that frame (as counted by gfx_frame_count) until the next
// line for the same controller. Buttons are a comma-separated list of A B Z
// START L R DU DD DL DR CU CD CL CR, or "-" for none; AUTO is not a button but
// a mode: while it is held, human racers drive themselves with the AI's inputs
// (so a scene can finish a race). p<N> addresses controller N (1-based; 1 is
// the default), and a script that names p2..p4 plugs those controllers in.
// Lines must be in frame order; '#' starts a comment.
// ---------------------------------------------------------------------------
#define SCRIPT_MAX_PORTS 4
#define BTN_SCRIPT_AUTO 0x0080 // an unused bit of the N64 button word

typedef struct {
    unsigned int frame;
    unsigned short buttons;
    signed char stickX, stickY;
    unsigned char port;
} ScriptStep;

static ScriptStep *sScript;
static int sScriptCount;
static int sScriptLoaded;
static int sScriptPorts;

static unsigned short script_button(const char *name) {
    static const struct {
        const char *name;
        unsigned short bit;
    } table[] = {
        { "A", BTN_A },    { "B", BTN_B },      { "Z", BTN_Z },      { "START", BTN_START }, { "L", BTN_L },
        { "R", BTN_R },    { "DU", BTN_UP },    { "DD", BTN_DOWN },  { "DL", BTN_LEFT },     { "DR", BTN_RIGHT },
        { "CU", BTN_CUP }, { "CD", BTN_CDOWN }, { "CL", BTN_CLEFT }, { "CR", BTN_CRIGHT },
    };
    size_t i;

    for (i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcmp(name, table[i].name) == 0) {
            return table[i].bit;
        }
    }
    if (strcmp(name, "AUTO") == 0) {
        return BTN_SCRIPT_AUTO;
    }
    fprintf(stderr, "INPUT: unknown button '%s' in DKR_INPUT_SCRIPT\n", name);
    return 0;
}

static void script_load(void) {
    const char *path = getenv("DKR_INPUT_SCRIPT");
    FILE *f;
    char line[256];

    sScriptLoaded = 1;
    if (path == NULL || path[0] == '\0') {
        return;
    }
    f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "INPUT: cannot read %s\n", path);
        exit(1);
    }
    while (fgets(line, sizeof(line), f) != NULL) {
        char buttons[128];
        char portName[16];
        ScriptStep step;
        int x, y;
        int port;
        char *tok;
        char *save;

        portName[0] = '\0';
        if (line[0] == '#' || sscanf(line, "%u %127s %d %d %15s", &step.frame, buttons, &x, &y, portName) < 4) {
            continue;
        }
        port = 1;
        if (portName[0] != '\0' && (sscanf(portName, "p%d", &port) != 1 || port < 1 || port > SCRIPT_MAX_PORTS)) {
            fprintf(stderr, "INPUT: bad controller '%s' in DKR_INPUT_SCRIPT (p1..p%d)\n", portName, SCRIPT_MAX_PORTS);
            exit(1);
        }
        step.port = (unsigned char) (port - 1);
        if (port > sScriptPorts) {
            sScriptPorts = port;
        }
        step.buttons = 0;
        step.stickX = (signed char) x;
        step.stickY = (signed char) y;
        for (tok = strtok_r(buttons, ",", &save); tok != NULL; tok = strtok_r(NULL, ",", &save)) {
            if (strcmp(tok, "-") != 0) {
                step.buttons |= script_button(tok);
            }
        }
        sScript = realloc(sScript, (size_t) (sScriptCount + 1) * sizeof(*sScript));
        sScript[sScriptCount++] = step;
    }
    fclose(f);
    if (sScriptCount > 0 && sScriptPorts < 1) {
        sScriptPorts = 1;
    }
    printf("INPUT: replaying %d steps from %s (%d controller%s)\n", sScriptCount, path, sScriptPorts,
           sScriptPorts == 1 ? "" : "s");
}

// The script step in force for `port` at the current frame, or NULL.
static const ScriptStep *script_current(int port) {
    unsigned int frame;
    int i;

    if (!sScriptLoaded) {
        script_load();
    }
    frame = gfx_frame_count();
    for (i = sScriptCount - 1; i >= 0; i--) {
        if (sScript[i].port == port && sScript[i].frame <= frame) {
            return &sScript[i];
        }
    }
    return NULL;
}

// Returns 1 and fills the outputs if a script is active.
static int script_read(int port, unsigned short *button, signed char *stickX, signed char *stickY) {
    const ScriptStep *step;

    if (!sScriptLoaded) {
        script_load();
    }
    if (sScriptCount == 0) {
        return 0;
    }
    step = script_current(port);
    if (step != NULL) {
        *button = step->buttons & ~BTN_SCRIPT_AUTO;
        *stickX = step->stickX;
        *stickY = step->stickY;
    }
    return 1;
}

int input_script_ports(void) {
    if (!sScriptLoaded) {
        script_load();
    }
    return sScriptPorts;
}

int input_script_autopilot(void) {
    const ScriptStep *step;

    if (!sScriptLoaded) {
        script_load();
    }
    if (sScriptCount == 0) {
        return 0;
    }
    step = script_current(0);
    return step != NULL && (step->buttons & BTN_SCRIPT_AUTO) != 0;
}

// src/racer.c asks through this name.
int pc_trace_autopilot(void) {
    return input_script_autopilot();
}

void input_host_read_port(int port, unsigned short *button, signed char *stickX, signed char *stickY) {
    *button = 0;
    *stickX = 0;
    *stickY = 0;
    script_read(port, button, stickX, stickY);
}

// Xbox-style layout. The N64 pad's yellow C buttons were used as a second
// digital stick (camera, weapons menus), so the right thumbstick stands in for
// them; the triggers take accelerate/brake the way every modern racer does, while
// the face buttons keep the N64 positions for anyone who thinks in those terms.
//
//   Left stick      -> analog stick          Right stick     -> C-up/down/left/right
//   RT / A          -> A (accelerate)        LT / B          -> B (brake, reverse)
//   X               -> Z (fire weapon)       Y               -> C-up
//   RB              -> R (hop, powerslide)   LB              -> L
//   D-pad           -> D-pad                 Menu (Start)    -> Start
typedef struct {
    SDL_GamepadButton button;
    unsigned short n64;
} PadBinding;

static const PadBinding sPadBindings[] = {
    { SDL_GAMEPAD_BUTTON_SOUTH, BTN_A },          { SDL_GAMEPAD_BUTTON_EAST, BTN_B },
    { SDL_GAMEPAD_BUTTON_WEST, BTN_Z },           { SDL_GAMEPAD_BUTTON_NORTH, BTN_CUP },
    { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, BTN_R }, { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, BTN_L },
    { SDL_GAMEPAD_BUTTON_START, BTN_START },      { SDL_GAMEPAD_BUTTON_DPAD_UP, BTN_UP },
    { SDL_GAMEPAD_BUTTON_DPAD_DOWN, BTN_DOWN },   { SDL_GAMEPAD_BUTTON_DPAD_LEFT, BTN_LEFT },
    { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, BTN_RIGHT },
};

#define NUM_PAD_BINDINGS (int) (sizeof(sPadBindings) / sizeof(sPadBindings[0]))
#define TRIGGER_THRESHOLD 8000  // of 32767
#define C_STICK_THRESHOLD 16000 // right stick deflection that counts as a C press

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
    if (script_read(0, button, stickX, stickY)) {
        return; // DKR_INPUT_SCRIPT drives the controller
    }
    if (pc_trace_active()) {
        return; // a trace replays with the controller untouched
    }

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
        if (SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > TRIGGER_THRESHOLD) {
            buttons |= BTN_A;
        }
        if (SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > TRIGGER_THRESHOLD) {
            buttons |= BTN_B;
        }
        {
            Sint16 rx = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHTX);
            Sint16 ry = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHTY);
            if (rx < -C_STICK_THRESHOLD)
                buttons |= BTN_CLEFT;
            if (rx > C_STICK_THRESHOLD)
                buttons |= BTN_CRIGHT;
            if (ry < -C_STICK_THRESHOLD)
                buttons |= BTN_CUP;
            if (ry > C_STICK_THRESHOLD)
                buttons |= BTN_CDOWN;
        }
        if (x == 0 && y == 0) {
            x = stick_from_axis(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTX), 0);
            y = stick_from_axis(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTY), 1);
        }
    }

    *button = buttons;
    *stickX = (signed char) x;
    *stickY = (signed char) y;

    // On-screen touch controls (Android, or DKR_VCONTROLS=1): OR the virtual
    // buttons in, and take over the stick while the screen is being touched.
    vcontrols_merge(button, stickX, stickY);
}
