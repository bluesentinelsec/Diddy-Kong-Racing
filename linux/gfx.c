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
#include "trace.h"
#include "gl3_loader.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // SDL_SetMainReady; SDL_MAIN_HANDLED keeps our plain main()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shader_dkr_frag.h"
#include "shader_dkr_vert.h"

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
static int sFbHeight = 240;
static int sWinWidth = 320;
static int sWinHeight = 240;
// The 4:3 image, centred in the window (letter/pillar-boxed), in window pixels.
static int sVpX, sVpY, sVpW = 320, sVpH = 240;
static int sFullscreen;

static GLuint sProgram;
static GLuint sVao;
static GLuint sVbo;
static GLint sAttrPos, sAttrUV, sAttrFog, sAttrColor;
static GLint sUniScreen, sUniTex, sUniTexMode, sUniEnvColor, sUniFogEnable, sUniFogColor, sUniAlphaRef;

// Shadowed uniform state, so a redundant gfx_set_* costs nothing.
static int sTexMode = 1;  // what the shader does with the texel (0/1/2)
static int sTexBound = 0; // whether a texture is bound (gates sTexMode)
static int sFogEnable = -1;
static float sFogColor[4] = { -1, -1, -1, -1 };
static float sEnvColor[4] = { -1, -1, -1, -1 };
static float sAlphaRef = -1.0f;

static unsigned int sFrameCount;
static long sDumpFrame = -1; // DKR_DUMP_FRAME: write that frame to a .ppm

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

static int try_create_context(const GlConfig *cfg, int width, int height) {
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
    if (sFullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN; // borderless, at the desktop resolution
    }
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
    sContext = SDL_GL_CreateContext(sWindow);
    if (sContext == NULL) {
        fprintf(stderr, "GFX: %s context unavailable: %s\n", cfg->name, SDL_GetError());
        SDL_DestroyWindow(sWindow);
        sWindow = NULL;
        return 0;
    }
    sIsES = (cfg->profile == SDL_GL_CONTEXT_PROFILE_ES);
    return 1;
}

static int create_context(int width, int height) {
    static const GlConfig core33 = { SDL_GL_CONTEXT_PROFILE_CORE, 3, 3, "OpenGL 3.3 core" };
    static const GlConfig es30 = { SDL_GL_CONTEXT_PROFILE_ES, 3, 0, "OpenGL ES 3.0" };
    const GlConfig *order[2];
    int i;

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__) || \
    (defined(__linux__) && (defined(__arm__) || defined(__aarch64__)))
    // Mesa's V3D (Raspberry Pi) exposes GL ES 3.1 but only desktop GL 3.1.
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

static int build_program(void) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, (const char *) shader_dkr_vert, shader_dkr_vert_len);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, (const char *) shader_dkr_frag, shader_dkr_frag_len);
    GLint ok = 0;

    if (vs == 0 || fs == 0) {
        return 0;
    }
    sProgram = glCreateProgram();
    glAttachShader(sProgram, vs);
    glAttachShader(sProgram, fs);
    glLinkProgram(sProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGetProgramiv(sProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei len = 0;
        glGetProgramInfoLog(sProgram, sizeof(log), &len, log);
        fprintf(stderr, "GFX: shader program failed to link:\n%.*s\n", (int) len, log);
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
    return 1;
}

static void set_tex_mode_uniform(void) {
    glUniform1i(sUniTexMode, sTexBound ? sTexMode : 0);
}

// ---------------------------------------------------------------------------
// Public API (see gfx.h)
// ---------------------------------------------------------------------------

// Largest 4:3 rectangle that fits the drawable, centred.
static void update_viewport(void) {
    SDL_GetWindowSizeInPixels(sWindow, &sWinWidth, &sWinHeight);
    if (sWinWidth * sFbHeight >= sWinHeight * sFbWidth) {
        sVpH = sWinHeight;
        sVpW = sWinHeight * sFbWidth / sFbHeight;
    } else {
        sVpW = sWinWidth;
        sVpH = sWinWidth * sFbHeight / sFbWidth;
    }
    sVpX = (sWinWidth - sVpW) / 2;
    sVpY = (sWinHeight - sVpH) / 2;
    glViewport(sVpX, sVpY, sVpW, sVpH);
}

void gfx_window_init(int width, int height, const GfxOptions *options) {
    const char *missing;
    const char *dump;
    int scale = options != NULL && options->scale > 0 ? options->scale : 3;

    sFbWidth = width;
    sFbHeight = height;
    sFullscreen = options == NULL || options->fullscreen;

    SDL_SetMainReady(); // we provide a plain main(); no SDL_main redirection on Windows
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "GFX: SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    if (!create_context(width * scale, height * scale)) {
        fprintf(stderr, "GFX: no usable OpenGL context; running without video\n");
        return;
    }
    missing = dkr_gl_load((void *(*) (const char *) ) SDL_GL_GetProcAddress);
    if (missing != NULL) {
        fprintf(stderr, "GFX: GL entry point %s not found; running without video\n", missing);
        SDL_GL_DestroyContext(sContext);
        SDL_DestroyWindow(sWindow);
        sWindow = NULL;
        return;
    }
    printf("GFX: %s / %s\n", (const char *) glGetString(GL_RENDERER), (const char *) glGetString(GL_VERSION));
    SDL_GL_SetSwapInterval(0); // pc_retrace_wait() already paces the game

    if (!build_program()) {
        SDL_GL_DestroyContext(sContext);
        SDL_DestroyWindow(sWindow);
        sWindow = NULL;
        return;
    }

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

    // Startup defaults; every one of these is driven per material by the
    // interpreter (apply_render_mode in main.c).
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);       // the interpreter culls in screen space itself
    glPolygonOffset(-1.0f, -1.0f); // nearer is smaller here: pull decals towards zero
    gfx_set_alpha_test(0.0f);
    gfx_set_texenv_modulate();
    {
        static const unsigned char black[4] = { 0, 0, 0, 255 };
        gfx_set_fog(0, black);
    }

    dump = getenv("DKR_DUMP_FRAME");
    if (dump != NULL) {
        sDumpFrame = atol(dump);
    }
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
    GLuint id = 0;

    if (sWindow == NULL) {
        return 0;
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
    if (sWindow == NULL) {
        return;
    }
    pc_trace_texture_bind(handle);
    pc_trace_mark("bind");
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

    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x100 | (point ? 1u : 0u));
    pc_trace_mark("filter");
    filter = point ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

