#include "render/renderer.h"
#include "render/text.h"
#include "render/png.h"
#include "core/util.h"
#include <cstring>

namespace sv {

// ---------------------------------------------------------------- shaders

static const char* kBasicVS = R"(#version 330 core
layout(location=0) in vec2 a_pos; layout(location=1) in vec2 a_uv; layout(location=2) in vec4 a_col;
uniform mat3 u_mvp; out vec2 v_uv; out vec4 v_col;
void main(){ vec3 p = u_mvp * vec3(a_pos,1.0); gl_Position = vec4(p.xy,0.0,1.0); v_uv=a_uv; v_col=a_col; })";

static const char* kBasicFS = R"(#version 330 core
in vec2 v_uv; in vec4 v_col; uniform sampler2D u_tex; uniform int u_alphaOnly; out vec4 o;
void main(){ vec4 t = texture(u_tex, v_uv); if (u_alphaOnly==1) t = vec4(1.0,1.0,1.0,t.r); o = t * v_col; })";

static const char* kFullscreenVS = R"(#version 330 core
out vec2 v_uv;
void main(){ vec2 p = vec2((gl_VertexID<<1)&2, gl_VertexID&2); v_uv = p; gl_Position = vec4(p*2.0-1.0,0.0,1.0); })";

static const char* kSkyFS = R"(#version 330 core
in vec2 v_uv; out vec4 o;
uniform vec3 u_top, u_mid, u_horizon; uniform float u_horizonY;
uniform vec2 u_moonPos; uniform float u_moonR; uniform vec3 u_moon; uniform float u_moonVeil;
uniform float u_stars; uniform vec4 u_cloud; uniform float u_time, u_scroll, u_aspect;
float h21(vec2 p){ p = fract(p*vec2(123.34,456.21)); p += dot(p,p+45.32); return fract(p.x*p.y); }
float vnoise(vec2 p){ vec2 i=floor(p), f=fract(p); vec2 u=f*f*(3.0-2.0*f);
  return mix(mix(h21(i),h21(i+vec2(1,0)),u.x), mix(h21(i+vec2(0,1)),h21(i+vec2(1,1)),u.x), u.y); }
float fbm(vec2 p){ float v=0.0,a=0.5; for(int i=0;i<4;i++){ v+=a*vnoise(p); p=p*2.03+vec2(1.7,9.2); a*=0.5;} return v/0.9375; }
void main(){
  vec2 uv = vec2(v_uv.x, 1.0 - v_uv.y);           // y down like the world
  float y = uv.y;
  vec3 col = mix(u_top, u_mid, smoothstep(0.0, u_horizonY*0.75, y));
  col = mix(col, u_horizon, smoothstep(u_horizonY*0.45, u_horizonY, y));
  col = mix(col, u_horizon*0.6, smoothstep(u_horizonY, 1.0, y));
  vec2 sp = vec2(uv.x*u_aspect, uv.y);
  // stars: sparse, twinkling, fading toward the horizon glow
  vec2 g = floor(sp*180.0); float s = h21(g);
  if (s > 0.9965) { vec2 f = fract(sp*180.0)-0.5; float tw = 0.6+0.4*sin(u_time*(1.0+s*5.0)+s*90.0);
    col += vec3(0.9,0.92,1.0) * smoothstep(0.18,0.0,length(f)) * tw * u_stars * (1.0-smoothstep(0.2,u_horizonY,y)); }
  // moon with halo
  vec2 mp = vec2(u_moonPos.x*u_aspect, u_moonPos.y);
  float d = length(sp - mp);
  float disc = smoothstep(u_moonR, u_moonR-0.002, d);
  float halo = exp(-max(d-u_moonR,0.0)*9.0)*0.35 + exp(-max(d-u_moonR,0.0)*38.0)*0.35;
  col += u_moon * halo * 0.55;
  if (disc > 0.0) {
    vec2 mq = (sp-mp)/u_moonR;
    float mare = fbm(mq*2.2 + 3.0);
    float limb = sqrt(max(0.0, 1.0 - dot(mq,mq)));
    vec3 moonCol = u_moon * (0.74 + 0.16*smoothstep(0.3,0.75,mare)) * (0.8 + 0.2*limb);
    col = mix(col, moonCol, disc);
  }
  // drifting cloud / smoke bands
  float band = smoothstep(0.15, 0.95, y);
  float cloud = 0.0;
  if (band > 0.0 && u_cloud.a > 0.0) {
    float c = fbm(vec2(sp.x*2.2 + u_time*0.012 + u_scroll*0.00006, y*7.0));
    float c2 = vnoise(vec2(sp.x*9.0 - u_time*0.04 + u_scroll*0.0002, y*24.0 + 4.0));
    cloud = smoothstep(0.48, 0.8, c*0.75 + c2*0.35) * band * u_cloud.a;
  }
  float veil = 0.0;
  if (u_moonVeil > 0.0 && d < u_moonR*2.5) veil = smoothstep(0.4,0.75,fbm(vec2(sp.x*3.0 + u_time*0.03, y*5.0)))*u_moonVeil*smoothstep(u_moonR*2.5,0.0,d);
  col = mix(col, u_cloud.rgb, clamp(cloud + veil,0.0,1.0));
  // silver lining near the moon
  col += u_moon * cloud * exp(-d*6.0) * 0.25;
  o = vec4(col, 1.0);
})";

