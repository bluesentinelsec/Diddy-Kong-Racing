// Host GL backend: SDL3 window + OpenGL 3.3 core / OpenGL ES 3.0, shader based.
//
// The F3DDKR interpreter (linux/main.c) does all the N64-side work and hands this
// file finished screen-space triangles plus a handful of RDP state toggles (see
// gfx.h). This file owns the window, the GL context, one shader program, one
// streaming vertex buffer and the texture objects. It never includes a platform
// GL header: every entry point is resolved through SDL at runtime (gl3_loader.h),
// so the same source and binary layout work with a desktop core profile or a
// GL ES profile.

#include "gfx.h"
#include "gl3_loader.h"
#include "trace.h"
#include "vcontrols.h"

#include <SDL3/SDL.h>
#define SDL_MAIN_NOIMPL    // only main.c may carry SDL's main() on the platforms where SDL owns it
#include <SDL3/SDL_main.h> // SDL_SetMainReady; SDL_MAIN_HANDLED keeps our plain main()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SDL_PLATFORM_IOS
void pc_ios_configure(void); // ios/app.c
#endif

#include "shader_dkr_frag.h"
#include "shader_dkr_vert.h"
#include "shader_post_frag.h"
#include "shader_post_vert.h"

#define DKR_GL_DEFINE(ret, name, args) dkr_##name##_t dkr_##name = NULL;
DKR_GL_FUNCS(DKR_GL_DEFINE)
#undef DKR_GL_DEFINE

const char *dkr_gl_load(void *(*getProc)(const char *name)) {
#define DKR_GL_RESOLVE(ret, name, args)           \
    dkr_##name = (dkr_##name##_t) getProc(#name); \
    if (dkr_##name == NULL) {                     \
        return #name;                             \
    }
    DKR_GL_FUNCS(DKR_GL_RESOLVE)
#undef DKR_GL_RESOLVE
    return NULL;
}

static SDL_Window *sWindow;
static SDL_GLContext sContext;
static int sIsES; // 1 when the context is OpenGL ES

// The N64 framebuffer size we draw in, and how many host pixels one of ours is.
// gfx_set_scissor() needs both: GL's scissor is in window pixels, measured from
// the bottom-left, while the game speaks 320x240 from the top-left.
static int sFbWidth = 320;
static int sFbBaseWidth = 320; // the N64's; sFbWidth grows past it in widescreen
static int sWidescreen = 1;
static int sFbHeight = 240;
static int sWinWidth = 320;
static int sWinHeight = 240;
// The 4:3 image, centred in the window (letter/pillar-boxed), in window pixels.
static int sVpX, sVpY, sVpW = 320, sVpH = 240;
static int sFullscreen;
#ifdef _WIN32
static int sWindowedX, sWindowedY, sWindowedW, sWindowedH;
static int sWindowedBoundsValid;
#endif

static GLuint sProgram;
static GLuint sVao;
static GLuint sVbo;
static GLint sAttrPos, sAttrUV, sAttrFog, sAttrColor;
static GLint sUniScreen, sUniTex, sUniTexMode, sUniEnvColor, sUniFogEnable, sUniFogColor, sUniAlphaRef;

// ---------------------------------------------------------------------------
// Post-processing. The scene is drawn into an offscreen framebuffer the size of
// the 4:3 image, and presenting it is one full-image quad through post.frag,
// which applies the effects that are switched on. Effects are runtime toggles:
//
//   F2  scanlines (on by default; --no-scanlines starts with it off)
// ---------------------------------------------------------------------------
static GLuint sSceneFbo, sSceneTex, sSceneDepth;
static int sSceneW, sSceneH; // what the offscreen buffers were allocated at
static GLuint sPostProgram, sPostVao, sPostVbo;
static GLint sPostAttrPos, sPostAttrUV, sPostUniScene, sPostUniSceneLines, sPostUniScanlines, sPostUniStrength;
static int sScanlines = 1;
static const float SCANLINE_STRENGTH = 0.3f;

static void set_tex_mode_uniform(void);

// Shadowed uniform state, so a redundant gfx_set_* costs nothing.
static int sTexMode = 1;  // what the shader does with the texel (0/1/2)
static int sTexBound = 0; // whether a texture is bound (gates sTexMode)
static int sFogEnable = -1;
static float sFogColor[4] = { -1, -1, -1, -1 };
static float sEnvColor[4] = { -1, -1, -1, -1 };
static float sAlphaRef = -1.0f;

static unsigned int sFrameCount;
// DKR_DUMP_FRAME: comma-separated frame numbers to write as .ppm files;
// DKR_DUMP_EVERY=N also writes every Nth frame.
#define MAX_DUMP_FRAMES 64
static long sDumpFrames[MAX_DUMP_FRAMES];
static int sDumpFrameCount;
static long sDumpEvery;
// DKR_DUMP_PRESENT=a-b: write every presented frame (logic and interpolated)
// numbered a..b as dkr_present_<n>.ppm, to look at the interpolation.
static long sDumpPresentLo = -1, sDumpPresentHi = -1;
static unsigned int sPresentCount;

