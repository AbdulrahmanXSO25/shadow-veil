#pragma once
// OpenGL 3.3 core entry points, loaded at runtime through SDL so the same code runs on
// Linux (Mesa / NVIDIA, X11 or Wayland) and Windows (where opengl32.dll only exports GL 1.1).
// GL 1.1 functions are called directly; everything newer goes through a pointer loaded here.
#include <string>
#include <SDL3/SDL_opengl.h>
#ifdef _WIN32
// windows.h (pulled in by SDL_opengl.h) defines these legacy pointer qualifiers as empty macros,
// which would erase any member or variable named far / near.
#undef far
#undef near
#endif

#define SV_GL_FUNCTIONS(X)                                                              \
    X(PFNGLACTIVETEXTUREPROC, glActiveTexture)                                          \
    X(PFNGLATTACHSHADERPROC, glAttachShader)                                            \
    X(PFNGLBEGINQUERYPROC, glBeginQuery)                                                \
    X(PFNGLBINDBUFFERPROC, glBindBuffer)                                                \
    X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer)                                      \
    X(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer)                                    \
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                                      \
    X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate)                                  \
    X(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer)                                      \
    X(PFNGLBUFFERDATAPROC, glBufferData)                                                \
    X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus)                        \
    X(PFNGLCOMPILESHADERPROC, glCompileShader)                                          \
    X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                                          \
    X(PFNGLCREATESHADERPROC, glCreateShader)                                            \
    X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                                          \
    X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)                                \
    X(PFNGLDELETEPROGRAMPROC, glDeleteProgram)                                          \
    X(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers)                              \
    X(PFNGLDELETESHADERPROC, glDeleteShader)                                            \
    X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)                                \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)                      \
    X(PFNGLENDQUERYPROC, glEndQuery)                                                    \
    X(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer)                      \
    X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D)                            \
    X(PFNGLGENBUFFERSPROC, glGenBuffers)                                                \
    X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)                                      \
    X(PFNGLGENQUERIESPROC, glGenQueries)                                                \
    X(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers)                                    \
    X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                                      \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                                  \
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                                            \
    X(PFNGLGETQUERYOBJECTIVPROC, glGetQueryObjectiv)                                    \
    X(PFNGLGETQUERYOBJECTUI64VPROC, glGetQueryObjectui64v)                              \
    X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)                                    \
    X(PFNGLGETSHADERIVPROC, glGetShaderiv)                                              \
    X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)                                \
    X(PFNGLLINKPROGRAMPROC, glLinkProgram)                                              \
    X(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glRenderbufferStorageMultisample)        \
    X(PFNGLSHADERSOURCEPROC, glShaderSource)                                            \
    X(PFNGLUNIFORM1FPROC, glUniform1f)                                                  \
    X(PFNGLUNIFORM1IPROC, glUniform1i)                                                  \
    X(PFNGLUNIFORM2FPROC, glUniform2f)                                                  \
    X(PFNGLUNIFORM3FPROC, glUniform3f)                                                  \
    X(PFNGLUNIFORM4FPROC, glUniform4f)                                                  \
    X(PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv)                                    \
    X(PFNGLUSEPROGRAMPROC, glUseProgram)                                                \
    X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)

#define SV_GL_DECLARE(type, name) extern type sv_##name;
SV_GL_FUNCTIONS(SV_GL_DECLARE)
#undef SV_GL_DECLARE

// Route the standard names to the loaded pointers.
#define glActiveTexture sv_glActiveTexture
#define glAttachShader sv_glAttachShader
#define glBeginQuery sv_glBeginQuery
#define glBindBuffer sv_glBindBuffer
#define glBindFramebuffer sv_glBindFramebuffer
#define glBindRenderbuffer sv_glBindRenderbuffer
#define glBindVertexArray sv_glBindVertexArray
#define glBlendFuncSeparate sv_glBlendFuncSeparate
#define glBlitFramebuffer sv_glBlitFramebuffer
#define glBufferData sv_glBufferData
#define glCheckFramebufferStatus sv_glCheckFramebufferStatus
#define glCompileShader sv_glCompileShader
#define glCreateProgram sv_glCreateProgram
#define glCreateShader sv_glCreateShader
#define glDeleteBuffers sv_glDeleteBuffers
#define glDeleteFramebuffers sv_glDeleteFramebuffers
#define glDeleteProgram sv_glDeleteProgram
#define glDeleteRenderbuffers sv_glDeleteRenderbuffers
#define glDeleteShader sv_glDeleteShader
#define glDeleteVertexArrays sv_glDeleteVertexArrays
#define glEnableVertexAttribArray sv_glEnableVertexAttribArray
#define glEndQuery sv_glEndQuery
#define glFramebufferRenderbuffer sv_glFramebufferRenderbuffer
#define glFramebufferTexture2D sv_glFramebufferTexture2D
#define glGenBuffers sv_glGenBuffers
#define glGenFramebuffers sv_glGenFramebuffers
#define glGenQueries sv_glGenQueries
#define glGenRenderbuffers sv_glGenRenderbuffers
#define glGenVertexArrays sv_glGenVertexArrays
#define glGetProgramInfoLog sv_glGetProgramInfoLog
#define glGetProgramiv sv_glGetProgramiv
#define glGetQueryObjectiv sv_glGetQueryObjectiv
#define glGetQueryObjectui64v sv_glGetQueryObjectui64v
#define glGetShaderInfoLog sv_glGetShaderInfoLog
#define glGetShaderiv sv_glGetShaderiv
#define glGetUniformLocation sv_glGetUniformLocation
#define glLinkProgram sv_glLinkProgram
#define glRenderbufferStorageMultisample sv_glRenderbufferStorageMultisample
#define glShaderSource sv_glShaderSource
#define glUniform1f sv_glUniform1f
#define glUniform1i sv_glUniform1i
#define glUniform2f sv_glUniform2f
#define glUniform3f sv_glUniform3f
#define glUniform4f sv_glUniform4f
#define glUniformMatrix3fv sv_glUniformMatrix3fv
#define glUseProgram sv_glUseProgram
#define glVertexAttribPointer sv_glVertexAttribPointer

namespace sv {
// Loads every entry point above from the current context. Returns false (and names what is
// missing in `missing`) if the driver lacks any of them.
bool loadGL(std::string* missing = nullptr);
}