static const char* kBrightFS = R"(#version 330 core
in vec2 v_uv; out vec4 o; uniform sampler2D u_tex; uniform vec2 u_texel;
void main(){
  vec3 c = vec3(0.0);
  c += texture(u_tex, v_uv + u_texel*vec2(-1,-1)).rgb; c += texture(u_tex, v_uv + u_texel*vec2(1,-1)).rgb;
  c += texture(u_tex, v_uv + u_texel*vec2(-1, 1)).rgb; c += texture(u_tex, v_uv + u_texel*vec2(1, 1)).rgb;
  c *= 0.25;
  float br = max(c.r, max(c.g, c.b));
  float knee = 0.25, thr = 0.62;
  float soft = clamp(br - thr + knee, 0.0, 2.0*knee); soft = soft*soft/(4.0*knee+1e-4);
  float w = max(soft, br - thr) / max(br, 1e-4);
  o = vec4(c * w, 1.0);
})";

static const char* kDownFS = R"(#version 330 core
in vec2 v_uv; out vec4 o; uniform sampler2D u_tex; uniform vec2 u_texel;
void main(){
  vec3 c = texture(u_tex, v_uv).rgb * 4.0;
  c += texture(u_tex, v_uv + u_texel*vec2(-1,-1)).rgb; c += texture(u_tex, v_uv + u_texel*vec2(1,-1)).rgb;
  c += texture(u_tex, v_uv + u_texel*vec2(-1, 1)).rgb; c += texture(u_tex, v_uv + u_texel*vec2(1, 1)).rgb;
  o = vec4(c/8.0, 1.0);
})";

static const char* kUpFS = R"(#version 330 core
in vec2 v_uv; out vec4 o; uniform sampler2D u_tex; uniform vec2 u_texel;
void main(){
  vec3 c = vec3(0.0);
  c += texture(u_tex, v_uv + u_texel*vec2(-2,0)).rgb; c += texture(u_tex, v_uv + u_texel*vec2(2,0)).rgb;
  c += texture(u_tex, v_uv + u_texel*vec2(0,-2)).rgb; c += texture(u_tex, v_uv + u_texel*vec2(0,2)).rgb;
  c += texture(u_tex, v_uv + u_texel*vec2(-1,-1)).rgb*2.0; c += texture(u_tex, v_uv + u_texel*vec2(1,-1)).rgb*2.0;
  c += texture(u_tex, v_uv + u_texel*vec2(-1,1)).rgb*2.0; c += texture(u_tex, v_uv + u_texel*vec2(1,1)).rgb*2.0;
  o = vec4(c/12.0, 1.0);
})";