// Interleaved vertex as uploaded: pos.xyzw, uv, fog, rgba8 — 32 bytes.
typedef struct {
    float x, y, z, w;
    float u, v;
    float fog;
    unsigned char r, g, b, a;
} GfxGpuVert;

// ---------------------------------------------------------------------------
// Context creation
// ---------------------------------------------------------------------------

typedef struct {
    int profile; // SDL_GL_CONTEXT_PROFILE_CORE or _ES
    int major, minor;
    const char *name;
} GlConfig;

static GLuint sWindowFbo; // the framebuffer that is the window (0 except on iOS)
static GLuint sWindowRbo; // iOS: the renderbuffer SDL presents; must be bound at swap

unsigned int gfx_window_fbo(void) {
    return sWindowFbo;
}

static int try_create_context(const GlConfig *cfg, int width, int height) {
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
#ifndef _WIN32
    if (sFullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN; // borderless, at the desktop resolution
    }
#endif
    SDL_GL_ResetAttributes();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, cfg->profile);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, cfg->major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, cfg->minor);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    sWindow = SDL_CreateWindow("Diddy Kong Racing", width, height, flags);
    if (sWindow == NULL) {
        fprintf(stderr, "GFX: SDL_CreateWindow (%s) failed: %s\n", cfg->name, SDL_GetError());
        return 0;
    }
#ifdef _WIN32
    SDL_GetWindowPosition(sWindow, &sWindowedX, &sWindowedY);
    SDL_GetWindowSize(sWindow, &sWindowedW, &sWindowedH);
    sWindowedBoundsValid = 1;
    if (sFullscreen) {
        SDL_Rect bounds;
        SDL_DisplayID display = SDL_GetDisplayForWindow(sWindow);

        if (display && SDL_GetDisplayBounds(display, &bounds)) {
            // Intel's Windows OpenGL driver promotes an exact-monitor drawable
            // into a direct-fullscreen presentation path. Once an Alt-Tab or
            // system OSD interrupts it, SwapBuffers can block for seconds on
            // every frame. Keep this as a compositor-owned borderless window;
            // the one-pixel overscan is clipped and is not visible.
            SDL_SetWindowBordered(sWindow, false);
            SDL_SetWindowPosition(sWindow, bounds.x - 1, bounds.y - 1);
            SDL_SetWindowSize(sWindow, bounds.w + 2, bounds.h + 2);
        }
    }
#endif
    sContext = SDL_GL_CreateContext(sWindow);
    if (sContext == NULL) {
        fprintf(stderr, "GFX: %s context unavailable: %s\n", cfg->name, SDL_GetError());
        SDL_DestroyWindow(sWindow);
        sWindow = NULL;
        return 0;
    }
    sIsES = (cfg->profile == SDL_GL_CONTEXT_PROFILE_ES);
    // On iOS the screen is not framebuffer 0: SDL presents its OpenGL view
    // through a framebuffer object of its own, and drawing to 0 goes nowhere.
    // Everywhere else the window is 0 and the property is absent.
    sWindowFbo = (GLuint) SDL_GetNumberProperty(SDL_GetWindowProperties(sWindow),
                                                SDL_PROP_WINDOW_UIKIT_OPENGL_FRAMEBUFFER_NUMBER, 0);
    sWindowRbo = (GLuint) SDL_GetNumberProperty(SDL_GetWindowProperties(sWindow),
                                                SDL_PROP_WINDOW_UIKIT_OPENGL_RENDERBUFFER_NUMBER, 0);
    return 1;
}

static int create_context(int width, int height) {
    static const GlConfig core33 = { SDL_GL_CONTEXT_PROFILE_CORE, 3, 3, "OpenGL 3.3 core" };
    static const GlConfig es30 = { SDL_GL_CONTEXT_PROFILE_ES, 3, 0, "OpenGL ES 3.0" };
    const GlConfig *order[2];
    int i;

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__) || defined(SDL_PLATFORM_IOS) || \
    (defined(__linux__) && (defined(__arm__) || defined(__aarch64__)))
    // Mesa's V3D (Raspberry Pi) exposes GL ES 3.1 but only desktop GL 3.1; iOS has GL ES only.
    order[0] = &es30;
    order[1] = &core33;
#else
    order[0] = &core33;
    order[1] = &es30;
#endif
    for (i = 0; i < 2; i++) {
        if (try_create_context(order[i], width, height)) {
            printf("GFX: %s\n", order[i]->name);
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------

static GLuint compile_shader(GLenum type, const char *body, unsigned int bodyLen) {
    const char *prelude =
        sIsES ? "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n"
              : "#version 330 core\n";
    const GLchar *sources[2] = { prelude, body };
    GLint lengths[2] = { (GLint) strlen(prelude), (GLint) bodyLen };
    GLuint shader = glCreateShader(type);
    GLint ok = 0;

    glShaderSource(shader, 2, sources, lengths);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        fprintf(stderr, "GFX: %s shader failed to compile:\n%.*s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                (int) len, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint link_program(const unsigned char *vert, unsigned int vertLen, const unsigned char *frag,
                           unsigned int fragLen) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, (const char *) vert, vertLen);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, (const char *) frag, fragLen);
    GLuint program;
    GLint ok = 0;

    if (vs == 0 || fs == 0) {
        return 0;
    }
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei len = 0;
        glGetProgramInfoLog(program, sizeof(log), &len, log);
        fprintf(stderr, "GFX: shader program failed to link:\n%.*s\n", (int) len, log);
        return 0;
    }
    return program;
}

