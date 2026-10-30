#include "ui/glyphs.h"
#include "render/renderer.h"
#include <vector>

namespace sv {

namespace {
constexpr int kMove = (int)Act::Count, kAim = (int)Act::Count + 1;
const Color kFill = rgb(0x1a1b23), kRing = rgb(0x6c6e7c), kLabel = rgb(0xefe6d2);

void pill(Renderer& r, const Rect& rc, Color fill, Color ring) {
    float rad = rc.h * 0.5f;
    for (int pass = 0; pass < 2; pass++) {
        float g = pass == 0 ? -1.5f : 0.f;    // ring first, slightly larger
        Color c = pass == 0 ? ring : fill;
        Rect q{rc.x - g, rc.y - g, rc.w + g * 2, rc.h + g * 2};
        float rr = q.h * 0.5f;
        r.rect({q.x + rr, q.y, q.w - rr * 2, q.h}, c);
        r.circle({q.x + rr, q.y + rr}, rr, c);
        r.circle({q.right() - rr, q.y + rr}, rr, c);
    }
    (void)rad;
}

void keycap(Renderer& r, const Rect& rc, float alpha) {
    r.rect({rc.x, rc.y + rc.h * 0.12f, rc.w, rc.h * 0.88f}, rgb(0x0c0c10, alpha));
    r.rect({rc.x, rc.y, rc.w, rc.h * 0.86f}, rgb(0x2a2a32, alpha));
    r.rect({rc.x, rc.y, rc.w, 1.5f}, rgb(0x8a8a96, 0.6f * alpha));
}

enum class Face { None, Cross, Circle, Square, Triangle };

Face faceFor(int a) {
    switch ((Act)a) {
    case Act::Jump: case Act::Confirm: return Face::Cross;
    case Act::Dash: case Act::Cancel: return Face::Circle;
    case Act::Attack: return Face::Square;
    case Act::Art: case Act::Interact: return Face::Triangle;
    default: return Face::None;
    }
}

std::string padLabel(int a, bool ps) {
    switch ((Act)a) {
    case Act::Guard: case Act::TabPrev: return ps ? "L1" : "LB";
    case Act::Grapple: case Act::TabNext: return ps ? "R1" : "RB";
    case Act::Heal: return ps ? "L2" : "LT";
    case Act::Shuriken: return ps ? "R2" : "RT";
    case Act::Pause: return ps ? "OPTIONS" : "MENU";
    case Act::Menu: return ps ? "TOUCHPAD" : "VIEW";
    default: return "";
    }
}

std::string keyFor(const Input& in, int a) {
    if (a == kMove) return "A  D";
    if (a == kAim) return "W / S";
    return in.keyLabel((Act)a);
}

Input::Style resolve(const Input& in, Input::Style forced) { return (int)forced >= 0 ? forced : in.style(); }
}

float glyphWidth(Renderer& r, const Input& in, Act a, float size, Input::Style forced) {
    Input::Style st = resolve(in, forced);
    int ai = (int)a;
    if (st == Input::Style::Keyboard) {
        float tw = r.text().measure(Font::Bold, size * 0.46f, keyFor(in, ai));
        return std::max(size, tw + size * 0.55f);
    }
    if (ai == kMove || ai == kAim || faceFor(ai) != Face::None || a == Act::Up || a == Act::Down || a == Act::Left || a == Act::Right) return size;
    std::string lbl = padLabel(ai, st == Input::Style::PlayStation);
    return r.text().measure(Font::Bold, size * 0.42f, lbl) + size * 0.7f;
}

float drawGlyph(Renderer& r, const Input& in, Act a, Vec2 c, float s, float alpha, Input::Style forced) {
    Input::Style st = resolve(in, forced);
    TextSystem& T = r.text();
    int ai = (int)a;
    float w = glyphWidth(r, in, a, s, forced);
    if (st == Input::Style::Keyboard) {
        Rect rc{c.x - w / 2, c.y - s * 0.46f, w, s * 0.92f};
        keycap(r, rc, alpha);
        T.draw(r, Font::Bold, s * 0.46f, {c.x, c.y + s * 0.13f}, keyFor(in, ai), kLabel.withA(alpha), Align::Center);
        return w;
    }
    bool ps = st == Input::Style::PlayStation;
    float R = s * 0.5f;
    if (ai == kMove || ai == kAim) {
        r.circle(c, R, kRing.withA(alpha));
        r.circle(c, R - 1.5f, kFill.withA(alpha));
        r.ring(c, R * 0.55f, R * 0.68f, kRing.withA(alpha));
        T.draw(r, Font::Bold, s * 0.4f, {c.x, c.y + s * 0.14f}, ai == kMove ? "L" : "R", kLabel.withA(alpha), Align::Center);
        return w;
    }
    if (a == Act::Up || a == Act::Down || a == Act::Left || a == Act::Right) {
        // D-pad with the pressed arm lit
        float k = s * 0.2f, L = s * 0.46f;
        Color dim = kRing.withA(alpha), lit = kLabel.withA(alpha);
        r.rect({c.x - k, c.y - L, k * 2, L * 2}, dim);
        r.rect({c.x - L, c.y - k, L * 2, k * 2}, dim);
        if (a == Act::Up) r.rect({c.x - k, c.y - L, k * 2, L - k}, lit);
        if (a == Act::Down) r.rect({c.x - k, c.y + k, k * 2, L - k}, lit);
        if (a == Act::Left) r.rect({c.x - L, c.y - k, L - k, k * 2}, lit);
        if (a == Act::Right) r.rect({c.x + k, c.y - k, L - k, k * 2}, lit);
        return w;
    }
    Face f = faceFor(ai);
    if (f != Face::None) {
        r.circle(c, R, kRing.withA(alpha));
        r.circle(c, R - 1.5f, kFill.withA(alpha));
        float q = s * 0.2f, lw = std::max(1.5f, s * 0.08f);
        if (ps) {
            switch (f) {
            case Face::Cross: { Color k = rgb(0x8fb4ff, alpha); r.line(c + Vec2{-q, -q}, c + Vec2{q, q}, lw, k); r.line(c + Vec2{-q, q}, c + Vec2{q, -q}, lw, k); break; }
            case Face::Circle: r.ring(c, q * 1.05f - lw * 0.5f, q * 1.05f + lw * 0.5f, rgb(0xff7070, alpha)); break;
            case Face::Square: {
                Color k = rgb(0xff9ad8, alpha); float h = q * 0.9f;
                r.line(c + Vec2{-h, -h}, c + Vec2{h, -h}, lw, k); r.line(c + Vec2{h, -h}, c + Vec2{h, h}, lw, k);
                r.line(c + Vec2{h, h}, c + Vec2{-h, h}, lw, k); r.line(c + Vec2{-h, h}, c + Vec2{-h, -h}, lw, k);
                break;
            }
            case Face::Triangle: {
                Color k = rgb(0x6fe6b8, alpha); Vec2 p0 = c + Vec2{0, -q * 1.1f}, p1 = c + Vec2{q, q * 0.75f}, p2 = c + Vec2{-q, q * 0.75f};
                r.line(p0, p1, lw, k); r.line(p1, p2, lw, k); r.line(p2, p0, lw, k);
                break;
            }
            default: break;
            }
        } else {
            const char* letter = f == Face::Cross ? "A" : f == Face::Circle ? "B" : f == Face::Square ? "X" : "Y";
            Color k = f == Face::Cross ? rgb(0x6cd46c) : f == Face::Circle ? rgb(0xff6a6a) : f == Face::Square ? rgb(0x6aa8ff) : rgb(0xf0d050);
            T.draw(r, Font::Bold, s * 0.5f, {c.x, c.y + s * 0.17f}, letter, k.withA(alpha), Align::Center);
        }
        return w;
    }
    std::string lbl = padLabel(ai, ps);
    pill(r, {c.x - w / 2, c.y - s * 0.38f, w, s * 0.76f}, kFill.withA(alpha), kRing.withA(alpha));
    T.draw(r, Font::Bold, s * 0.38f, {c.x, c.y + s * 0.13f}, lbl, kLabel.withA(alpha), Align::Center);
    return w;
}

namespace {
struct Seg { bool glyph; std::string text; int act; };

int tokenAct(const std::string& t) {
    static const std::pair<const char*, int> table[] = {
        {"Jump", (int)Act::Jump}, {"Attack", (int)Act::Attack}, {"Guard", (int)Act::Guard}, {"Dash", (int)Act::Dash},
        {"Grapple", (int)Act::Grapple}, {"Shuriken", (int)Act::Shuriken}, {"Art", (int)Act::Art}, {"Heal", (int)Act::Heal},
        {"Interact", (int)Act::Interact}, {"Pause", (int)Act::Pause}, {"Menu", (int)Act::Menu}, {"Confirm", (int)Act::Confirm},
        {"Cancel", (int)Act::Cancel}, {"Down", (int)Act::Down}, {"Up", (int)Act::Up}, {"Left", (int)Act::Left}, {"Right", (int)Act::Right},
        {"TabPrev", (int)Act::TabPrev}, {"TabNext", (int)Act::TabNext}, {"Move", kMove}, {"Aim", kAim}};
    for (auto& [n, a] : table) if (t == n) return a;
    return -1;
}

std::vector<Seg> parse(const std::string& s) {
    std::vector<Seg> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '{') {
            size_t j = s.find('}', i);
            if (j != std::string::npos) {
                int a = tokenAct(s.substr(i + 1, j - i - 1));
                if (a >= 0) {
                    if (!cur.empty()) { out.push_back({false, cur, 0}); cur.clear(); }
                    out.push_back({true, "", a});
                    i = j;
                    continue;
                }
            }
        }
        cur += s[i];
    }
    if (!cur.empty()) out.push_back({false, cur, 0});
    return out;
}
}

float measureRich(Renderer& r, const Input& in, Font f, float size, const std::string& text) {
    float w = 0;
    for (auto& seg : parse(text))
        w += seg.glyph ? glyphWidth(r, in, (Act)seg.act, size * 1.3f) + size * 0.3f : r.text().measure(f, size, seg.text);
    return w;
}

float drawRich(Renderer& r, const Input& in, Font f, float size, Vec2 pos, const std::string& text, Color col, Align align) {
    float w = measureRich(r, in, f, size, text);
    float x = pos.x - (align == Align::Center ? w / 2 : (align == Align::Right ? w : 0));
    for (auto& seg : parse(text)) {
        if (seg.glyph) {
            float gs = size * 1.3f;
            float gw = glyphWidth(r, in, (Act)seg.act, gs);
            drawGlyph(r, in, (Act)seg.act, {x + size * 0.15f + gw / 2, pos.y - size * 0.34f}, gs, col.a);
            x += gw + size * 0.3f;
        } else x += r.text().draw(r, f, size, {x, pos.y}, seg.text, col);
    }
    return w;
}

} // namespace sv