static const char* kCompositeFS = R"(#version 330 core
in vec2 v_uv; out vec4 o;
uniform sampler2D u_scene, u_bloom;
uniform vec3 u_lift, u_gamma, u_gain; uniform float u_sat, u_bloomAmt, u_vignette, u_grain, u_chroma;
uniform float u_flash; uniform vec3 u_flashCol; uniform float u_fade, u_letterbox, u_desat, u_time, u_aspect;
uniform vec2 u_rippleC; uniform float u_ripple;
float h(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233)))*43758.5453); }
vec3 softClip(vec3 c){ vec3 k = vec3(0.78); vec3 over = max(c-k, 0.0); return min(c,k) + (1.0-k)*(1.0-exp(-over/(1.0-k))); }
void main(){
  vec2 uv = v_uv;
  if (u_ripple > 0.0) {
    vec2 d = uv - u_rippleC; d.x *= u_aspect; float r = length(d);
    float w = sin(r*55.0 - u_time*18.0) * exp(-r*5.0) * u_ripple * 0.012;
    uv += (r > 1e-4 ? d/r : vec2(0.0)) * w * vec2(1.0/u_aspect, 1.0);
  }
  vec2 off = (uv - 0.5) * u_chroma * 0.012;
  vec3 c;
  c.r = texture(u_scene, uv + off).r; c.g = texture(u_scene, uv).g; c.b = texture(u_scene, uv - off).b;
  c += texture(u_bloom, uv).rgb * u_bloomAmt;
  c = softClip(c);
  c = c * u_gain + u_lift * (1.0 - c);
  c = pow(max(c, 0.0), 1.0 / u_gamma);
  float l = dot(c, vec3(0.299, 0.587, 0.114));
  c = mix(vec3(l), c, u_sat * (1.0 - u_desat));
  vec2 vd = (v_uv - 0.5) * vec2(u_aspect, 1.0);
  c *= 1.0 - u_vignette * smoothstep(0.35, 1.05, length(vd));
  c += (h(v_uv * 913.0 + fract(u_time * 7.13)) - 0.5) * u_grain;
  c = mix(c, u_flashCol, u_flash);
  c *= 1.0 - u_fade;
  float bar = u_letterbox * 0.115;
  if (v_uv.y < bar || v_uv.y > 1.0 - bar) c = vec3(0.0);
  o = vec4(c, 1.0);
})";

// ---------------------------------------------------------------- setup

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

GLuint Renderer::compile(const char* vs, const char* fs, const char* name) {
    auto sh = [&](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[2048]; glGetShaderInfoLog(s, sizeof log, nullptr, log); logError("shader %s: %s", name, log); }
        return s;
    };
    GLuint v = sh(GL_VERTEX_SHADER, vs), f = sh(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[2048]; glGetProgramInfoLog(p, sizeof log, nullptr, log); logError("link %s: %s", name, log); }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static GLuint makeTex(int w, int h, GLenum ifmt, GLenum fmt, GLenum type, const void* data, bool linear, bool repeat) {
    GLuint t; glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    return t;
}

// Tileable value-noise fbm, used for fog, mist and paper textures.
static std::vector<uint8_t> genNoise(int n) {
    std::vector<uint8_t> px(n * n);
    auto lattice = [](int x, int y, int period, int seed) { return hashf(((x % period) + period) % period + seed * 1000, ((y % period) + period) % period); };
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float v = 0, amp = 0.5f, tot = 0;
            int period = 8;
            for (int o = 0; o < 5; o++) {
                float fx = x * period / (float)n, fy = y * period / (float)n;
                int ix = (int)fx, iy = (int)fy; float tx = fx - ix, ty = fy - iy;
                tx = tx * tx * (3 - 2 * tx); ty = ty * ty * (3 - 2 * ty);
                float a = lattice(ix, iy, period, o), b = lattice(ix + 1, iy, period, o);
                float c = lattice(ix, iy + 1, period, o), d = lattice(ix + 1, iy + 1, period, o);
                v += lerp(lerp(a, b, tx), lerp(c, d, tx), ty) * amp;
                tot += amp; amp *= 0.5f; period *= 2;
            }
            px[y * n + x] = (uint8_t)clampf(v / tot * 255.f, 0, 255);
        }
    return px;
}

