// On-screen touch controls: a 2D GL overlay (left analog stick + action buttons)
// merged into the host controller state. See vcontrols.h.

#include "vcontrols.h"

#include "gfx.h"
#include "gl3_loader.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// The CONT_* button bits (include/PR/os_cont.h), repeated here as in input.c.
#define BTN_A 0x8000     // accelerate
#define BTN_B 0x4000     // brake / reverse
#define BTN_Z 0x2000     // use item (fire weapon)
#define BTN_START 0x1000 // pause
#define BTN_R 0x0010     // hop / powerslide
#define BTN_CUP 0x0008   // camera zoom

#define STICK_MAX 80 // N64 full deflection

#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif

// --- geometry layout (computed from the window size each frame) --------------

typedef struct {
    float x, y, r;
} Disc;

typedef struct {
    Disc base;   // stick base
    float knobR; // stick knob radius
    Disc accel, brake, item, hop, cam;
    Disc pause; // a pill: `r` is its half-height, pauseW its full width
    float pauseW;
} Layout;

static Layout vc_layout(int w, int h) {
    float fw = (float) w, fh = (float) h;
    float s = fw < fh ? fw : fh;
    Layout L;

    L.base.r = 0.16f * s;
    L.base.x = L.base.r + 0.045f * fw;
    L.base.y = fh - L.base.r - 0.055f * fh;
    L.knobR = L.base.r * 0.46f;

    // Four face buttons in a diamond at the bottom-right, the N64's A at the
    // thumb's resting place: A bottom, B left, Z top, R right.
    float b = 0.082f * s; // face button radius
    float cx = fw - 0.045f * fw - 2.35f * b;
    float cy = fh - 0.06f * fh - 2.35f * b;
    L.accel.x = cx;
    L.accel.y = cy + 1.35f * b;
    L.accel.r = b;
    L.brake.x = cx - 1.35f * b;
    L.brake.y = cy;
    L.brake.r = b;
    L.item.x = cx;
    L.item.y = cy - 1.35f * b;
    L.item.r = b;
    L.hop.x = cx + 1.35f * b;
    L.hop.y = cy;
    L.hop.r = b;

    // Start as a pill at the bottom-centre; the camera button beside it.
    L.pause.r = 0.034f * s;
    L.pauseW = L.pause.r * 5.2f;
    L.pause.x = fw * 0.5f;
    L.pause.y = fh - L.pause.r - 0.03f * fh;
    L.cam.r = L.pause.r;
    L.cam.x = L.pause.x + L.pauseW * 0.5f + L.cam.r * 1.8f;
    L.cam.y = L.pause.y;
    return L;
}

// --- touch/pointer state -----------------------------------------------------

enum { ROLE_NONE = 0, ROLE_STICK, ROLE_ACCEL, ROLE_BRAKE, ROLE_ITEM, ROLE_HOP, ROLE_CAM, ROLE_PAUSE };

typedef struct {
    SDL_FingerID id;
    int role;
    int used;
} Pointer;

#define MAX_POINTERS 8
#define MOUSE_ID ((SDL_FingerID) - 12345)

static Pointer sPtr[MAX_POINTERS];
static float sStickNx, sStickNy; // knob offset in screen space, -1..1 (y down)
static int sStickActive;
static unsigned short sButtons; // currently-held action buttons

// Cruise control. While the player is driving, a double tap on the accelerator
// latches it down so the thumb is free for the brake and item buttons; a single
// tap, or the brake, releases it. It can only engage while driving
// (pc_player_is_driving, src/thread3_main.c) and drops the moment driving
// stops, so a menu never sees a held A.
#define CRUISE_DOUBLE_TAP_MS 300
#define CRUISE_PULSE_MS 250
static int sCruise;                    // latched
static Uint64 sAccelDownMs;            // when the accelerator was last pressed (for the double tap)
static Uint64 sCruisePulseMs;          // when the latch last changed (for the pulse)
extern int pc_player_is_driving(void); // src/thread3_main.c (s32 there)

