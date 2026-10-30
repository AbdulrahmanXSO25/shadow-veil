#include "render/text.h"
#include "render/renderer.h"
#include "core/util.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace sv {

std::vector<uint32_t> decodeUtf8(const std::string& s) {
    std::vector<uint32_t> out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        uint32_t cp; int n;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c >> 5) == 6) { cp = c & 0x1f; n = 2; }
        else if ((c >> 4) == 14) { cp = c & 0x0f; n = 3; }
        else if ((c >> 3) == 30) { cp = c & 0x07; n = 4; }
        else { i++; continue; }
        if (i + n > s.size()) break;
        for (int k = 1; k < n; k++) cp = (cp << 6) | (s[i + k] & 0x3f);
        out.push_back(cp);
        i += n;
    }
    return out;
}

struct TextSystem::Impl {
    FT_Library lib = nullptr;
    FT_Face faces[(int)Font::Count]{};
    std::string data[(int)Font::Count];  // FreeType reads from memory we own
};

TextSystem::TextSystem() : m(std::make_unique<Impl>()) {}
TextSystem::~TextSystem() {
    clearCache();
    for (auto f : m->faces) if (f) FT_Done_Face(f);
    if (m->lib) FT_Done_FreeType(m->lib);
}

bool TextSystem::init() {
    if (FT_Init_FreeType(&m->lib)) { logError("FreeType init failed"); return false; }
    const char* files[] = {"NotoSerif-Regular.ttf", "NotoSerif-SemiBold.ttf", "NotoSerif-Italic.ttf", "NotoSerifCJKjp-Bold-subset.otf"};
    for (int i = 0; i < (int)Font::Count; i++) {
        auto bytes = readFile(contentPath(std::string("assets/fonts/") + files[i]));
        if (!bytes) { logError("Missing font %s", files[i]); return false; }
        m->data[i] = std::move(*bytes);
        if (FT_New_Memory_Face(m->lib, (const FT_Byte*)m->data[i].data(), (FT_Long)m->data[i].size(), 0, &m->faces[i])) {
            logError("Cannot load font %s", files[i]);
            return false;
        }
    }
    return true;
}

void TextSystem::clearCache() {
    for (auto& [k, a] : m_atlases) if (a.tex) glDeleteTextures(1, &a.tex);
    m_atlases.clear();
}

TextSystem::Atlas& TextSystem::atlasFor(Font f, int px) {
    uint64_t key = ((uint64_t)f << 32) | (uint32_t)px;
    auto it = m_atlases.find(key);
    if (it != m_atlases.end()) return it->second;
    Atlas& a = m_atlases[key];
    a.size = px > 96 ? 2048 : 1024;
    glGenTextures(1, &a.tex);
    glBindTexture(GL_TEXTURE_2D, a.tex);
    std::vector<uint8_t> zero(a.size * a.size, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, a.size, a.size, 0, GL_RED, GL_UNSIGNED_BYTE, zero.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return a;
}

const TextSystem::Glyph& TextSystem::glyph(Font f, int px, uint32_t cp) {
    Atlas& a = atlasFor(f, px);
    auto it = a.glyphs.find(cp);
    if (it != a.glyphs.end()) return it->second;
    Glyph g{}; g.ok = false;
    FT_Face face = m->faces[(int)f];
    FT_UInt idx = FT_Get_Char_Index(face, cp);
    if (idx == 0) {  // fall back: Latin faces -> kanji face, kanji face -> bold
        face = (f == Font::Kanji) ? m->faces[(int)Font::Bold] : m->faces[(int)Font::Kanji];
        idx = FT_Get_Char_Index(face, cp);
    }
    if (idx && !a.full) {
        FT_Set_Pixel_Sizes(face, 0, px);
        if (!FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT) && !FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
            FT_Bitmap& bm = face->glyph->bitmap;
            g.w = bm.width; g.h = bm.rows;
            g.bx = face->glyph->bitmap_left; g.by = face->glyph->bitmap_top;
            g.adv = face->glyph->advance.x / 64.f;
            if (a.penX + g.w + 1 >= a.size) { a.penX = 1; a.penY += a.rowH + 1; a.rowH = 0; }
            if (a.penY + g.h + 1 >= a.size) { a.full = true; logWarn("glyph atlas full (px=%d)", px); }
            else {
                if (g.w > 0 && g.h > 0) {
                    std::vector<uint8_t> buf(g.w * g.h);
                    for (int y = 0; y < g.h; y++)
                        for (int x = 0; x < g.w; x++) buf[y * g.w + x] = bm.buffer[y * bm.pitch + x];
                    glBindTexture(GL_TEXTURE_2D, a.tex);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, a.penX, a.penY, g.w, g.h, GL_RED, GL_UNSIGNED_BYTE, buf.data());
                }
                g.u0 = a.penX / (float)a.size; g.v0 = a.penY / (float)a.size;
                g.u1 = (a.penX + g.w) / (float)a.size; g.v1 = (a.penY + g.h) / (float)a.size;
                a.penX += g.w + 1;
                a.rowH = std::max(a.rowH, g.h);
                g.ok = true;
            }
        }
    }
    if (!g.ok && cp == ' ') { g.adv = px * 0.26f; g.ok = true; g.w = g.h = 0; }
    return a.glyphs[cp] = g;
}

float TextSystem::measure(Font f, float size, const std::string& s, float tracking) {
    int px = std::max(4, (int)std::lround(size * m_pxScale));
    float w = 0;
    auto cps = decodeUtf8(s);
    for (uint32_t cp : cps) w += glyph(f, px, cp).adv / m_pxScale + tracking;
    if (!cps.empty()) w -= tracking;
    return w;
}

float TextSystem::draw(Renderer& r, Font f, float size, Vec2 pos, const std::string& s, Color col, Align align, float tracking) {
    int px = std::max(4, (int)std::lround(size * m_pxScale));
    float w = measure(f, size, s, tracking);
    float x = pos.x;
    if (align == Align::Center) x -= w * .5f;
    else if (align == Align::Right) x -= w;
    float inv = 1.f / m_pxScale;
    x = std::round(x * m_pxScale) * inv;
    float y = std::round(pos.y * m_pxScale) * inv;
    Atlas& a = atlasFor(f, px);
    for (uint32_t cp : decodeUtf8(s)) {
        const Glyph& g = glyph(f, px, cp);
        if (g.ok && g.w > 0) {
            Rect d{x + g.bx * inv, y - g.by * inv, g.w * inv, g.h * inv};
            r.textured(a.tex, true, d, {g.u0, g.v0, g.u1 - g.u0, g.v1 - g.v0}, col);
        }
        x += g.adv * inv + tracking;
    }
    return w;
}

std::vector<std::string> TextSystem::wrap(Font f, float size, const std::string& s, float maxWidth) {
    std::vector<std::string> lines;
    for (const auto& para : split(s, '\n')) {
        std::string cur;
        for (const auto& word : split(para, ' ')) {
            std::string cand = cur.empty() ? word : cur + " " + word;
            if (!cur.empty() && measure(f, size, cand) > maxWidth) { lines.push_back(cur); cur = word; }
            else cur = cand;
        }
        lines.push_back(cur);
    }
    return lines;
}

} // namespace sv
