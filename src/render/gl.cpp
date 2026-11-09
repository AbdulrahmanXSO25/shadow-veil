#include <string>
#include "render/gl.h"
#include <SDL3/SDL_video.h>

#define SV_GL_DEFINE(type, name) type sv_##name = nullptr;
SV_GL_FUNCTIONS(SV_GL_DEFINE)
#undef SV_GL_DEFINE

namespace sv {

bool loadGL(std::string* missing) {
    bool ok = true;
#define SV_GL_LOAD(type, name)                                                     \
    sv_##name = reinterpret_cast<type>(SDL_GL_GetProcAddress(#name));            \
    if (!sv_##name) { ok = false; if (missing) { *missing += missing->empty() ? "" : ", "; *missing += #name; } }
    SV_GL_FUNCTIONS(SV_GL_LOAD)
#undef SV_GL_LOAD
    return ok;
}

}  // namespace sv
