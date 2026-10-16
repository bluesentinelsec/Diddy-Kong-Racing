#ifndef DKR_GL3_LOADER_H
#define DKR_GL3_LOADER_H

/*
 * Minimal OpenGL 3.3 core / OpenGL ES 3.0 binding for the renderer.
 *
 * Only the entry points linux/gfx.c uses, resolved at runtime through
 * SDL_GL_GetProcAddress, so the executable links against SDL3 alone: no libGL,
 * libGLESv2, or platform GL headers. The same binary then works wherever SDL can
 * give us either profile (desktop GL on Windows/macOS/Linux, GL ES on the
 * Raspberry Pi, WebGL 2 under Emscripten, GL ES on mobile).
 */

#include <stddef.h>
#include <stdint.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef float GLfloat;
typedef char GLchar;
typedef unsigned int GLbitfield;
typedef intptr_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef void GLvoid;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_LESS 0x0201
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_SCISSOR_TEST 0x0C11
#define GL_BLEND 0x0BE2
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_FLOAT 0x1406
#define GL_RGBA 0x1908
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_TEXTURE0 0x84C0
#define GL_ARRAY_BUFFER 0x8892
#define GL_STREAM_DRAW 0x88E0
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_VERSION 0x1F02
#define GL_RENDERER 0x1F01
#define GL_NO_ERROR 0
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5

#define DKR_GL_FUNCS(X)                                                                                        \
    X(const GLubyte *, glGetString, (GLenum name))                                                             \
    X(GLenum, glGetError, (void) )                                                                             \
    X(void, glEnable, (GLenum cap))                                                                            \
    X(void, glDisable, (GLenum cap))                                                                           \
    X(void, glViewport, (GLint x, GLint y, GLsizei w, GLsizei h))                                              \
    X(void, glClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a))                                        \
    X(void, glClear, (GLbitfield mask))                                                                        \
    X(void, glDepthFunc, (GLenum func))                                                                        \
    X(void, glDepthMask, (GLboolean flag))                                                                     \
    X(void, glBlendFunc, (GLenum sfactor, GLenum dfactor))                                                     \
    X(void, glPolygonOffset, (GLfloat factor, GLfloat units))                                                  \
    X(void, glScissor, (GLint x, GLint y, GLsizei w, GLsizei h))                                               \
    X(void, glPixelStorei, (GLenum pname, GLint param))                                                        \
    X(void, glReadPixels, (GLint x, GLint y, GLsizei w, GLsizei h, GLenum fmt, GLenum type, void *data))       \
    X(void, glGenTextures, (GLsizei n, GLuint * textures))                                                     \
    X(void, glDeleteTextures, (GLsizei n, const GLuint *textures))                                             \
    X(void, glBindTexture, (GLenum target, GLuint texture))                                                    \
    X(void, glActiveTexture, (GLenum texture))                                                                 \
    X(void, glTexImage2D,                                                                                      \
      (GLenum target, GLint level, GLint internalformat, GLsizei w, GLsizei h, GLint border, GLenum format,    \
       GLenum type, const void *pixels))                                                                       \
    X(void, glTexParameteri, (GLenum target, GLenum pname, GLint param))                                       \
    X(GLuint, glCreateShader, (GLenum type))                                                                   \
    X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length))  \
    X(void, glCompileShader, (GLuint shader))                                                                  \
    X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint * params))                                      \
    X(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog))          \
    X(void, glDeleteShader, (GLuint shader))                                                                   \
    X(GLuint, glCreateProgram, (void) )                                                                        \
    X(void, glAttachShader, (GLuint program, GLuint shader))                                                   \
    X(void, glLinkProgram, (GLuint program))                                                                   \
    X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint * params))                                    \
    X(void, glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog))        \
    X(void, glUseProgram, (GLuint program))                                                                    \
    X(GLint, glGetUniformLocation, (GLuint program, const GLchar *name))                                       \
    X(GLint, glGetAttribLocation, (GLuint program, const GLchar *name))                                        \
    X(void, glUniform1i, (GLint location, GLint v0))                                                           \
    X(void, glUniform1f, (GLint location, GLfloat v0))                                                         \
    X(void, glUniform2f, (GLint location, GLfloat v0, GLfloat v1))                                             \
    X(void, glUniform4f, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))                     \
    X(void, glGenVertexArrays, (GLsizei n, GLuint * arrays))                                                   \
    X(void, glBindVertexArray, (GLuint array))                                                                 \
    X(void, glGenBuffers, (GLsizei n, GLuint * buffers))                                                       \
    X(void, glBindBuffer, (GLenum target, GLuint buffer))                                                      \
    X(void, glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage))                    \
    X(void, glEnableVertexAttribArray, (GLuint index))                                                         \
    X(void, glVertexAttribPointer,                                                                             \
      (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))      \
    X(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count))                                           \
    X(void, glGenFramebuffers, (GLsizei n, GLuint * framebuffers))                                             \
    X(void, glDeleteFramebuffers, (GLsizei n, const GLuint *framebuffers))                                     \
    X(void, glBindFramebuffer, (GLenum target, GLuint framebuffer))                                            \
    X(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint tex, GLint l)) \
    X(void, glGenRenderbuffers, (GLsizei n, GLuint * renderbuffers))                                           \
    X(void, glDeleteRenderbuffers, (GLsizei n, const GLuint *renderbuffers))                                   \
    X(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer))                                          \
    X(void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height))      \
    X(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum rbtarget, GLuint rb))         \
    X(GLenum, glCheckFramebufferStatus, (GLenum target))