static void cruise_set(int on) {
    if (sCruise != on) {
        sCruise = on;
        sCruisePulseMs = SDL_GetTicks();
    }
}

static unsigned short role_bit(int role) {
    switch (role) {
        case ROLE_ACCEL:
            return BTN_A;
        case ROLE_BRAKE:
            return BTN_B;
        case ROLE_ITEM:
            return BTN_Z;
        case ROLE_HOP:
            return BTN_R;
        case ROLE_CAM:
            return BTN_CUP;
        case ROLE_PAUSE:
            return BTN_START;
        default:
            return 0;
    }
}

static int inside(const Disc *d, float px, float py) {
    float dx = px - d->x, dy = py - d->y;
    return dx * dx + dy * dy <= d->r * d->r;
}

static int inside_pill(const Disc *d, float w, float px, float py) {
    float half = w * 0.5f - d->r;
    float dx = px - d->x;
    if (dx > half) {
        dx -= half;
    } else if (dx < -half) {
        dx += half;
    } else {
        dx = 0.0f;
    }
    return dx * dx + (py - d->y) * (py - d->y) <= d->r * d->r;
}

static int hit_role(const Layout *L, float px, float py) {
    if (inside(&L->base, px, py))
        return ROLE_STICK;
    if (inside(&L->accel, px, py))
        return ROLE_ACCEL;
    if (inside(&L->brake, px, py))
        return ROLE_BRAKE;
    if (inside(&L->item, px, py))
        return ROLE_ITEM;
    if (inside(&L->hop, px, py))
        return ROLE_HOP;
    if (inside(&L->cam, px, py))
        return ROLE_CAM;
    if (inside_pill(&L->pause, L->pauseW, px, py))
        return ROLE_PAUSE;
    return ROLE_NONE;
}

static void stick_update(const Layout *L, float px, float py) {
    float dx = px - L->base.x, dy = py - L->base.y;
    float mag = sqrtf(dx * dx + dy * dy);
    float maxr = L->base.r;
    if (mag > maxr && mag > 0.0f) {
        dx *= maxr / mag;
        dy *= maxr / mag;
    }
    sStickNx = dx / maxr;
    sStickNy = dy / maxr;
}

static void recompute_buttons(void) {
    int i;
    sButtons = 0;
    sStickActive = 0;
    for (i = 0; i < MAX_POINTERS; i++) {
        if (!sPtr[i].used) {
            continue;
        }
        if (sPtr[i].role == ROLE_STICK) {
            sStickActive = 1;
        } else {
            sButtons |= role_bit(sPtr[i].role);
        }
    }
    if (!sStickActive) {
        sStickNx = sStickNy = 0.0f;
    }
    if (sCruise) {
        sButtons |= BTN_A;
    }
}

static Pointer *ptr_find(SDL_FingerID id) {
    int i;
    for (i = 0; i < MAX_POINTERS; i++) {
        if (sPtr[i].used && sPtr[i].id == id) {
            return &sPtr[i];
        }
    }
    return NULL;
}

static void ptr_down(SDL_FingerID id, float px, float py, int w, int h) {
    Layout L = vc_layout(w, h);
    int role = hit_role(&L, px, py);
    int i;
    if (role == ROLE_NONE || ptr_find(id) != NULL) {
        return;
    }
    for (i = 0; i < MAX_POINTERS; i++) {
        if (!sPtr[i].used) {
            sPtr[i].used = 1;
            sPtr[i].id = id;
            sPtr[i].role = role;
            if (role == ROLE_STICK) {
                stick_update(&L, px, py);
            } else if (role == ROLE_ACCEL) {
                Uint64 now = SDL_GetTicks();

                if (sCruise) {
                    cruise_set(0); // a tap releases; this press still holds A until it lifts
                } else if (pc_player_is_driving() && sAccelDownMs != 0 && now - sAccelDownMs < CRUISE_DOUBLE_TAP_MS) {
                    cruise_set(1);
                    sAccelDownMs = 0;
                } else {
                    sAccelDownMs = now;
                }
            } else if (role == ROLE_BRAKE) {
                cruise_set(0);
            }
            recompute_buttons();
            return;
        }
    }
}