static int build_program(void) {
    sProgram = link_program(shader_dkr_vert, shader_dkr_vert_len, shader_dkr_frag, shader_dkr_frag_len);
    if (sProgram == 0) {
        return 0;
    }

    sAttrPos = glGetAttribLocation(sProgram, "aPos");
    sAttrUV = glGetAttribLocation(sProgram, "aUV");
    sAttrFog = glGetAttribLocation(sProgram, "aFog");
    sAttrColor = glGetAttribLocation(sProgram, "aColor");
    sUniScreen = glGetUniformLocation(sProgram, "uScreen");
    sUniTex = glGetUniformLocation(sProgram, "uTex");
    sUniTexMode = glGetUniformLocation(sProgram, "uTexMode");
    sUniEnvColor = glGetUniformLocation(sProgram, "uEnvColor");
    sUniFogEnable = glGetUniformLocation(sProgram, "uFogEnable");
    sUniFogColor = glGetUniformLocation(sProgram, "uFogColor");
    sUniAlphaRef = glGetUniformLocation(sProgram, "uAlphaRef");

    sPostProgram = link_program(shader_post_vert, shader_post_vert_len, shader_post_frag, shader_post_frag_len);
    if (sPostProgram == 0) {
        return 0;
    }
    sPostAttrPos = glGetAttribLocation(sPostProgram, "aPos");
    sPostAttrUV = glGetAttribLocation(sPostProgram, "aUV");
    sPostUniScene = glGetUniformLocation(sPostProgram, "uScene");
    sPostUniSceneLines = glGetUniformLocation(sPostProgram, "uSceneLines");
    sPostUniScanlines = glGetUniformLocation(sPostProgram, "uScanlines");
    sPostUniStrength = glGetUniformLocation(sPostProgram, "uScanlineStrength");
    return 1;
}

/**
 * (Re)create the offscreen scene buffers at the current image size. Returns 0,
 * with the buffers released, when the GL implementation refuses: the scene then
 * draws straight to the window and the effects are unavailable.
 */
static int scene_buffers_create(void) {
    if (sSceneFbo != 0 && sSceneW == sVpW && sSceneH == sVpH) {
        return 1;
    }
    if (sSceneFbo != 0) {
        glDeleteFramebuffers(1, &sSceneFbo);
        glDeleteTextures(1, &sSceneTex);
        glDeleteRenderbuffers(1, &sSceneDepth);
        sSceneFbo = sSceneTex = sSceneDepth = 0;
    }
    if (sVpW <= 0 || sVpH <= 0) {
        return 0;
    }

    glGenTextures(1, &sSceneTex);
    glBindTexture(GL_TEXTURE_2D, sSceneTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sVpW, sVpH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // presented 1:1
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    sTexBound = 0; // the scene texture is bound now, not a game texture

    glGenRenderbuffers(1, &sSceneDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, sSceneDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, sVpW, sVpH);

    glGenFramebuffers(1, &sSceneFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sSceneFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sSceneTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sSceneDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "GFX: offscreen framebuffer incomplete; post-processing effects unavailable\n");
        glBindFramebuffer(GL_FRAMEBUFFER, sWindowFbo);
        glDeleteFramebuffers(1, &sSceneFbo);
        glDeleteTextures(1, &sSceneTex);
        glDeleteRenderbuffers(1, &sSceneDepth);
        sSceneFbo = sSceneTex = sSceneDepth = 0;
        return 0;
    }
    sSceneW = sVpW;
    sSceneH = sVpH;
    return 1;
}

/** Point the scene draws at the offscreen buffer (or the window when there is none). */
static void scene_target_bind(void) {
    if (sSceneFbo != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, sSceneFbo);
        glViewport(0, 0, sVpW, sVpH);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, sWindowFbo);
        glViewport(sVpX, sVpY, sVpW, sVpH);
    }
}

/**
 * Put back the scene pipeline state the renderer assumes between draws, after
 * something else (the post shader, the touch overlay) drew with its own. The
 * renderer enables blending and depth testing once and never again, so leaving
 * either off makes every translucent or alpha-cut surface draw solid.
 */
static void scene_pipeline_restore(void) {
    glUseProgram(sProgram);
    glBindVertexArray(sVao);
    glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
}