// A horizontal sumi-e brush stroke: dense core, dry-brush streaks, ragged ends.
static std::vector<uint8_t> genBrush(int w, int h) {
    std::vector<uint8_t> px(w * h);
    for (int y = 0; y < h; y++) {
        float fy = (y + 0.5f) / h * 2 - 1;
        float streak = 0.75f + 0.25f * fbm1(y * 0.9f, 7, 3);
        for (int x = 0; x < w; x++) {
            float fx = (x + 0.5f) / w;
            float edgeL = 0.03f + 0.05f * fbm1(y * 0.21f, 3);
            float edgeR = 0.93f - 0.08f * fbm1(y * 0.17f, 5);
            float thick = 0.82f - 0.18f * std::pow(std::abs(fx - 0.45f) * 2, 2.f) + 0.08f * (fbm1(fx * 9, 11) - .5f);
            float v = smoothstep(thick, thick - 0.08f, std::abs(fy));
            v *= smoothstep(edgeL, edgeL + 0.04f, fx) * smoothstep(edgeR, edgeR - 0.12f, fx);
            float dry = fbm1(fx * 40 + y * 3.1f, 13, 2);
            if (fx > 0.6f) v *= lerp(1.f, smoothstep(0.25f, 0.6f, dry * streak), (fx - 0.6f) / 0.4f);
            v *= 0.85f + 0.15f * streak;
            px[y * w + x] = (uint8_t)clampf(v * 255.f, 0, 255);
        }
    }
    return px;
}

bool Renderer::init(int width, int height) {
    m_w = width; m_h = height;
    m_winW = width; m_winH = height;
    std::string missing;
    if (!loadGL(&missing)) { logError("OpenGL 3.3 is required; the driver is missing: %s", missing.c_str()); return false; }
    m_progBasic = compile(kBasicVS, kBasicFS, "basic");
    m_progSky = compile(kFullscreenVS, kSkyFS, "sky");
    m_progBright = compile(kFullscreenVS, kBrightFS, "bright");
    m_progDown = compile(kFullscreenVS, kDownFS, "down");
    m_progUp = compile(kFullscreenVS, kUpFS, "up");
    m_progComposite = compile(kFullscreenVS, kCompositeFS, "composite");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)8);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16);
    glGenVertexArrays(1, &m_fullscreenVao);

    uint32_t white = 0xffffffff;
    m_whiteTex = makeTex(1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, &white, false, false);
    {
        const int n = 128; std::vector<uint8_t> px(n * n);
        for (int y = 0; y < n; y++) for (int x = 0; x < n; x++) {
            float dx = (x + .5f) / n * 2 - 1, dy = (y + .5f) / n * 2 - 1;
            float r = std::sqrt(dx * dx + dy * dy);
            float v = std::pow(clamp01(1 - r), 2.2f);
            px[y * n + x] = (uint8_t)(v * 255);
        }
        m_glowTex = makeTex(n, n, GL_R8, GL_RED, GL_UNSIGNED_BYTE, px.data(), true, false);
    }
    { auto px = genNoise(256); m_noiseTex = makeTex(256, 256, GL_R8, GL_RED, GL_UNSIGNED_BYTE, px.data(), true, true); }
    { auto px = genBrush(512, 96); m_brushTex = makeTex(512, 96, GL_R8, GL_RED, GL_UNSIGNED_BYTE, px.data(), true, false); }

    GLint maxSamples = 4; glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    m_samples = std::min(4, (int)maxSamples);
    createTargets();

    m_text = std::make_unique<TextSystem>();
    if (!m_text->init()) return false;
    m_verts.reserve(65536);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    return true;
}