static void ptr_move(SDL_FingerID id, float px, float py, int w, int h) {
    Pointer *p = ptr_find(id);
    Layout L;
    if (p == NULL || p->role != ROLE_STICK) {
        return;
    }
    L = vc_layout(w, h);
    stick_update(&L, px, py);
}

static void ptr_up(SDL_FingerID id) {
    Pointer *p = ptr_find(id);
    if (p == NULL) {
        return;
    }
    p->used = 0;
    p->role = ROLE_NONE;
    recompute_buttons();
}

// Phones and tablets only: Android (Kindle Fire included) and iOS natively, and
// the browser build when touch is the primary pointer. A desktop browser, or a
// touchscreen laptop, reports a fine (mouse) pointer and gets no overlay.
static int host_is_mobile(void) {
#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    return 1;
#elif defined(__EMSCRIPTEN__)
    return EM_ASM_INT({ return (window.matchMedia && window.matchMedia('(pointer: coarse)').matches) ? 1 : 0; });
#else
    return 0;
#endif
}

int vcontrols_active(void) {
    static int cached = -1;
    if (cached < 0) {
        // DKR_VCONTROLS=1 forces the overlay on (desktop testing, mouse as a
        // finger); DKR_VCONTROLS=0 forces it off.
        const char *e = getenv("DKR_VCONTROLS");
        if (e != NULL && (e[0] == '0' || e[0] == '1')) {
            cached = e[0] == '1';
        } else {
            cached = host_is_mobile();
        }
    }
    return cached;
}