/** Draw the offscreen scene onto the window through the post shader. */
static void post_process(void) {
    float x0, y0, x1, y1;
    float quad[4][4];

    if (sSceneFbo == 0) {
        return; // the scene is already on the window
    }
    glBindFramebuffer(GL_FRAMEBUFFER, sWindowFbo);
    glViewport(0, 0, sWinWidth, sWinHeight);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // The image rectangle in NDC.
    x0 = (float) sVpX / (float) sWinWidth * 2.0f - 1.0f;
    x1 = (float) (sVpX + sVpW) / (float) sWinWidth * 2.0f - 1.0f;
    y0 = (float) sVpY / (float) sWinHeight * 2.0f - 1.0f;
    y1 = (float) (sVpY + sVpH) / (float) sWinHeight * 2.0f - 1.0f;
    // Texture v runs bottom-up like GL, and so does NDC y, so no flip. The
    // scanline phase in the shader wants v = 0 at the top of the image, which
    // 1 - v gives it; the shader only uses fract() of it, so either works.
    quad[0][0] = x0;
    quad[0][1] = y0;
    quad[0][2] = 0.0f;
    quad[0][3] = 0.0f;
    quad[1][0] = x1;
    quad[1][1] = y0;
    quad[1][2] = 1.0f;
    quad[1][3] = 0.0f;
    quad[2][0] = x0;
    quad[2][1] = y1;
    quad[2][2] = 0.0f;
    quad[2][3] = 1.0f;
    quad[3][0] = x1;
    quad[3][1] = y1;
    quad[3][2] = 1.0f;
    quad[3][3] = 1.0f;

    glUseProgram(sPostProgram);
    glBindVertexArray(sPostVao);
    glBindBuffer(GL_ARRAY_BUFFER, sPostVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STREAM_DRAW);
    glBindTexture(GL_TEXTURE_2D, sSceneTex);
    glUniform1i(sPostUniScene, 0);
    glUniform2f(sPostUniSceneLines, (float) sFbWidth, (float) sFbHeight);
    glUniform1i(sPostUniScanlines, sScanlines);
    glUniform1f(sPostUniStrength, SCANLINE_STRENGTH);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Back to the scene pipeline for the next frame. The scene texture must
    // not stay bound: the next frame renders into it, and WebGL refuses to
    // draw with a texture bound to the framebuffer it is drawing to, even
    // when the shader never samples it.
    glBindTexture(GL_TEXTURE_2D, 0);
    scene_pipeline_restore();
    sTexBound = 0;
    set_tex_mode_uniform();
}

static void set_tex_mode_uniform(void) {
    glUniform1i(sUniTexMode, sTexBound ? sTexMode : 0);
}

// ---------------------------------------------------------------------------
// Public API (see gfx.h)
// ---------------------------------------------------------------------------

// Largest 4:3 rectangle that fits the drawable, centred — or, in widescreen,
// the whole drawable, with the framebuffer the interpreter draws in widened to
// match (clamped so an absurd window can't become an absurd field of view).
static void update_viewport(void) {
    SDL_GetWindowSizeInPixels(sWindow, &sWinWidth, &sWinHeight);
    sFbWidth = sFbBaseWidth;
    if (sWidescreen && sWinHeight > 0) {
        int wide = (sWinWidth * sFbHeight + sWinHeight / 2) / sWinHeight;

        if (wide > sFbBaseWidth * 12 / 5) {
            wide = sFbBaseWidth * 12 / 5;
        }
        if (wide > sFbBaseWidth) {
            sFbWidth = wide;
        }
    }
    if (sProgram != 0) {
        glUseProgram(sProgram);
        glUniform2f(sUniScreen, (float) sFbWidth, (float) sFbHeight);
    }
    if (sWinWidth * sFbHeight >= sWinHeight * sFbWidth) {
        sVpH = sWinHeight;
        sVpW = sWinHeight * sFbWidth / sFbHeight;
    } else {
        sVpW = sWinWidth;
        sVpH = sWinWidth * sFbHeight / sFbWidth;
    }
    sVpX = (sWinWidth - sVpW) / 2;
    sVpY = (sWinHeight - sVpH) / 2;
    scene_buffers_create();
    scene_target_bind();
}

// Startup render state. Every one of these is driven per material by the
// interpreter (apply_render_mode in main.c); they also open the trace, so a run
// without a display sets them too.
static void set_state_defaults(void) {
    static const unsigned char black[4] = { 0, 0, 0, 255 };

    gfx_set_alpha_test(0.0f);
    gfx_set_texenv_modulate();
    gfx_set_fog(0, black);
}