void Renderer::createTargets() {
    glGenFramebuffers(1, &m_msaaFbo);
    glGenRenderbuffers(1, &m_msaaColor);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_R11F_G11F_B10F, m_w, m_h);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msaaColor);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) logError("MSAA framebuffer incomplete");

    m_sceneTex = makeTex(m_w, m_h, GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT, nullptr, true, false);
    glGenFramebuffers(1, &m_sceneFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sceneTex, 0);

    if (m_fixed) {
        m_finalTex = makeTex(m_w, m_h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, nullptr, true, false);
        glGenFramebuffers(1, &m_finalFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_finalFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_finalTex, 0);
    }
    int bw = std::max(1, m_w / 2), bh = std::max(1, m_h / 2);
    for (int i = 0; i < kBloomLevels; i++) {
        m_bloomW[i] = bw; m_bloomH[i] = bh;
        m_bloomTex[i] = makeTex(bw, bh, GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT, nullptr, true, false);
        glGenFramebuffers(1, &m_bloomFbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomTex[i], 0);
        bw = std::max(1, bw / 2); bh = std::max(1, bh / 2);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::destroyTargets() {
    glDeleteFramebuffers(1, &m_msaaFbo); glDeleteRenderbuffers(1, &m_msaaColor);
    glDeleteFramebuffers(1, &m_sceneFbo); glDeleteTextures(1, &m_sceneTex);
    glDeleteFramebuffers(kBloomLevels, m_bloomFbo); glDeleteTextures(kBloomLevels, m_bloomTex);
    if (m_finalFbo) { glDeleteFramebuffers(1, &m_finalFbo); glDeleteTextures(1, &m_finalTex); m_finalFbo = m_finalTex = 0; }
}

void Renderer::setFixedResolution(int width, int height) {
    if (width <= 0 || height <= 0) return;
    destroyTargets();
    m_fixed = true;
    m_w = width; m_h = height;
    createTargets();
    m_text->clearCache();
}

void Renderer::bindOutput() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fixed ? m_finalFbo : 0);
    glViewport(0, 0, m_w, m_h);
}

void Renderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    m_winW = width; m_winH = height;
    if (m_fixed) return;
    if (width == m_w && height == m_h) return;
    m_w = width; m_h = height;
    destroyTargets();
    createTargets();
    m_text->clearCache();
}

void Renderer::shutdown() {
    if (!m_progBasic) return;
    destroyTargets();
    m_text.reset();
    GLuint progs[] = {m_progBasic, m_progSky, m_progBright, m_progDown, m_progUp, m_progComposite};
    for (GLuint p : progs) glDeleteProgram(p);
    GLuint texs[] = {m_whiteTex, m_glowTex, m_noiseTex, m_brushTex};
    glDeleteTextures(4, texs);
    glDeleteBuffers(1, &m_vbo); glDeleteVertexArrays(1, &m_vao); glDeleteVertexArrays(1, &m_fullscreenVao);
    m_progBasic = 0;
}

// ---------------------------------------------------------------- frame