void vcontrols_event(const SDL_Event *e, int winW, int winH) {
    if (!vcontrols_active() || winW <= 0 || winH <= 0) {
        return;
    }
    switch (e->type) {
        case SDL_EVENT_FINGER_DOWN:
            ptr_down(e->tfinger.fingerID, e->tfinger.x * winW, e->tfinger.y * winH, winW, winH);
            break;
        case SDL_EVENT_FINGER_MOTION:
            ptr_move(e->tfinger.fingerID, e->tfinger.x * winW, e->tfinger.y * winH, winW, winH);
            break;
        case SDL_EVENT_FINGER_UP:
            ptr_up(e->tfinger.fingerID);
            break;
        // Mouse stands in for a single finger when DKR_VCONTROLS=1 on the desktop.
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (e->button.button == SDL_BUTTON_LEFT) {
                ptr_down(MOUSE_ID, e->button.x, e->button.y, winW, winH);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (e->motion.state & SDL_BUTTON_LMASK) {
                ptr_move(MOUSE_ID, e->motion.x, e->motion.y, winW, winH);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e->button.button == SDL_BUTTON_LEFT) {
                ptr_up(MOUSE_ID);
            }
            break;
        default:
            break;
    }
}

void vcontrols_merge(unsigned short *button, signed char *stickX, signed char *stickY) {
    if (!vcontrols_active()) {
        return;
    }
    if (sCruise && !pc_player_is_driving()) {
        cruise_set(0); // pause menu, results, cutscene: never hand them a held A
        recompute_buttons();
    }
    *button |= sButtons;
    if (sStickActive) {
        int x = (int) lrintf(sStickNx * (float) STICK_MAX);
        int y = (int) lrintf(-sStickNy * (float) STICK_MAX); // screen y is down; N64 y is up
        if (x > STICK_MAX)
            x = STICK_MAX;
        if (x < -STICK_MAX)
            x = -STICK_MAX;
        if (y > STICK_MAX)
            y = STICK_MAX;
        if (y < -STICK_MAX)
            y = -STICK_MAX;
        if (abs(x) < 6)
            x = 0; // small centre dead zone
        if (abs(y) < 6)
            y = 0;
        *stickX = (signed char) x;
        *stickY = (signed char) y;
    }
}

// --- GL overlay rendering ----------------------------------------------------

typedef struct {
    float x, y;       // NDC
    float r, g, b, a; // colour
} VcVert;

#define VC_MAX_VERTS 8192
static VcVert sVerts[VC_MAX_VERTS];
static int sVertCount;
static float sVpW, sVpH;

static GLuint sProg, sVao, sVbo;
static GLint sAttrPos = -1, sAttrColor = -1;
static int sGlReady = -1;

static GLuint vc_compile(GLenum type, const char *body) {
    const char *version = gfx_is_gles() ? "#version 300 es\nprecision mediump float;\n" : "#version 330 core\n";
    const char *src[2] = { version, body };
    GLint ok = 0;
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 2, src, NULL);
    glCompileShader(sh);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        SDL_Log("VCONTROLS: shader compile failed: %s", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static int vc_gl_init(void) {
    static const char *vs = "in vec2 aPos;\nin vec4 aColor;\nout vec4 vColor;\n"
                            "void main(){ vColor = aColor; gl_Position = vec4(aPos, 0.0, 1.0); }\n";
    static const char *fs = "in vec4 vColor;\nout vec4 oColor;\nvoid main(){ oColor = vColor; }\n";
    GLuint v, f;
    GLint ok = 0;

    v = vc_compile(GL_VERTEX_SHADER, vs);
    f = vc_compile(GL_FRAGMENT_SHADER, fs);
    if (v == 0 || f == 0) {
        return 0;
    }
    sProg = glCreateProgram();
    glAttachShader(sProg, v);
    glAttachShader(sProg, f);
    glLinkProgram(sProg);
    glDeleteShader(v);
    glDeleteShader(f);
    glGetProgramiv(sProg, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(sProg, sizeof(log), NULL, log);
        SDL_Log("VCONTROLS: program link failed: %s", log);
        return 0;
    }
    sAttrPos = glGetAttribLocation(sProg, "aPos");
    sAttrColor = glGetAttribLocation(sProg, "aColor");

    glGenVertexArrays(1, &sVao);
    glBindVertexArray(sVao);
    glGenBuffers(1, &sVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    glEnableVertexAttribArray((GLuint) sAttrPos);
    glVertexAttribPointer((GLuint) sAttrPos, 2, GL_FLOAT, GL_FALSE, sizeof(VcVert), (const void *) 0);
    glEnableVertexAttribArray((GLuint) sAttrColor);
    glVertexAttribPointer((GLuint) sAttrColor, 4, GL_FLOAT, GL_FALSE, sizeof(VcVert),
                          (const void *) (2 * sizeof(float)));
    glBindVertexArray(0);
    return 1;
}

static void emit(float px, float py, const float c[4]) {
    VcVert *v;
    if (sVertCount >= VC_MAX_VERTS) {
        return;
    }
    v = &sVerts[sVertCount++];
    v->x = px / sVpW * 2.0f - 1.0f;
    v->y = 1.0f - py / sVpH * 2.0f;
    v->r = c[0];
    v->g = c[1];
    v->b = c[2];
    v->a = c[3];
}

static void tri(float x0, float y0, float x1, float y1, float x2, float y2, const float c[4]) {
    emit(x0, y0, c);
    emit(x1, y1, c);
    emit(x2, y2, c);
}

static void rect(float x, float y, float w, float h, const float c[4]) {
    tri(x, y, x + w, y, x + w, y + h, c);
    tri(x, y, x + w, y + h, x, y + h, c);
}

static void disc(float cx, float cy, float r, const float c[4]) {
    const int seg = 40;
    float px = cx + r, py = cy;
    int i;
    for (i = 1; i <= seg; i++) {
        float a = (float) i / seg * 6.28318531f;
        float x = cx + cosf(a) * r, y = cy + sinf(a) * r;
        tri(cx, cy, px, py, x, y, c);
        px = x;
        py = y;
    }
}

static void ring(float cx, float cy, float rOut, float rIn, const float c[4]) {
    const int seg = 48;
    float pox = cx + rOut, poy = cy, pix = cx + rIn, piy = cy;
    int i;
    for (i = 1; i <= seg; i++) {
        float a = (float) i / seg * 6.28318531f;
        float ox = cx + cosf(a) * rOut, oy = cy + sinf(a) * rOut;
        float ix = cx + cosf(a) * rIn, iy = cy + sinf(a) * rIn;
        tri(pox, poy, pix, piy, ox, oy, c);
        tri(pix, piy, ix, iy, ox, oy, c);
        pox = ox;
        poy = oy;
        pix = ix;
        piy = iy;
    }
}

// Up- or down-pointing triangle glyph centred at (cx,cy).
// A 5x7 pixel font for the button labels, drawn as little squares so it stays
// crisp at any size and matches the game's look. Only the letters used here.
typedef struct {
    char ch;
    const char *rows[7];
} Glyph;

static const Glyph sFont[] = {
    { 'A', { " ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #" } },
    { 'B', { "#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### " } },
    { 'C', { " ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####" } },
    { 'R', { "#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #" } },
    { 'S', { " ####", "#    ", "#    ", " ### ", "    #", "    #", "#### " } },
    { 'T', { "#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  " } },
    { 'Z', { "#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####" } },
    { '^', { "  #  ", " ### ", "#####", "  #  ", "  #  ", "  #  ", "  #  " } }, // up arrow
};

static const Glyph *glyph_find(char ch) {
    size_t i;
    for (i = 0; i < sizeof(sFont) / sizeof(sFont[0]); i++) {
        if (sFont[i].ch == ch) {
            return &sFont[i];
        }
    }
    return NULL;
}

/** Draw `text` centred on (cx, cy); `cell` is the size of one font pixel. */
static void glyph_text(float cx, float cy, float cell, const char *text, const float c[4]) {
    int n = (int) strlen(text);
    float w = (float) (n * 5 + (n - 1)) * cell;
    float x0 = cx - w * 0.5f;
    float y0 = cy - 3.5f * cell;
    int i, row, col;

    for (i = 0; i < n; i++) {
        const Glyph *g = glyph_find(text[i]);
        float gx = x0 + (float) i * 6.0f * cell;
        if (g == NULL) {
            continue;
        }
        for (row = 0; row < 7; row++) {
            for (col = 0; col < 5; col++) {
                if (g->rows[row][col] == '#') {
                    rect(gx + (float) col * cell, y0 + (float) row * cell, cell, cell, c);
                }
            }
        }
    }
}

static void button_face(float x, float y, float r, const float fill[4], int pressed, int latched) {
    float c[4];
    int i;
    for (i = 0; i < 3; i++) {
        c[i] = fill[i];
    }
    c[3] = pressed ? 0.85f : 0.5f;
    if (latched) {
        c[3] = 0.95f; // cruise: a solid disc, unmistakably different from a press
    }
    disc(x, y, r, c);
    {
        float ringc[4] = { 1.0f, 1.0f, 1.0f, (pressed || latched) ? 0.6f : 0.3f };
        ring(x, y, r, r * 0.9f, ringc);
    }
}

/** A round face button labelled like the N64 pad: "A", "B", "Z", "R", "C^". */
static void draw_button(const Disc *d, const float fill[4], int pressed, int latched, const char *label) {
    const float white[4] = { 1.0f, 1.0f, 1.0f, 0.92f };
    float cell = d->r * (strlen(label) > 1 ? 0.11f : 0.13f);

    button_face(d->x, d->y, d->r, fill, pressed, latched);
    glyph_text(d->x, d->y - (latched ? d->r * 0.12f : 0.0f), cell, label, white);
    if (latched) {
        // Two bars under the letter: the "held down" mark.
        rect(d->x - d->r * 0.3f, d->y + d->r * 0.42f, d->r * 0.6f, d->r * 0.07f, white);
        rect(d->x - d->r * 0.3f, d->y + d->r * 0.58f, d->r * 0.6f, d->r * 0.07f, white);
    }
}

/** The Start button: a pill with the word on it. */
static void draw_pill(const Disc *d, float w, const float fill[4], int pressed, const char *label) {
    const float white[4] = { 1.0f, 1.0f, 1.0f, 0.92f };
    float half = w * 0.5f - d->r;
    float c[4] = { fill[0], fill[1], fill[2], pressed ? 0.85f : 0.5f };
    float ringc[4] = { 1.0f, 1.0f, 1.0f, pressed ? 0.6f : 0.3f };
    float t = d->r * 0.1f;

    disc(d->x - half, d->y, d->r, c);
    disc(d->x + half, d->y, d->r, c);
    rect(d->x - half, d->y - d->r, 2.0f * half, 2.0f * d->r, c);
    ring(d->x - half, d->y, d->r, d->r - t, ringc);
    ring(d->x + half, d->y, d->r, d->r - t, ringc);
    rect(d->x - half, d->y - d->r, 2.0f * half, t, ringc);
    rect(d->x - half, d->y + d->r - t, 2.0f * half, t, ringc);
    glyph_text(d->x, d->y, d->r * 0.16f, label, white);
}

void vcontrols_render(int winW, int winH) {
    Layout L;
    const float baseFill[4] = { 0.06f, 0.06f, 0.09f, 0.33f };
    const float baseRing[4] = { 0.9f, 0.9f, 0.95f, 0.28f };
    const float knobFill[4] = { 0.85f, 0.85f, 0.92f, 0.55f };
    const float accelCol[4] = { 0.20f, 0.78f, 0.32f, 1.0f };
    const float brakeCol[4] = { 0.86f, 0.26f, 0.22f, 1.0f };
    const float itemCol[4] = { 0.26f, 0.52f, 0.92f, 1.0f };
    const float hopCol[4] = { 0.95f, 0.78f, 0.20f, 1.0f };
    const float pauseCol[4] = { 0.65f, 0.65f, 0.70f, 1.0f };
    float kx, ky;

    if (!vcontrols_active() || winW <= 0 || winH <= 0) {
        return;
    }
    if (sGlReady < 0) {
        sGlReady = vc_gl_init();
    }
    if (sGlReady == 0) {
        return;
    }

    sVpW = (float) winW;
    sVpH = (float) winH;
    sVertCount = 0;
    L = vc_layout(winW, winH);

    disc(L.base.x, L.base.y, L.base.r, baseFill);
    ring(L.base.x, L.base.y, L.base.r, L.base.r * 0.92f, baseRing);
    kx = L.base.x + sStickNx * (L.base.r - L.knobR);
    ky = L.base.y + sStickNy * (L.base.r - L.knobR);
    disc(kx, ky, L.knobR, knobFill);

    draw_button(&L.accel, accelCol, (sButtons & BTN_A) != 0, sCruise, "A");
    draw_button(&L.brake, brakeCol, (sButtons & BTN_B) != 0, 0, "B");
    draw_button(&L.item, itemCol, (sButtons & BTN_Z) != 0, 0, "Z");
    draw_button(&L.hop, hopCol, (sButtons & BTN_R) != 0, 0, "R");
    draw_button(&L.cam, pauseCol, (sButtons & BTN_CUP) != 0, 0, "C^");
    draw_pill(&L.pause, L.pauseW, pauseCol, (sButtons & BTN_START) != 0, "START");
    {
        // A ring that swells out of the accelerator and fades when cruise
        // engages or releases, so the change registers without looking down.
        Uint64 age = SDL_GetTicks() - sCruisePulseMs;

        if (sCruisePulseMs != 0 && age < CRUISE_PULSE_MS) {
            float t = (float) age / (float) CRUISE_PULSE_MS;
            float pulse[4] = { 1.0f, 1.0f, 1.0f, 0.8f * (1.0f - t) };
            float r = L.accel.r * (1.0f + 0.6f * t);

            ring(L.accel.x, L.accel.y, r, r * 0.9f, pulse);
        }
    }

    if (sVertCount == 0) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, gfx_window_fbo());
    glViewport(0, 0, winW, winH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(sProg);
    glBindVertexArray(sVao);
    glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (sVertCount * (int) sizeof(VcVert)), sVerts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sVertCount);

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}