#define DKR_GL_DECLARE(ret, name, args) \
    typedef ret(*dkr_##name##_t) args;  \
    extern dkr_##name##_t dkr_##name;
DKR_GL_FUNCS(DKR_GL_DECLARE)
#undef DKR_GL_DECLARE

// Make the call sites read like plain GL.
#define DKR_GL_ALIAS(ret, name, args)
#define glGetString dkr_glGetString
#define glGetError dkr_glGetError
#define glEnable dkr_glEnable
#define glDisable dkr_glDisable
#define glViewport dkr_glViewport
#define glClearColor dkr_glClearColor
#define glClear dkr_glClear
#define glDepthFunc dkr_glDepthFunc
#define glDepthMask dkr_glDepthMask
#define glBlendFunc dkr_glBlendFunc
#define glPolygonOffset dkr_glPolygonOffset
#define glScissor dkr_glScissor
#define glPixelStorei dkr_glPixelStorei
#define glReadPixels dkr_glReadPixels
#define glGenTextures dkr_glGenTextures
#define glDeleteTextures dkr_glDeleteTextures
#define glBindTexture dkr_glBindTexture
#define glActiveTexture dkr_glActiveTexture
#define glTexImage2D dkr_glTexImage2D
#define glTexParameteri dkr_glTexParameteri
#define glCreateShader dkr_glCreateShader
#define glShaderSource dkr_glShaderSource
#define glCompileShader dkr_glCompileShader
#define glGetShaderiv dkr_glGetShaderiv
#define glGetShaderInfoLog dkr_glGetShaderInfoLog
#define glDeleteShader dkr_glDeleteShader
#define glCreateProgram dkr_glCreateProgram
#define glAttachShader dkr_glAttachShader
#define glLinkProgram dkr_glLinkProgram
#define glGetProgramiv dkr_glGetProgramiv
#define glGetProgramInfoLog dkr_glGetProgramInfoLog
#define glUseProgram dkr_glUseProgram
#define glGetUniformLocation dkr_glGetUniformLocation
#define glGetAttribLocation dkr_glGetAttribLocation
#define glUniform1i dkr_glUniform1i
#define glUniform1f dkr_glUniform1f
#define glUniform2f dkr_glUniform2f
#define glUniform4f dkr_glUniform4f
#define glGenVertexArrays dkr_glGenVertexArrays
#define glBindVertexArray dkr_glBindVertexArray
#define glGenBuffers dkr_glGenBuffers
#define glBindBuffer dkr_glBindBuffer
#define glBufferData dkr_glBufferData
#define glEnableVertexAttribArray dkr_glEnableVertexAttribArray
#define glVertexAttribPointer dkr_glVertexAttribPointer
#define glDrawArrays dkr_glDrawArrays
#define glGenFramebuffers dkr_glGenFramebuffers
#define glDeleteFramebuffers dkr_glDeleteFramebuffers
#define glBindFramebuffer dkr_glBindFramebuffer
#define glFramebufferTexture2D dkr_glFramebufferTexture2D
#define glGenRenderbuffers dkr_glGenRenderbuffers
#define glDeleteRenderbuffers dkr_glDeleteRenderbuffers
#define glBindRenderbuffer dkr_glBindRenderbuffer
#define glRenderbufferStorage dkr_glRenderbufferStorage
#define glFramebufferRenderbuffer dkr_glFramebufferRenderbuffer
#define glCheckFramebufferStatus dkr_glCheckFramebufferStatus

// Resolves every entry point above through `getProc` (SDL_GL_GetProcAddress).
// Returns the name of the first function that could not be found, or NULL.
const char *dkr_gl_load(void *(*getProc)(const char *name));

#endif
