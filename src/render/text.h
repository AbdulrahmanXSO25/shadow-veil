#pragma once
#include "core/math.h"
#include <string>
#include "render/gl.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace sv {

class Renderer;

enum class Font { Body, Bold, Italic, Kanji, Count };
enum class Align { Left, Center, Right };

// FreeType-backed text, rasterized on demand at the exact on-screen pixel size for crispness.
class TextSystem {
public:
    TextSystem();
    ~TextSystem();
    bool init();
    void clearCache();

    // Positions/sizes are in the current view's units; pxScale = screen pixels per unit.
    // Returns advance width. pos is the baseline origin.
    float draw(Renderer& r, Font f, float size, Vec2 pos, const std::string& utf8, Color col, Align align = Align::Left, float tracking = 0);
    float measure(Font f, float size, const std::string& utf8, float tracking = 0);
    std::vector<std::string> wrap(Font f, float size, const std::string& utf8, float maxWidth);
    void setPixelScale(float s) { m_pxScale = s; }
    float lineHeight(float size) const { return size * 1.35f; }

private:
    struct Glyph { float u0, v0, u1, v1; int w, h, bx, by; float adv; bool ok; };
    struct Atlas {
        GLuint tex = 0; int size = 1024; int penX = 1, penY = 1, rowH = 0;
        std::unordered_map<uint64_t, Glyph> glyphs;
        bool full = false;
    };
    const Glyph& glyph(Font f, int px, uint32_t cp);
    Atlas& atlasFor(Font f, int px);
    struct Impl;
    std::unique_ptr<Impl> m;
    std::unordered_map<uint64_t, Atlas> m_atlases;
    float m_pxScale = 1.0f;
};

std::vector<uint32_t> decodeUtf8(const std::string& s);

} // namespace sv