/** Texel-weighted lerp from the vertex colour towards `color` (GL_BLEND texenv). */
void gfx_set_texenv_blend(const unsigned char color[4]) {
    float c[4];

    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x200);
    pc_trace_gfx(color, 4);
    pc_trace_mark("texenv_blend");
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
    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x300);
    pc_trace_mark("texenv_modulate");
    if (sTexMode != 1) {
        sTexMode = 1;
        set_tex_mode_uniform();
    }
}

void gfx_set_fog(int enable, const unsigned char color[4]) {
    float c[4];

    if (sWindow == NULL) {
        return;
    }
    enable = enable ? 1 : 0;
    pc_trace_gfx_u32(0x400 | (unsigned) enable);
    if (enable) {
        pc_trace_gfx(color, 4);
    }
    pc_trace_mark("fog");
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
    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x500);
    pc_trace_gfx_f32(ref);
    pc_trace_mark("alpha_test");
    if (ref != sAlphaRef) {
        sAlphaRef = ref;
        glUniform1f(sUniAlphaRef, ref);
    }
}

void gfx_set_depth_test(int enable) {
    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x600 | (enable ? 1u : 0u));
    pc_trace_mark("depth_test");
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void gfx_set_depth_write(int enable) {
    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x700 | (enable ? 1u : 0u));
    pc_trace_mark("depth_write");
    glDepthMask(enable ? GL_TRUE : GL_FALSE);
}

void gfx_set_depth_offset(int enable) {
    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x800 | (enable ? 1u : 0u));
    pc_trace_mark("depth_offset");
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

    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0x900);
    pc_trace_gfx_f32(x0);
    pc_trace_gfx_f32(y0);
    pc_trace_gfx_f32(x1);
    pc_trace_gfx_f32(y1);
    pc_trace_mark("scissor");
    sx = (float) sVpW / (float) sFbWidth;
    sy = (float) sVpH / (float) sFbHeight;
    // GL measures from the bottom-left, so y flips; offset into the letterbox.
    gx = sVpX + (int) (x0 * sx);
    gy = sVpY + (int) ((sFbHeight - y1) * sy);
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
    if (sWindow == NULL) {
        return;
    }
    pc_trace_gfx_u32(0xA00);
    pc_trace_mark("scissor_off");
    glEnable(GL_SCISSOR_TEST);
    glScissor(sVpX, sVpY, sVpW, sVpH);
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

    if (sWindow == NULL || count <= 0) {
        return;
    }
    pc_trace_gfx_u32(0xB00);
    pc_trace_gfx_u32((uint32_t) count);
    pc_trace_gfx(verts, (size_t) count * sizeof(*verts));
    pc_trace_mark("draw");
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
static void dump_frame(void) {
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
    snprintf(name, sizeof(name), "dkr_frame_%ld.ppm", sDumpFrame);
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

void gfx_frame_end(void) {
    SDL_Event event;

    if (sWindow == NULL) {
        return;
    }
    sFrameCount++;
    if (sDumpFrame >= 0 && (long) sFrameCount == sDumpFrame) {
        dump_frame();
    }
    SDL_GL_SwapWindow(sWindow);
    pc_trace_frame_end();

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
            SDL_Quit();
            exit(0);
        }
        if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            update_viewport();
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F11) {
            sFullscreen = !sFullscreen;
            SDL_SetWindowFullscreen(sWindow, sFullscreen);
        }
    }
}