void gfx_window_init(int width, int height, const GfxOptions *options) {
    const char *missing;
    const char *dump;
    int scale = options != NULL && options->scale > 0 ? options->scale : 3;

    sFbWidth = width;
    sFbBaseWidth = width;
    sFbHeight = height;
    sFullscreen = options == NULL || options->fullscreen;
    sScanlines = options == NULL || options->scanlines;
    sWidescreen = options == NULL || options->widescreen;

#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady(); // we provide a plain main(); on Android and iOS SDL owns main()
#endif
#ifdef SDL_PLATFORM_IOS
    pc_ios_configure(); // ios/app.c: landscape lock and the other phone hints
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "GFX: SDL_Init failed: %s\n", SDL_GetError());
        set_state_defaults();
        return;
    }
    // A widescreen window opens at 16:9; fullscreen takes the display's shape either way.
    if (!create_context((sWidescreen ? height * 16 / 9 : width) * scale, height * scale)) {
        fprintf(stderr, "GFX: no usable OpenGL context; running without video\n");
        set_state_defaults();
        return;
    }
    missing = dkr_gl_load((void *(*) (const char *) ) SDL_GL_GetProcAddress);
    if (missing != NULL) {
        fprintf(stderr, "GFX: GL entry point %s not found; running without video\n", missing);
        SDL_GL_DestroyContext(sContext);
        SDL_DestroyWindow(sWindow);
        sWindow = NULL;
        set_state_defaults();
        return;
    }
    printf("GFX: %s / %s\n", (const char *) glGetString(GL_RENDERER), (const char *) glGetString(GL_VERSION));
    // The game's logic is paced by pc_retrace_wait(); vsync only decides whether
    // the frames presented in between wait for the display or run uncapped.
    SDL_GL_SetSwapInterval(options != NULL && options->vsync ? 1 : 0);

    if (!build_program()) {
        SDL_GL_DestroyContext(sContext);
        SDL_DestroyWindow(sWindow);
        sWindow = NULL;
        set_state_defaults();
        return;
    }

    glGenVertexArrays(1, &sPostVao);
    glBindVertexArray(sPostVao);
    glGenBuffers(1, &sPostVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sPostVbo);
    glEnableVertexAttribArray(sPostAttrPos);
    glVertexAttribPointer(sPostAttrPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void *) 0);
    glEnableVertexAttribArray(sPostAttrUV);
    glVertexAttribPointer(sPostAttrUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void *) (2 * sizeof(float)));

    update_viewport();

    glGenVertexArrays(1, &sVao);
    glBindVertexArray(sVao);
    glGenBuffers(1, &sVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    glEnableVertexAttribArray(sAttrPos);
    glVertexAttribPointer(sAttrPos, 4, GL_FLOAT, GL_FALSE, sizeof(GfxGpuVert), (const void *) offsetof(GfxGpuVert, x));
    glEnableVertexAttribArray(sAttrUV);
    glVertexAttribPointer(sAttrUV, 2, GL_FLOAT, GL_FALSE, sizeof(GfxGpuVert), (const void *) offsetof(GfxGpuVert, u));
    glEnableVertexAttribArray(sAttrFog);
    glVertexAttribPointer(sAttrFog, 1, GL_FLOAT, GL_FALSE, sizeof(GfxGpuVert),
                          (const void *) offsetof(GfxGpuVert, fog));
    glEnableVertexAttribArray(sAttrColor);
    glVertexAttribPointer(sAttrColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GfxGpuVert),
                          (const void *) offsetof(GfxGpuVert, r));

    glUseProgram(sProgram);
    glUniform2f(sUniScreen, (float) width, (float) height);
    glUniform1i(sUniTex, 0);
    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);       // the interpreter culls in screen space itself
    glPolygonOffset(-1.0f, -1.0f); // nearer is smaller here: pull decals towards zero
    set_state_defaults();

    dump = getenv("DKR_DUMP_FRAME");
    while (dump != NULL && *dump != '\0' && sDumpFrameCount < MAX_DUMP_FRAMES) {
        sDumpFrames[sDumpFrameCount++] = atol(dump);
        dump = strchr(dump, ',');
        if (dump != NULL) {
            dump++;
        }
    }
    dump = getenv("DKR_DUMP_PRESENT");
    if (dump != NULL && sscanf(dump, "%ld-%ld", &sDumpPresentLo, &sDumpPresentHi) == 1) {
        sDumpPresentHi = sDumpPresentLo;
    }
    dump = getenv("DKR_DUMP_EVERY");
    if (dump != NULL) {
        sDumpEvery = atol(dump);
    }
}

unsigned int gfx_frame_count(void) {
    return sFrameCount;
}

static int dump_wanted(long frame) {
    int i;

    if (sDumpEvery > 0 && frame % sDumpEvery == 0) {
        return 1;
    }
    for (i = 0; i < sDumpFrameCount; i++) {
        if (sDumpFrames[i] == frame) {
            return 1;
        }
    }
    return 0;
}

// The RDP's cms/cmt are a 2-bit field, not a flag: bit 0 is mirror, bit 1 is
// clamp (G_TX_MIRROR = 1, G_TX_CLAMP = 2, plain wrap = 0). Clamp wins if both.
static GLint wrap_mode(int cm) {
    if (cm & 0x2) {
        return GL_CLAMP_TO_EDGE;
    }
    if (cm & 0x1) {
        return GL_MIRRORED_REPEAT;
    }
    return GL_REPEAT;
}