void Renderer::beginScene() {
    m_drawCallsLast = m_drawCalls; m_drawCalls = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
    glViewport(0, 0, m_w, m_h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    m_blend = Blend::Alpha;
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    useWhite();
}

void Renderer::drawSky(const SkyParams& p) {
    flush();
    glUseProgram(m_progSky);
    auto u = [&](const char* n) { return glGetUniformLocation(m_progSky, n); };
    glUniform3f(u("u_top"), p.top.r, p.top.g, p.top.b);
    glUniform3f(u("u_mid"), p.mid.r, p.mid.g, p.mid.b);
    glUniform3f(u("u_horizon"), p.horizon.r, p.horizon.g, p.horizon.b);
    glUniform1f(u("u_horizonY"), p.horizonY);
    glUniform2f(u("u_moonPos"), p.moonPos.x, p.moonPos.y);
    glUniform1f(u("u_moonR"), p.moonRadius);
    glUniform3f(u("u_moon"), p.moon.r, p.moon.g, p.moon.b);
    glUniform1f(u("u_moonVeil"), p.moonVeil);
    glUniform1f(u("u_stars"), p.stars);
    glUniform4f(u("u_cloud"), p.cloud.r, p.cloud.g, p.cloud.b, p.cloud.a);
    glUniform1f(u("u_time"), p.time);
    glUniform1f(u("u_scroll"), p.scroll);
    glUniform1f(u("u_aspect"), m_w / (float)m_h);
    glDisable(GL_BLEND);
    glBindVertexArray(m_fullscreenVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glEnable(GL_BLEND);
    m_drawCalls++;
}

void Renderer::setMatrix(const float* m) {
    flush();
    std::memcpy(m_matrix, m, sizeof m_matrix);
}

void Renderer::setWorldView(Vec2 center, float zoom, float parallax, float rotation) {
    float vh = 1080.f / zoom, vw = vh * m_w / (float)m_h;
    m_viewSize = {vw, vh};
    m_pixelWorld = vh / m_h;
    Vec2 c = center * parallax;
    float sx = 2.f / vw, sy = -2.f / vh;
    float cs = std::cos(rotation), sn = std::sin(rotation);
    // ndc = S * R * (p - c)
    float m[9] = {
        sx * cs, sy * sn, 0,
        -sx * sn, sy * cs, 0,
        0, 0, 1};
    m[6] = -(m[0] * c.x + m[3] * c.y);
    m[7] = -(m[1] * c.x + m[4] * c.y);
    setMatrix(m);
}

void Renderer::setUIView() {
    float vw = uiWidth(), vh = 1080.f;
    m_viewSize = {vw, vh};
    m_pixelWorld = vh / m_h;
    float m[9] = {2.f / vw, 0, 0, 0, -2.f / vh, 0, -1, 1, 1};
    setMatrix(m);
}

void Renderer::endScene(const PostParams& p) {
    flush();
    glDisable(GL_BLEND);
    // Resolve MSAA
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_sceneFbo);
    glBlitFramebuffer(0, 0, m_w, m_h, 0, 0, m_w, m_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindVertexArray(m_fullscreenVao);
    glActiveTexture(GL_TEXTURE0);
    // Bright pass
    glUseProgram(m_progBright);
    glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[0]);
    glViewport(0, 0, m_bloomW[0], m_bloomH[0]);
    glBindTexture(GL_TEXTURE_2D, m_sceneTex);
    glUniform1i(glGetUniformLocation(m_progBright, "u_tex"), 0);
    glUniform2f(glGetUniformLocation(m_progBright, "u_texel"), 1.f / m_w, 1.f / m_h);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // Downsample chain
    glUseProgram(m_progDown);
    for (int i = 1; i < kBloomLevels; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[i]);
        glViewport(0, 0, m_bloomW[i], m_bloomH[i]);
        glBindTexture(GL_TEXTURE_2D, m_bloomTex[i - 1]);
        glUniform1i(glGetUniformLocation(m_progDown, "u_tex"), 0);
        glUniform2f(glGetUniformLocation(m_progDown, "u_texel"), 1.f / m_bloomW[i - 1], 1.f / m_bloomH[i - 1]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    // Upsample, additively accumulating
    glUseProgram(m_progUp);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (int i = kBloomLevels - 2; i >= 0; i--) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[i]);
        glViewport(0, 0, m_bloomW[i], m_bloomH[i]);
        glBindTexture(GL_TEXTURE_2D, m_bloomTex[i + 1]);
        glUniform1i(glGetUniformLocation(m_progUp, "u_tex"), 0);
        glUniform2f(glGetUniformLocation(m_progUp, "u_texel"), 1.f / m_bloomW[i + 1], 1.f / m_bloomH[i + 1]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glDisable(GL_BLEND);
    // Composite to the output (backbuffer, or the fixed-resolution target)
    bindOutput();
    glUseProgram(m_progComposite);
    auto u = [&](const char* n) { return glGetUniformLocation(m_progComposite, n); };
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_sceneTex);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_bloomTex[0]);
    glUniform1i(u("u_scene"), 0); glUniform1i(u("u_bloom"), 1);
    glUniform3f(u("u_lift"), p.lift.r, p.lift.g, p.lift.b);
    glUniform3f(u("u_gamma"), p.gamma.r, p.gamma.g, p.gamma.b);
    glUniform3f(u("u_gain"), p.gain.r, p.gain.g, p.gain.b);
    glUniform1f(u("u_sat"), p.saturation);
    glUniform1f(u("u_bloomAmt"), p.bloom);
    glUniform1f(u("u_vignette"), p.vignette);
    glUniform1f(u("u_grain"), p.grain);
    glUniform1f(u("u_chroma"), p.chroma);
    glUniform1f(u("u_flash"), p.flash);
    glUniform3f(u("u_flashCol"), p.flashColor.r, p.flashColor.g, p.flashColor.b);
    glUniform1f(u("u_fade"), p.fade);
    glUniform1f(u("u_letterbox"), p.letterbox);
    glUniform1f(u("u_desat"), p.desaturate);
    glUniform1f(u("u_time"), p.time);
    glUniform1f(u("u_aspect"), m_w / (float)m_h);
    glUniform2f(u("u_rippleC"), p.rippleCenter.x, p.rippleCenter.y);
    glUniform1f(u("u_ripple"), p.ripple);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glActiveTexture(GL_TEXTURE0);
    m_drawCalls += 3 + kBloomLevels * 2;

    // Prepare for UI on the backbuffer
    glEnable(GL_BLEND);
    m_blend = Blend::Alpha;
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    setUIView();
    useWhite();
}

