#pragma once
#include "core/math.h"
#include <string>
#include "render/gl.h"
#include <vector>
#include <string>
#include <memory>

namespace sv {

class TextSystem;

enum class Blend { Alpha, Additive };

struct Vertex { float x, y, u, v, r, g, b, a; };

struct SkyParams {
    Color top{0.02f, 0.03f, 0.08f}, mid{0.08f, 0.10f, 0.22f}, horizon{0.35f, 0.30f, 0.45f};
    float horizonY = 0.72f;          // 0..1 screen height where horizon colour peaks
    Vec2 moonPos{0.72f, 0.24f};      // normalized screen coordinates
    float moonRadius = 0.09f;        // relative to screen height
    Color moon{0.95f, 0.93f, 0.85f};
    float moonVeil = 0.0f;           // smoke crossing the moon (0..1)
    float stars = 1.0f;
    Color cloud{0.10f, 0.10f, 0.18f, 0.6f};
    float time = 0;
    float scroll = 0;                // camera x drives slow cloud parallax
};

struct PostParams {
    Color lift{0, 0, 0}, gamma{1, 1, 1}, gain{1, 1, 1};
    float saturation = 1.0f;
    float bloom = 0.6f;
    float vignette = 0.35f;
    float grain = 0.035f;
    float chroma = 0.0f;             // chromatic aberration amount (impact punctuation)
    float flash = 0.0f;              // white flash 0..1
    Color flashColor{1, 1, 1};
    float fade = 0.0f;               // fade to black 0..1
    float letterbox = 0.0f;          // 0..1 bar coverage animation
    float desaturate = 0.0f;         // execution/stillness moments
    Vec2 rippleCenter{0.5f, 0.5f};   // veil ripple (screen uv)
    float ripple = 0.0f;
    float time = 0;
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    bool init(int width, int height);
    void shutdown();
    void resize(int width, int height);
    // Render at a fixed internal resolution and scale to the window (captures, render scale).
    void setFixedResolution(int width, int height);
    int width() const { return m_w; }
    int height() const { return m_h; }

    // UI space: height is always 1080 units; width follows the aspect ratio.
    float uiWidth() const { return 1080.f * m_w / (float)m_h; }
    float uiHeight() const { return 1080.f; }
    float uiScale() const { return m_h / 1080.f; }

    void beginScene();
    void drawSky(const SkyParams& p);
    // World camera. zoom=1 shows 1080 world units vertically. parallax scales camera movement.
    void setWorldView(Vec2 center, float zoom, float parallax = 1.0f, float rotation = 0.0f);
    Vec2 viewSize() const { return m_viewSize; }
    void setUIView();
    void endScene(const PostParams& p);   // post-processes into the default framebuffer
    void endFrame();                      // flush UI

    void setBlend(Blend b);
    void setTexture(GLuint tex, bool alphaOnly);
    void useWhite();
    void flush();

    // Primitives (current view & blend).
    void tri(Vec2 a, Vec2 b, Vec2 c, Color col);
    void tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc);
    void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color col);
    void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color ca, Color cb, Color cc, Color cd);
    void quadUV(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 uva, Vec2 uvb, Vec2 uvc, Vec2 uvd, Color col);
    void rect(const Rect& r, Color c);
    void rectGrad(const Rect& r, Color top, Color bottom);
    void rectGradH(const Rect& r, Color left, Color right);
    void circle(Vec2 c, float r, Color col, int segments = 0);
    void ring(Vec2 c, float r0, float r1, Color col, float a0 = 0, float a1 = TAU, int segments = 0);
    void line(Vec2 a, Vec2 b, float width, Color col);
    void lineGrad(Vec2 a, Vec2 b, float wa, float wb, Color ca, Color cb);
    // Tapered limb with round caps.
    void capsule(Vec2 a, Vec2 b, float ra, float rb, Color col);
    void polygon(const Vec2* pts, int n, Color col);  // convex
    void glow(Vec2 c, float radius, Color col);       // additive-friendly radial falloff
    void textured(GLuint tex, bool alphaOnly, const Rect& dst, const Rect& uv, Color col);

    GLuint glowTex() const { return m_glowTex; }
    GLuint noiseTex() const { return m_noiseTex; }
    GLuint brushTex() const { return m_brushTex; }
    TextSystem& text() { return *m_text; }

    bool saveScreenshot(const std::string& path);
    // Raw RGBA of the final image, bottom row first (for video capture).
    void readPixels(std::vector<uint8_t>& out);
    int drawCalls() const { return m_drawCallsLast; }

private:
    void push(const Vertex& v);
    GLuint compile(const char* vs, const char* fs, const char* name);
    void createTargets();
    void destroyTargets();
    void setMatrix(const float* m);

    int m_w = 0, m_h = 0;          // internal render size
    int m_winW = 0, m_winH = 0;    // window framebuffer size
    bool m_fixed = false;
    GLuint m_finalFbo = 0, m_finalTex = 0;
    void bindOutput();
    GLuint m_vao = 0, m_vbo = 0;
    GLuint m_progBasic = 0, m_progSky = 0, m_progBright = 0, m_progDown = 0, m_progUp = 0, m_progComposite = 0;
    GLuint m_fullscreenVao = 0;
    GLuint m_whiteTex = 0, m_glowTex = 0, m_noiseTex = 0, m_brushTex = 0;
    GLuint m_msaaFbo = 0, m_msaaColor = 0;
    GLuint m_sceneFbo = 0, m_sceneTex = 0;
    static constexpr int kBloomLevels = 5;
    GLuint m_bloomFbo[kBloomLevels]{}, m_bloomTex[kBloomLevels]{};
    int m_bloomW[kBloomLevels]{}, m_bloomH[kBloomLevels]{};
    std::vector<Vertex> m_verts;
    GLuint m_curTex = 0;
    bool m_curAlphaOnly = false;
    Blend m_blend = Blend::Alpha;
    float m_matrix[9]{};
    Vec2 m_viewSize{1920, 1080};
    float m_pixelWorld = 1.0f;  // world units per pixel, for circle tessellation
    int m_drawCalls = 0, m_drawCallsLast = 0;
    int m_samples = 4;
    std::unique_ptr<TextSystem> m_text;
};

} // namespace sv