unsigned int gfx_create_texture(const void *rgba, int width, int height, int cmS, int cmT) {
    static GLuint sHeadlessId;
    GLuint id = 0;

    if (sWindow == NULL) {
        // No GL: hand out stand-in handles so a headless trace still names and
        // hashes every texture the game binds.
        id = ++sHeadlessId;
        pc_trace_texture_create(id, rgba, width, height, cmS, cmT);
        return id;
    }
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode(cmS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode(cmT));
    pc_trace_texture_create(id, rgba, width, height, cmS, cmT);
    return id;
}

void gfx_delete_texture(unsigned int handle) {
    GLuint id = handle;

    if (sWindow == NULL || handle == 0) {
        return;
    }
    glDeleteTextures(1, &id);
}

void gfx_bind_texture(unsigned int handle) {
    pc_trace_texture_bind(handle);
    pc_trace_mark("bind");
    if (sWindow == NULL) {
        return;
    }
    if (handle == 0) {
        if (sTexBound) {
            sTexBound = 0;
            set_tex_mode_uniform();
        }
        return;
    }
    glBindTexture(GL_TEXTURE_2D, handle);
    if (!sTexBound) {
        sTexBound = 1;
        set_tex_mode_uniform();
    }
}

void gfx_set_texture_filter(int point) {
    GLint filter;

    pc_trace_gfx_u32(0x100 | (point ? 1u : 0u));
    pc_trace_mark("filter");
    if (sWindow == NULL) {
        return;
    }
    if (sTexBound == 0) {
        return; // nothing bound to set it on (WebGL reports the attempt as an error)
    }
    filter = point ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

/** Texel-weighted lerp from the vertex colour towards `color` (GL_BLEND texenv). */
void gfx_set_texenv_blend(const unsigned char color[4]) {
    float c[4];

    pc_trace_gfx_u32(0x200);
    pc_trace_gfx(color, 4);
    pc_trace_mark("texenv_blend");
    if (sWindow == NULL) {
        return;
    }
    c[0] = color[0] / 255.0f;
    c[1] = color[1] / 255.0f;
    c[2] = color[2] / 255.0f;
    c[3] = color[3] / 255.0f;
    if (memcmp(c, sEnvColor, sizeof(c)) != 0) {
        memcpy(sEnvColor, c, sizeof(c));
        glUniform4f(sUniEnvColor, c[0], c[1], c[2], c[3]);
    }
    if (sTexMode != 2) {
        sTexMode = 2;
        set_tex_mode_uniform();
    }
}

/** Plain texel * vertex colour, which is what the 3D path wants. */
void gfx_set_texenv_modulate(void) {
    pc_trace_gfx_u32(0x300);
    pc_trace_mark("texenv_modulate");
    if (sWindow == NULL) {
        return;
    }
    if (sTexMode != 1) {
        sTexMode = 1;
        set_tex_mode_uniform();
    }
}

void gfx_set_fog(int enable, const unsigned char color[4]) {
    float c[4];

    enable = enable ? 1 : 0;
    pc_trace_gfx_u32(0x400 | (unsigned) enable);
    if (enable) {
        pc_trace_gfx(color, 4);
    }
    pc_trace_mark("fog");
    if (sWindow == NULL) {
        return;
    }
    if (enable != sFogEnable) {
        sFogEnable = enable;
        glUniform1i(sUniFogEnable, enable);
    }
    if (!enable) {
        return;
    }
    c[0] = color[0] / 255.0f;
    c[1] = color[1] / 255.0f;
    c[2] = color[2] / 255.0f;
    c[3] = color[3] / 255.0f;
    if (memcmp(c, sFogColor, sizeof(c)) != 0) {
        memcpy(sFogColor, c, sizeof(c));
        glUniform4f(sUniFogColor, c[0], c[1], c[2], c[3]);
    }
}

void gfx_set_alpha_test(float ref) {
    pc_trace_gfx_u32(0x500);
    pc_trace_gfx_f32(ref);
    pc_trace_mark("alpha_test");
    if (sWindow == NULL) {
        return;
    }
    if (ref != sAlphaRef) {
        sAlphaRef = ref;
        glUniform1f(sUniAlphaRef, ref);
    }
}

void gfx_set_depth_test(int enable) {
    pc_trace_gfx_u32(0x600 | (enable ? 1u : 0u));
    pc_trace_mark("depth_test");
    if (sWindow == NULL) {
        return;
    }
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void gfx_set_depth_write(int enable) {
    pc_trace_gfx_u32(0x700 | (enable ? 1u : 0u));
    pc_trace_mark("depth_write");
    if (sWindow == NULL) {
        return;
    }
    glDepthMask(enable ? GL_TRUE : GL_FALSE);
}

void gfx_set_depth_offset(int enable) {
    pc_trace_gfx_u32(0x800 | (enable ? 1u : 0u));
    pc_trace_mark("depth_offset");
    if (sWindow == NULL) {
        return;
    }
    if (enable) {
        glEnable(GL_POLYGON_OFFSET_FILL);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

// `x1`/`y1` are exclusive — the caller has already turned the RDP's inclusive
// lower-right corner into one past the end.
void gfx_set_scissor(float x0, float y0, float x1, float y1) {
    float sx, sy;
    int w, h, gx, gy;

    pc_trace_gfx_u32(0x900);
    pc_trace_gfx_f32(x0);
    pc_trace_gfx_f32(y0);
    pc_trace_gfx_f32(x1);
    pc_trace_gfx_f32(y1);
    pc_trace_mark("scissor");
    if (sWindow == NULL) {
        return;
    }
    sx = (float) sVpW / (float) sFbWidth;
    sy = (float) sVpH / (float) sFbHeight;
    // GL measures from the bottom-left, so y flips; offset into the letterbox
    // when drawing straight to the window (the offscreen buffer is the image).
    gx = (sSceneFbo != 0 ? 0 : sVpX) + (int) (x0 * sx);
    gy = (sSceneFbo != 0 ? 0 : sVpY) + (int) ((sFbHeight - y1) * sy);
    w = (int) ((x1 - x0) * sx);
    h = (int) ((y1 - y0) * sy);
    if (w < 0 || h < 0) {
        // An empty rect means draw nothing, which is not the same as "no clip".
        w = 0;
        h = 0;
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(gx, gy, w, h);
}

// "No clip" still means the 4:3 image: the letterbox bars stay black.
void gfx_disable_scissor(void) {
    pc_trace_gfx_u32(0xA00);
    pc_trace_mark("scissor_off");
    if (sWindow == NULL) {
        return;
    }
    glEnable(GL_SCISSOR_TEST);
    if (sSceneFbo != 0) {
        glScissor(0, 0, sVpW, sVpH);
    } else {
        glScissor(sVpX, sVpY, sVpW, sVpH);
    }
}

float gfx_wide_scale(void) {
    return (float) sFbWidth / (float) sFbBaseWidth;
}

void gfx_frame_begin(void) {
    if (sWindow == NULL) {
        return;
    }
    // glClear honours both the depth mask and the scissor, so neither may be left
    // where the previous frame's last command put it, or the clear silently does
    // nothing (or only part of the screen).
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

#define GFX_MAX_BATCH_VERTS 4096
static GfxGpuVert sBatch[GFX_MAX_BATCH_VERTS];

void gfx_draw_tris(const GfxTriVert *verts, int count) {
    int i, done = 0;

    if (count <= 0) {
        return;
    }
    pc_trace_gfx_u32(0xB00);
    pc_trace_gfx_u32((uint32_t) count);
    pc_trace_gfx(verts, (size_t) count * sizeof(*verts));
    pc_trace_mark("draw");
    if (sWindow == NULL) {
        return;
    }
    // DKR_TRACE_DRAW=<frame>:<n>: print the vertices of draw call n (0-based) of
    // that frame, to compare a diverging draw between hosts.
    {
        static long sWantFrame = -2, sWantDraw;
        static unsigned int sDrawFrame = ~0u;
        static long sDrawIndex;

        if (sWantFrame == -2) {
            const char *want = getenv("DKR_TRACE_DRAW");
            sWantFrame = -1;
            if (want != NULL && sscanf(want, "%ld:%ld", &sWantFrame, &sWantDraw) != 2) {
                sWantFrame = -1;
            }
        }
        if (sDrawFrame != sFrameCount) {
            sDrawFrame = sFrameCount;
            sDrawIndex = 0;
        }
        if (sWantFrame >= 0 && (long) sFrameCount + 1 == sWantFrame && sDrawIndex == sWantDraw) {
            for (i = 0; i < count; i++) {
                const GfxTriVert *v = &verts[i];
                fprintf(stderr, "DRAW %ld:%ld v%d %a %a %a %a uv %a %a fog %a rgba %u %u %u %u\n", sWantFrame,
                        sWantDraw, i, v->x, v->y, v->z, v->w, v->u, v->v, v->fog, v->r, v->g, v->b, v->a);
            }
        }
        sDrawIndex++;
    }
    while (done < count) {
        int n = count - done;
        if (n > GFX_MAX_BATCH_VERTS) {
            n = GFX_MAX_BATCH_VERTS - (GFX_MAX_BATCH_VERTS % 3);
        }
        for (i = 0; i < n; i++) {
            const GfxTriVert *v = &verts[done + i];
            GfxGpuVert *g = &sBatch[i];
            // Positions arrive divided by w; multiply w back in so the rasterizer
            // interpolates perspective-correctly (see the vertex shader).
            g->x = v->x * v->w;
            g->y = v->y * v->w;
            g->z = v->z * v->w;
            g->w = v->w;
            g->u = v->u;
            g->v = v->v;
            g->fog = v->fog;
            g->r = v->r;
            g->g = v->g;
            g->b = v->b;
            g->a = v->a;
        }
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (n * sizeof(GfxGpuVert)), sBatch, GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, n);
        done += n;
    }
}

// DKR_DUMP_FRAME=<n>: save frame n as dkr_frame_<n>.ppm in the working directory.
// A screenshot facility the game carries itself, for headless/remote testing.
static void dump_frame_named(const char *prefix, unsigned int n) {
    int w = sWinWidth, h = sWinHeight;
    unsigned char *pixels = malloc((size_t) w * h * 4);
    char name[64];
    FILE *f;
    int y;

    if (pixels == NULL) {
        return;
    }
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    snprintf(name, sizeof(name), "dkr_%s_%u.ppm", prefix, n);
    f = fopen(name, "wb");
    if (f != NULL) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (y = h - 1; y >= 0; y--) { // GL rows run bottom-up
            const unsigned char *row = pixels + (size_t) y * w * 4;
            int x;
            for (x = 0; x < w; x++) {
                fwrite(row + x * 4, 1, 3, f);
            }
        }
        fclose(f);
        printf("GFX: wrote %s\n", name);
    }
    free(pixels);
}

static void dump_frame(void) {
    dump_frame_named("frame", sFrameCount);
}

static int present(int dumpLogicFrame);

int gfx_frame_end(void) {
    int presented = 0;

    sFrameCount++;
    if (sWindow != NULL) {
        presented = present(dump_wanted((long) sFrameCount));
    }
    // The trace hashes what the renderer is handed, not pixels, so a run with no
    // display (ssh into a Pi, a CI box) counts frames and finishes like any other.
    pc_trace_frame_end();
    return presented;
}

int gfx_window_valid(void) {
    return sWindow != NULL;
}

// Whether the live GL context is OpenGL ES (vs. desktop core). The touch overlay
// needs it to pick the matching GLSL version, same as the renderer's shaders.
int gfx_is_gles(void) {
    return sIsES;
}

int gfx_present(void) {
    return present(0);
}

static int present(int dumpLogicFrame) {
    SDL_Event event;
    SDL_WindowFlags flags;

    if (sWindow == NULL) {
        return 0;
    }

    // Pump before presenting. On Windows, SwapBuffers can block for seconds when
    // a fullscreen OpenGL window loses focus or is covered by a system overlay.
    // Pumping only after the swap creates a feedback loop: the focus/minimize
    // event that tells us to stop presenting is itself stuck behind that swap.
    while (SDL_PollEvent(&event)) {
        vcontrols_event(&event, sWinWidth, sWinHeight);
        if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
            SDL_Quit();
            exit(0);
        }
        if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            update_viewport();
        }
        if (event.type == SDL_EVENT_KEY_DOWN && (event.key.key == SDLK_TAB || event.key.key == SDLK_F11)) {
#ifdef _WIN32
            if (sFullscreen) {
                SDL_SetWindowBordered(sWindow, true);
                if (sWindowedBoundsValid) {
                    SDL_SetWindowSize(sWindow, sWindowedW, sWindowedH);
                    SDL_SetWindowPosition(sWindow, sWindowedX, sWindowedY);
                }
                sFullscreen = 0;
            } else {
                SDL_Rect bounds;
                SDL_DisplayID display;

                SDL_GetWindowPosition(sWindow, &sWindowedX, &sWindowedY);
                SDL_GetWindowSize(sWindow, &sWindowedW, &sWindowedH);
                sWindowedBoundsValid = 1;
                display = SDL_GetDisplayForWindow(sWindow);
                if (display && SDL_GetDisplayBounds(display, &bounds)) {
                    SDL_SetWindowBordered(sWindow, false);
                    SDL_SetWindowPosition(sWindow, bounds.x - 1, bounds.y - 1);
                    SDL_SetWindowSize(sWindow, bounds.w + 2, bounds.h + 2);
                    sFullscreen = 1;
                }
            }
#else
            sFullscreen = !sFullscreen;
            SDL_SetWindowFullscreen(sWindow, sFullscreen);
#endif
        }
        if (event.type == SDL_EVENT_KEY_DOWN && (event.key.key == SDLK_1 || event.key.key == SDLK_F2)) {
            sScanlines = !sScanlines;
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_2) {
            sWidescreen = !sWidescreen;
            update_viewport();
        }
    }

    flags = SDL_GetWindowFlags(sWindow);
    if ((flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED)) ||
        (sFullscreen && !(flags & SDL_WINDOW_INPUT_FOCUS))) {
        return 0;
    }

    post_process();
    sPresentCount++;
    if (dumpLogicFrame) {
        dump_frame();
    }
    if ((long) sPresentCount >= sDumpPresentLo && (long) sPresentCount <= sDumpPresentHi) {
        dump_frame_named("present", sPresentCount);
    }
    vcontrols_render(sWinWidth, sWinHeight); // touch controls, drawn above the game
    if (vcontrols_active()) {
        // The overlay drew with its own program and VAO and turned blending and
        // depth testing off; the next frame needs the scene pipeline back.
        scene_pipeline_restore();
    }
    if (sWindowRbo != 0) {
        // iOS presents whatever renderbuffer is bound; the offscreen scene's
        // depth renderbuffer was bound last (SDL_uikitopenglview.m swapBuffers).
        glBindRenderbuffer(GL_RENDERBUFFER, sWindowRbo);
    }
    SDL_GL_SwapWindow(sWindow);
    scene_target_bind();
    return 1;
}