void Renderer::endFrame() {
    flush();
    if (!m_fixed) return;
    // letterbox-fit the fixed-resolution image into the window
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_finalFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, m_winW, m_winH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    float s = std::min(m_winW / (float)m_w, m_winH / (float)m_h);
    int dw = (int)(m_w * s), dh = (int)(m_h * s), dx = (m_winW - dw) / 2, dy = (m_winH - dh) / 2;
    glBlitFramebuffer(0, 0, m_w, m_h, dx, dy, dx + dw, dy + dh, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void Renderer::setBlend(Blend b) {
    if (b == m_blend) return;
    flush();
    m_blend = b;
    if (b == Blend::Alpha) glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    else glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
}

void Renderer::setTexture(GLuint tex, bool alphaOnly) {
    if (tex == m_curTex && alphaOnly == m_curAlphaOnly) return;
    flush();
    m_curTex = tex; m_curAlphaOnly = alphaOnly;
}
void Renderer::useWhite() { setTexture(m_whiteTex, false); }

void Renderer::flush() {
    if (m_verts.empty()) return;
    glUseProgram(m_progBasic);
    glUniformMatrix3fv(glGetUniformLocation(m_progBasic, "u_mvp"), 1, GL_FALSE, m_matrix);
    glUniform1i(glGetUniformLocation(m_progBasic, "u_tex"), 0);
    glUniform1i(glGetUniformLocation(m_progBasic, "u_alphaOnly"), m_curAlphaOnly ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_curTex);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_verts.size() * sizeof(Vertex), m_verts.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_verts.size());
    m_verts.clear();
    m_drawCalls++;
}

// ---------------------------------------------------------------- primitives

inline void Renderer::push(const Vertex& v) { m_verts.push_back(v); }

void Renderer::tri(Vec2 a, Vec2 b, Vec2 c, Color col) { tri(a, b, c, col, col, col); }
void Renderer::tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc) {
    push({a.x, a.y, .5f, .5f, ca.r, ca.g, ca.b, ca.a});
    push({b.x, b.y, .5f, .5f, cb.r, cb.g, cb.b, cb.a});
    push({c.x, c.y, .5f, .5f, cc.r, cc.g, cc.b, cc.a});
}
void Renderer::quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color col) { quad(a, b, c, d, col, col, col, col); }
void Renderer::quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color ca, Color cb, Color cc, Color cd) {
    tri(a, b, c, ca, cb, cc);
    tri(a, c, d, ca, cc, cd);
}
void Renderer::quadUV(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 ua, Vec2 ub, Vec2 uc, Vec2 ud, Color k) {
    push({a.x, a.y, ua.x, ua.y, k.r, k.g, k.b, k.a});
    push({b.x, b.y, ub.x, ub.y, k.r, k.g, k.b, k.a});
    push({c.x, c.y, uc.x, uc.y, k.r, k.g, k.b, k.a});
    push({a.x, a.y, ua.x, ua.y, k.r, k.g, k.b, k.a});
    push({c.x, c.y, uc.x, uc.y, k.r, k.g, k.b, k.a});
    push({d.x, d.y, ud.x, ud.y, k.r, k.g, k.b, k.a});
}
void Renderer::rect(const Rect& r, Color c) {
    quad({r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}, c);
}
void Renderer::rectGrad(const Rect& r, Color top, Color bottom) {
    quad({r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}, top, top, bottom, bottom);
}
void Renderer::rectGradH(const Rect& r, Color left, Color right) {
    quad({r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}, left, right, right, left);
}
void Renderer::circle(Vec2 c, float r, Color col, int seg) {
    if (seg <= 0) seg = (int)clampf(6 + std::sqrt(r / m_pixelWorld) * 3.f, 8, 72);
    Vec2 prev = {c.x + r, c.y};
    for (int i = 1; i <= seg; i++) {
        float a = TAU * i / seg;
        Vec2 p = {c.x + std::cos(a) * r, c.y + std::sin(a) * r};
        tri(c, prev, p, col);
        prev = p;
    }
}
void Renderer::ring(Vec2 c, float r0, float r1, Color col, float a0, float a1, int seg) {
    if (seg <= 0) seg = (int)clampf((6 + std::sqrt(r1 / m_pixelWorld) * 3.f) * (a1 - a0) / TAU, 4, 96);
    for (int i = 0; i < seg; i++) {
        float t0 = a0 + (a1 - a0) * i / seg, t1 = a0 + (a1 - a0) * (i + 1) / seg;
        Vec2 d0{std::cos(t0), std::sin(t0)}, d1{std::cos(t1), std::sin(t1)};
        quad(c + d0 * r0, c + d0 * r1, c + d1 * r1, c + d1 * r0, col);
    }
}
void Renderer::line(Vec2 a, Vec2 b, float w, Color col) { lineGrad(a, b, w, w, col, col); }
void Renderer::lineGrad(Vec2 a, Vec2 b, float wa, float wb, Color ca, Color cb) {
    Vec2 n = (b - a).norm().perp();
    quad(a + n * (wa * .5f), b + n * (wb * .5f), b - n * (wb * .5f), a - n * (wa * .5f), ca, cb, cb, ca);
}
void Renderer::capsule(Vec2 a, Vec2 b, float ra, float rb, Color col) {
    Vec2 d = b - a; float L = d.len();
    if (L < 1e-3f) { circle(a, std::max(ra, rb), col); return; }
    Vec2 n = d / L, p = n.perp();
    quad(a + p * ra, b + p * rb, b - p * rb, a - p * ra, col);
    circle(a, ra, col);
    circle(b, rb, col);
}
void Renderer::polygon(const Vec2* pts, int n, Color col) {
    for (int i = 1; i + 1 < n; i++) tri(pts[0], pts[i], pts[i + 1], col);
}
void Renderer::glow(Vec2 c, float radius, Color col) {
    GLuint prevTex = m_curTex; bool prevA = m_curAlphaOnly;
    setTexture(m_glowTex, true);
    quadUV({c.x - radius, c.y - radius}, {c.x + radius, c.y - radius}, {c.x + radius, c.y + radius}, {c.x - radius, c.y + radius},
           {0, 0}, {1, 0}, {1, 1}, {0, 1}, col);
    setTexture(prevTex, prevA);
}
void Renderer::textured(GLuint tex, bool alphaOnly, const Rect& d, const Rect& uv, Color col) {
    GLuint prevTex = m_curTex; bool prevA = m_curAlphaOnly;
    setTexture(tex, alphaOnly);
    quadUV({d.x, d.y}, {d.x + d.w, d.y}, {d.x + d.w, d.y + d.h}, {d.x, d.y + d.h},
           {uv.x, uv.y}, {uv.x + uv.w, uv.y}, {uv.x + uv.w, uv.y + uv.h}, {uv.x, uv.y + uv.h}, col);
    setTexture(prevTex, prevA);
}

void Renderer::readPixels(std::vector<uint8_t>& out) {
    flush();
    out.resize((size_t)m_w * m_h * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fixed ? m_finalFbo : 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_w, m_h, GL_RGBA, GL_UNSIGNED_BYTE, out.data());
}

bool Renderer::saveScreenshot(const std::string& path) {
    flush();
    std::vector<uint8_t> px(m_w * m_h * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fixed ? m_finalFbo : 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_w, m_h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    std::vector<uint8_t> flipped(px.size());
    for (int y = 0; y < m_h; y++)
        std::memcpy(&flipped[y * m_w * 4], &px[(m_h - 1 - y) * m_w * 4], m_w * 4);
    for (size_t i = 3; i < flipped.size(); i += 4) flipped[i] = 255;
    return writePNG(path, m_w, m_h, flipped.data());
}

} // namespace sv
