#include "game/world.h"
#include "render/renderer.h"
#include "render/text.h"
#include "core/util.h"
#include "ui/glyphs.h"

namespace sv {

// =================================================================== background construction

namespace {
struct BgBuilder {
    World::BgLayer& L;
    void tri(Vec2 a, Vec2 b, Vec2 c, float shade = 1, float alpha = 1) {
        L.tris.push_back({a, shade, alpha}); L.tris.push_back({b, shade, alpha}); L.tris.push_back({c, shade, alpha});
    }
    void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, float shade = 1, float alpha = 1) { tri(a, b, c, shade, alpha); tri(a, c, d, shade, alpha); }
    void rect(float x, float y, float w, float h, float shade = 1, float alpha = 1) { quad({x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, shade, alpha); }
    void ellipse(Vec2 c, float rx, float ry, float shade = 1, float alpha = 1, int seg = 18) {
        for (int i = 0; i < seg; i++) {
            float a0 = TAU * i / seg, a1 = TAU * (i + 1) / seg;
            tri(c, c + Vec2{std::cos(a0) * rx, std::sin(a0) * ry}, c + Vec2{std::cos(a1) * rx, std::sin(a1) * ry}, shade, alpha);
        }
    }
    // a ridge line filled down to `bottom`
    void ridge(float x0, float x1, float base, float amp, float freq, int seed, float sharp, float bottom, float shade = 1) {
        float step = 24;
        float prevX = x0, prevY = base - amp * ridgeH(x0, freq, seed, sharp);
        for (float x = x0 + step; x <= x1 + step; x += step) {
            float y = base - amp * ridgeH(x, freq, seed, sharp);
            quad({prevX, prevY}, {x, y}, {x, bottom}, {prevX, bottom}, shade);
            prevX = x; prevY = y;
        }
    }
    static float ridgeH(float x, float freq, int seed, float sharp) {
        float v = fbm1(x * freq, seed, 5);
        float r = 1 - std::abs(v * 2 - 1);  // ridged
        return lerp(v, r * r, sharp);
    }
    // curved, upturned house roof
    void roof(float cx, float y, float halfW, float h, float flare, float shade = 1) {
        int n = 10;
        Vec2 prevL{cx - halfW, y + flare * -0.0f}, prevR{cx + halfW, y};
        for (int i = 1; i <= n; i++) {
            float t = i / (float)n;
            float curve = std::pow(1 - t, 2.2f);
            float hw = halfW * (1 - t * 0.55f);
            float yy = y - h * t + flare * curve * 0;
            Vec2 l{cx - hw, yy - flare * curve * 0.5f}, r{cx + hw, yy - flare * curve * 0.5f};
            if (i == 1) { prevL.y -= flare; prevR.y -= flare; }
            quad(prevL, prevR, r, l, shade);
            prevL = l; prevR = r;
        }
        rect(cx - halfW * 0.45f, y - h - 6, halfW * 0.9f, 6, shade);
    }
    void watchtower(float cx, float base, float s, World::BgLayer& layer, Rng& rng) {
        // a fire-watch tower: splayed timber legs, a railed lookout and a small hipped roof
        float h = 300 * s, w = 70 * s, top = base - h;
        quad({cx - w, base}, {cx - w + 8 * s, base}, {cx - w * 0.45f + 8 * s, top}, {cx - w * 0.45f, top});
        quad({cx + w - 8 * s, base}, {cx + w, base}, {cx + w * 0.45f, top}, {cx + w * 0.45f - 8 * s, top});
        for (int k = 1; k < 4; k++) {
            float y = base - h * k / 4.f, hw = lerp(w, w * 0.45f, k / 4.f);
            rect(cx - hw, y - 3 * s, hw * 2, 6 * s);
            line({cx - hw, y}, {cx + lerp(w, w * 0.45f, (k - 1) / 4.f), base - h * (k - 1) / 4.f}, 3 * s);
        }
        rect(cx - w * 0.7f, top - 40 * s, w * 1.4f, 40 * s);
        layer.windows.push_back({{cx, top - 20 * s}, 14 * s});
        roof(cx, top - 38 * s, w * 0.95f, 34 * s, 10 * s);
        (void)rng;
    }
    void house(float x, float base, float w, float h, World::BgLayer& layer, Rng& rng, bool lit) {
        rect(x, base - h, w, h);
        roof(x + w / 2, base - h + 4, w * 0.65f, h * 0.55f, h * 0.18f);
        if (lit) {
            int nw = std::max(1, (int)(w / 50));
            for (int i = 0; i < nw; i++)
                if (rng.chance(0.55f)) layer.windows.push_back({{x + (i + 0.5f) * w / nw, base - h * 0.45f}, h * 0.16f});
        }
    }
    void bamboo(float x, float base, float h, float w, Rng& rng, float shade = 1) {
        float lean = rng.range(-0.05f, 0.05f);
        Vec2 a{x, base}, b{x + lean * h, base - h};
        Vec2 n{w / 2, 0};
        quad(a - n, a + n, b + n * 0.8f, b - n * 0.8f, shade);
        for (float t = rng.range(0.08f, 0.15f); t < 1; t += rng.range(0.1f, 0.16f)) {
            Vec2 p = lerp(a, b, t);
            rect(p.x - w * 0.7f, p.y - 1.5f, w * 1.4f, 3, shade * 0.8f);
        }
        // leaf sprays near the top
        for (int k = 0; k < 5; k++) {
            Vec2 p = lerp(a, b, rng.range(0.55f, 1.0f));
            float dir = rng.chance(0.5f) ? 1.f : -1.f;
            for (int l = 0; l < 4; l++) {
                float ang = rng.range(0.2f, 0.9f);
                float len = rng.range(18, 34) * (w / 8 + 0.6f);
                Vec2 tip = p + Vec2{dir * std::cos(ang) * len, std::sin(ang) * len * 0.6f + rng.range(-6, 10)};
                Vec2 mid = lerp(p, tip, 0.5f);
                Vec2 perp = (tip - p).norm().perp() * (len * 0.12f);
                quad(p, mid + perp, tip, mid - perp, shade);
            }
        }
    }
    void pine(float x, float base, float h, Rng& rng, float shade = 1) {
        // gnarled trunk + flat cloud-pads of needles
        Vec2 p{x, base};
        float ang = rng.range(-0.3f, 0.3f);
        float w = h * 0.06f;
        std::vector<Vec2> pads;
        for (int i = 0; i < 7; i++) {
            ang += rng.range(-0.35f, 0.35f);
            ang = clampf(ang, -0.7f, 0.7f);
            Vec2 q = p + Vec2{std::sin(ang) * h / 7, -std::cos(ang) * h / 7};
            Vec2 n = (q - p).norm().perp() * (w * (1 - i / 9.f));
            quad(p - n, p + n, q + n * 0.85f, q - n * 0.85f, shade);
            if (i >= 3) pads.push_back(q);
            p = q;
        }
        for (auto& c : pads) {
            float side = rng.chance(0.5f) ? 1.f : -1.f;
            Vec2 bc = c + Vec2{side * rng.range(20, 60) * h / 300, rng.range(-10, 10)};
            line(c, bc, w * 0.5f, shade);
            ellipse(bc, rng.range(40, 70) * h / 300, rng.range(10, 16) * h / 300, shade);
            ellipse(bc + Vec2{rng.range(-15, 15), -8 * h / 300}, rng.range(25, 45) * h / 300, rng.range(8, 12) * h / 300, shade);
        }
        ellipse(p, 55 * h / 300, 14 * h / 300, shade);
    }
    void line(Vec2 a, Vec2 b, float w, float shade = 1) {
        Vec2 n = (b - a).norm().perp() * (w / 2);
        quad(a + n, b + n, b - n, a - n, shade);
    }
    void gateway(float cx, float base, float s, float shade = 1) {
        // a fortified gatehouse: stone footings, heavy timber posts, closed doors and a tiled roof
        float h = 230 * s, w = 240 * s;
        quad({cx - w * 0.5f, base}, {cx - w * 0.28f, base}, {cx - w * 0.3f, base - 70 * s}, {cx - w * 0.47f, base - 70 * s}, shade);
        quad({cx + w * 0.28f, base}, {cx + w * 0.5f, base}, {cx + w * 0.47f, base - 70 * s}, {cx + w * 0.3f, base - 70 * s}, shade);
        rect(cx - w * 0.44f, base - h, w * 0.12f, h - 60 * s, shade);
        rect(cx + w * 0.32f, base - h, w * 0.12f, h - 60 * s, shade);
        rect(cx - w * 0.32f, base - h * 0.92f, w * 0.64f, h * 0.92f, shade * 0.85f);
        rect(cx - 2 * s, base - h * 0.92f, 4 * s, h * 0.92f, shade * 0.6f);
        rect(cx - w * 0.5f, base - h - 18 * s, w, 22 * s, shade);
        roof(cx, base - h - 14 * s, w * 0.7f, 70 * s, 22 * s, shade);
    }
    void castle(float cx, float base, float s, World::BgLayer& layer, Rng& rng) {
        // stone base, then stacked keep with roofs — Castle Hakuro on the far peak
        quad({cx - 120 * s, base}, {cx + 120 * s, base}, {cx + 95 * s, base - 60 * s}, {cx - 95 * s, base - 60 * s});
        float y = base - 60 * s, w = 170 * s;
        for (int t = 0; t < 4; t++) {
            float bh = 34 * s;
            rect(cx - w * 0.4f, y - bh, w * 0.8f, bh);
            if (rng.chance(0.8f)) layer.windows.push_back({{cx + rng.range(-w * 0.25f, w * 0.25f), y - bh * 0.5f}, 5 * s});
            y -= bh;
            roof(cx, y + 3 * s, w * 0.55f, 16 * s, 10 * s);
            y -= 16 * s;
            w *= 0.78f;
        }
    }
    void rockPillar(float x, float base, float w, float h, Rng& rng, float shade = 1) {
        float top = base - h;
        int n = 8;
        float prevL = x, prevR = x + w;
        float py = base;
        for (int i = 1; i <= n; i++) {
            float y = base - h * i / n;
            float l = x + rng.range(-w * 0.12f, w * 0.15f) + (i / (float)n) * w * 0.1f;
            float r = x + w + rng.range(-w * 0.15f, w * 0.12f) - (i / (float)n) * w * 0.1f;
            quad({prevL, py}, {prevR, py}, {r, y}, {l, y}, shade);
            prevL = l; prevR = r; py = y;
        }
        tri({prevL, top}, {prevR, top}, {(prevL + prevR) / 2 + rng.range(-10, 10), top - rng.range(10, 40)}, shade);
    }
    // a ridge hanging from above, mirrored (the Mirror Province's upside-down sky-land)
    void ridgeInv(float x0, float x1, float base, float amp, float freq, int seed, float sharp, float top, float shade = 1) {
        float step = 24;
        float prevX = x0, prevY = base + amp * ridgeH(x0, freq, seed, sharp);
        for (float x = x0 + step; x <= x1 + step; x += step) {
            float y = base + amp * ridgeH(x, freq, seed, sharp);
            quad({prevX, top}, {x, top}, {x, y}, {prevX, prevY}, shade);
            prevX = x; prevY = y;
        }
    }
    // stepped paddy terraces cut into a hillside, each step holding a sheet of water
    void terraces(float x0, float x1, float base, Rng& rng, World::BgLayer& layer, float s) {
        float y = base;
        float l = x0, rr = x1;
        for (int k = 0; k < 6; k++) {
            float h = rng.range(26, 40) * s;
            rect(l, y - h, rr - l, h + 4, 1.0f);
            // the water's edge catches the sky
            rect(l, y - h - 2 * s, rr - l, 2 * s, 1.9f, 0.5f);
            y -= h;
            l += rng.range(60, 160) * s; rr -= rng.range(60, 160) * s;
            if (rr - l < 200) break;
        }
        (void)layer;
    }
    // a crenellated castle curtain wall with towers
    void wall(float x0, float x1, float base, float h, World::BgLayer& layer, Rng& rng, float shade = 1) {
        rect(x0, base - h, x1 - x0, h + 2400, shade);
        for (float x = x0; x < x1; x += 34) rect(x, base - h - 14, 18, 14, shade);
        for (float x = x0 + rng.range(100, 300); x < x1; x += rng.range(500, 800)) {
            rect(x - 50, base - h - 90, 100, 90, shade);
            roof(x, base - h - 86, 80, 40, 12, shade);
            if (rng.chance(0.6f)) layer.windows.push_back({{x, base - h - 50}, 10});
        }
    }
    // a floating island fragment with roots hanging below
    void island(Vec2 c, float w, Rng& rng, float shade = 1) {
        ellipse(c, w * 0.5f, w * 0.08f, shade);
        tri(c + Vec2{-w * 0.5f, 0}, c + Vec2{w * 0.5f, 0}, c + Vec2{rng.range(-w * 0.1f, w * 0.1f), w * 0.45f}, shade);
        for (int k = 0; k < 4; k++) line(c + Vec2{rng.range(-w * 0.3f, w * 0.3f), w * 0.05f}, c + Vec2{rng.range(-w * 0.35f, w * 0.35f), w * rng.range(0.3f, 0.7f)}, 2, shade);
    }
    // the great loom: warp threads from sky to earth, crossed by a weft
    void loom(float x0, float x1, float base, float h, Rng& rng, float shade = 1) {
        for (float x = x0; x < x1; x += rng.range(40, 90)) line({x, base}, {x + rng.range(-30, 30), base - h}, rng.range(1.5f, 4), shade);
        for (float y = base - h * 0.2f; y > base - h; y -= rng.range(80, 160)) line({x0, y}, {x1, y + rng.range(-40, 40)}, 1.5f, shade * 0.9f);
    }
    void maple(float x, float base, float h, Rng& rng, float shade = 1) {
        Vec2 p{x, base};
        line(p, p + Vec2{rng.range(-10, 10), -h * 0.55f}, h * 0.05f, shade);
        for (int k = 0; k < 9; k++) {
            Vec2 c{x + rng.range(-h * 0.35f, h * 0.35f), base - h * rng.range(0.5f, 0.95f)};
            ellipse(c, rng.range(h * 0.12f, h * 0.22f), rng.range(h * 0.08f, h * 0.14f), shade * rng.range(0.85f, 1.1f));
        }
    }
};
}

void World::buildBackground() {
    bgLayers.clear();
    const float par[5] = {0.06f, 0.15f, 0.3f, 0.52f, 0.76f};
    const int key[5] = {0, 0, 1, 1, 2};
    const float fogMix[5] = {0.6f, 0.38f, 0.32f, 0.16f, 0.05f};
    for (int i = 0; i < 5; i++) { BgLayer L; L.parallax = par[i]; L.colorKey = key[i]; L.fogMix = fogMix[i]; L.yOffset = 0; bgLayers.push_back(L); }
    // typical camera height when standing on the main ground line
    float groundRef = (level.height - 5) * TILE;
    float camRef = groundRef - 250;
    const float screenOff[5] = {60, 130, 210, 300, 400};
    const float halfView = 1300;
    for (size_t ri = 0; ri < level.rooms.size(); ri++) {
        const Room& R = level.rooms[ri];
        for (int li = 0; li < 5; li++) {
            BgLayer& L = bgLayers[li];
            BgBuilder B{L};
            float p = L.parallax;
            float x0 = R.x0 * TILE * p, x1 = (R.x0 + R.w) * TILE * p;
            if (ri == 0) x0 -= halfView * 1.2f;
            if (ri + 1 == level.rooms.size()) x1 += halfView * 1.2f;
            float base = camRef * p + screenOff[li];
            float bottom = base + 2400;
            Rng rng(R.seed * 31 + li * 7);
            switch (li) {
            case 0: {
                float amp = R.style == BgStyle::Grove ? 160 : (R.style == BgStyle::Gate ? 260 : (R.style == BgStyle::Terrace ? 200 : (R.style == BgStyle::Castle ? 420 : 320)));
                if (R.style == BgStyle::Loom) {
                    B.loom(x0, x1, base + 200, 2200, rng, 1.0f);
                    break;
                }
                B.ridge(x0, x1, base, amp, 0.0016f, 11, R.style == BgStyle::Grove ? 0.1f : 0.7f, bottom, 1.0f);
                if (R.style == BgStyle::Mirror) B.ridgeInv(x0, x1, base - 900, amp, 0.0016f, 11, 0.7f, base - 3000, 1.0f);
                break;
            }
            case 1: {
                float amp = R.style == BgStyle::Ravine ? 300 : 170;
                B.ridge(x0, x1, base + 30, amp, 0.0032f, 23 + (int)ri, 0.4f, bottom, 1.0f);
                if (R.style == BgStyle::Castle || R.style == BgStyle::Maple) {
                    // Castle Hakuro looms nearer with every step up the pass
                    float cx = (x0 + x1) * 0.5f;
                    float ridgeY = base + 30 - 170 * BgBuilder::ridgeH(cx, 0.0032f, 23 + (int)ri, 0.4f);
                    B.castle(cx, ridgeY + 6, R.style == BgStyle::Castle ? 1.3f : 0.75f, L, rng);
                }
                if (R.style == BgStyle::Mirror || R.style == BgStyle::Loom)
                    for (float x = x0; x < x1; x += rng.range(300, 600)) B.island({x, base - rng.range(200, 600)}, rng.range(120, 260), rng);
                if (R.style == BgStyle::Cliff || R.style == BgStyle::Village) {
                    // Castle Hakuro, far away on the ridge: the story's destination
                    float cx = (x0 + x1) * 0.5f + 200;
                    float ridgeY = base + 30 - 170 * BgBuilder::ridgeH(cx, 0.0032f, 23 + (int)ri, 0.4f);
                    if (ri % 2 == 0) B.castle(cx, ridgeY + 6, 0.55f, L, rng);
                }
                break;
            }
            case 2: case 3: case 4: {
                float density = li == 2 ? 0.7f : (li == 3 ? 1.0f : 0.5f);
                float s = li == 2 ? 0.7f : (li == 3 ? 1.0f : 1.35f);
                switch (R.style) {
                case BgStyle::Cliff:
                    if (li == 2) B.ridge(x0, x1, base + 40, 90, 0.004f, 5 + (int)ri, 0.2f, bottom, 1.0f);
                    for (float x = x0; x < x1; x += rng.range(260, 520) / density) {
                        if (li == 4 && rng.chance(0.5f)) continue;
                        B.pine(x, base + rng.range(0, 40), rng.range(200, 320) * s, rng);
                    }
                    if (li == 3) B.rect(x0, base + 20, x1 - x0, 2400);
                    break;
                case BgStyle::Village: {
                    if (li == 2) {
                        B.ridge(x0, x1, base + 50, 50, 0.003f, 7, 0.1f, bottom);
                        B.watchtower((x0 + x1) * 0.5f + rng.range(-200, 200), base + 50, 1.0f, L, rng);
                        for (float x = x0; x < x1; x += rng.range(140, 260)) B.house(x, base + 60 + rng.range(0, 20), rng.range(90, 160), rng.range(50, 70), L, rng, true);
                    } else if (li == 3) {
                        for (float x = x0; x < x1; x += rng.range(220, 360)) B.house(x, base + 40 + rng.range(-10, 20), rng.range(140, 220), rng.range(80, 110), L, rng, true);
                        B.rect(x0, base + 40, x1 - x0, 2400);
                    }
                    break;
                }
                case BgStyle::Grove: {
                    if (li == 3) B.rect(x0, base + 30, x1 - x0, 2400);
                    float step = li == 2 ? 26 : (li == 3 ? 46 : 140);
                    for (float x = x0; x < x1; x += rng.range(step * 0.6f, step * 1.4f)) {
                        if (li == 4 && rng.chance(0.4f)) continue;
                        B.bamboo(x, base + 60 + rng.range(0, 30), rng.range(500, 900) * s, rng.range(5, 9) * s, rng);
                    }
                    break;
                }
                case BgStyle::Ravine: {
                    for (float x = x0; x < x1; x += rng.range(160, 380) / density) {
                        float h = rng.range(300, 700) * s;
                        B.rockPillar(x, base + 120, rng.range(60, 150) * s, h, rng);
                        if (rng.chance(0.4f)) B.pine(x + 40 * s, base + 120 - h + 10, 140 * s, rng);
                    }
                    if (li == 3) B.rect(x0, base + 100, x1 - x0, 2400);
                    break;
                }
                case BgStyle::Terrace: {
                    if (li == 2) {
                        B.terraces(x0, x1, base + 80, rng, L, 1.0f);
                        for (float x = x0; x < x1; x += rng.range(500, 900)) B.house(x, base + 70, rng.range(90, 140), rng.range(50, 70), L, rng, rng.chance(0.4f));
                    } else if (li == 3) {
                        B.rect(x0, base + 40, x1 - x0, 2400);
                        for (float x = x0; x < x1; x += rng.range(280, 520)) B.pine(x, base + 40, rng.range(180, 280) * s, rng);
                    } else {
                        for (float x = x0; x < x1; x += rng.range(160, 300)) B.bamboo(x, base + 80, rng.range(300, 600) * s, rng.range(5, 8) * s, rng);
                    }
                    break;
                }
                case BgStyle::Maple: {
                    if (li == 2) {
                        for (float x = x0; x < x1; x += rng.range(220, 420)) {
                            float h = rng.range(300, 600);
                            B.rockPillar(x, base + 120, rng.range(70, 140), h, rng);
                            B.maple(x + 40, base + 120 - h + 20, 120, rng);
                        }
                    } else {
                        for (float x = x0; x < x1; x += rng.range(160, 320) / density) B.maple(x, base + 40, rng.range(220, 360) * s, rng);
                        if (li == 3) B.rect(x0, base + 30, x1 - x0, 2400);
                    }
                    break;
                }
                case BgStyle::Castle: {
                    if (li == 2) B.wall(x0, x1, base + 20, 150, L, rng);
                    else if (li == 3) {
                        for (float x = x0; x < x1; x += rng.range(260, 480)) B.pine(x, base + 30, rng.range(200, 300) * s, rng);
                        B.rect(x0, base + 30, x1 - x0, 2400);
                    } else for (float x = x0; x < x1; x += rng.range(500, 900)) B.pine(x, base + 60, rng.range(260, 380) * s, rng);
                    break;
                }
                case BgStyle::Mirror: {
                    if (li == 2) {
                        B.ridge(x0, x1, base + 40, 80, 0.004f, 31 + (int)ri, 0.5f, bottom);
                        B.ridgeInv(x0, x1, base - 700, 80, 0.004f, 31 + (int)ri, 0.5f, base - 2600);
                    } else {
                        for (float x = x0; x < x1; x += rng.range(260, 520) / density) {
                            B.pine(x, base + 40, rng.range(200, 300) * s, rng);
                            if (li == 3 && rng.chance(0.5f)) B.island({x + 120, base - rng.range(300, 520)}, rng.range(80, 160) * s, rng);
                        }
                        if (li == 3) B.rect(x0, base + 30, x1 - x0, 2400);
                    }
                    break;
                }
                case BgStyle::Loom: {
                    if (li == 2) B.loom(x0, x1, base + 100, 1600, rng, 1.0f);
                    else {
                        for (float x = x0; x < x1; x += rng.range(400, 800) / density) B.island({x, base - rng.range(0, 400)}, rng.range(140, 300) * s, rng);
                        if (li == 3) B.rect(x0, base + 120, x1 - x0, 2400);
                    }
                    break;
                }
                case BgStyle::Gate: {
                    if (li == 2) {
                        B.ridge(x0, x1, base + 30, 80, 0.003f, 13, 0.3f, bottom);
                        // The Old Gate, impossibly large against the blood moon
                        B.gateway((x0 + x1) * 0.5f, base + 40, 2.4f);
                    } else {
                        for (float x = x0; x < x1; x += rng.range(220, 420) / density) B.maple(x, base + 40, rng.range(220, 380) * s, rng);
                        if (li == 3) { B.rect(x0, base + 30, x1 - x0, 2400); for (float x = x0; x < x1; x += rng.range(500, 800)) B.gateway(x, base + 40, 0.7f); }
                    }
                    break;
                }
                }
                break;
            }
            }
        }
    }
}

// =================================================================== drawing helpers

static Color layerColor(const Palette& P, int key, float fogMix) {
    Color c = key == 0 ? P.far : (key == 1 ? P.mid : P.nearC);
    return lerp(c, P.fog, fogMix);
}

Color World::rimAt(Vec2 p) const {
    Color c = palette.rim;
    float best = 0;
    Color lc = c;
    for (auto& L : lights) {
        float d = (L.pos - p).len();
        if (d > L.radius) continue;
        float w = std::pow(1 - d / L.radius, 2.f);
        if (w > best) { best = w; lc = L.col; }
    }
    return lerp(c, lc, clamp01(best * 1.6f));
}

void World::drawBackground(Renderer& r, Vec2 camPos, float zoom) {
    for (size_t li = 0; li < bgLayers.size(); li++) {
        auto& L = bgLayers[li];
        r.setWorldView(camPos, zoom, L.parallax);
        Color base = layerColor(palette, L.colorKey, L.fogMix);
        // cull roughly by view
        Vec2 vs = r.viewSize();
        float cx = camPos.x * L.parallax;
        float left = cx - vs.x * 0.6f, right = cx + vs.x * 0.6f;
        for (size_t i = 0; i + 2 < L.tris.size(); i += 3) {
            const auto& a = L.tris[i]; const auto& b = L.tris[i + 1]; const auto& c = L.tris[i + 2];
            float mnx = std::min(a.p.x, std::min(b.p.x, c.p.x)), mxx = std::max(a.p.x, std::max(b.p.x, c.p.x));
            if (mxx < left || mnx > right) continue;
            r.tri(a.p, b.p, c.p, Color{base.r * a.shade, base.g * a.shade, base.b * a.shade, a.alpha});
        }
        // lit windows in burning villages and the distant castle
        if (!L.windows.empty()) {
            r.setBlend(Blend::Additive);
            for (auto& [p, s] : L.windows) {
                if (p.x < left || p.x > right) continue;
                float fl = 0.7f + 0.3f * noise1(time * 3 + p.x * 0.1f, (int)p.y);
                Color g = palette.glow.mulA(0.0f);
                g = lerp(palette.glow, rgb(0xffd890), 0.3f);
                float amt = std::max(0.25f, palette.glowAmt) * fl;
                r.rect({p.x - s * 0.5f, p.y - s * 0.4f, s, s * 0.8f}, g.withA(0.9f * amt));
                r.glow(p, s * 3.5f, palette.glow.withA(0.35f * amt));
            }
            r.setBlend(Blend::Alpha);
        }
        // atmospheric fog band at the foot of each layer
        float groundRef = (level.height - 5) * TILE;
        float base0 = (groundRef - 250) * L.parallax + (li == 0 ? 60 : li == 1 ? 130 : li == 2 ? 210 : li == 3 ? 300 : 400);
        Color fog = palette.fog;
        r.rectGrad({left - 200, base0 - 160, right - left + 400, 220}, fog.withA(0), fog.withA(0.42f * (1 - li * 0.15f)));
        r.rect({left - 200, base0 + 60, right - left + 400, 3000}, fog.withA(0.42f * (1 - li * 0.15f)));
        // drifting noise mist
        if (li >= 1 && li <= 3) {
            float a = palette.mist * 0.22f;
            float u0 = (left * 0.0007f) + time * 0.006f * (li + 1);
            r.textured(r.noiseTex(), true, {left - 200, base0 - 260, right - left + 400, 360},
                       {u0, 0.1f * li, (right - left + 400) * 0.0007f, 0.35f}, fog.withA(a));
        }
    }
    // horizon glow of the burning village
    if (palette.glowAmt > 0.05f) {
        r.setWorldView(camPos, zoom, 0.05f);
        r.setBlend(Blend::Additive);
        Vec2 vs = r.viewSize();
        float cx = camPos.x * 0.05f, cy = camPos.y * 0.05f;
        float fl = 0.85f + 0.15f * noise1(time * 1.5f, 3);
        r.rectGrad({cx - vs.x, cy + vs.y * 0.05f, vs.x * 2, vs.y * 0.6f}, palette.glow.withA(0), palette.glow.withA(0.22f * palette.glowAmt * fl));
        r.setBlend(Blend::Alpha);
    }
}

// =================================================================== tiles & grass

void World::drawTiles(Renderer& r, const Rect& view) {
    int x0 = std::max(0, (int)std::floor(view.x / TILE) - 1), x1 = std::min(level.width - 1, (int)std::floor(view.right() / TILE) + 1);
    int y0 = std::max(0, (int)std::floor(view.y / TILE) - 1), y1 = std::min(level.height - 1, (int)std::floor(view.bottom() / TILE) + 1);
    Color ground = palette.ground;
    Color lip = lerp(palette.ground, palette.nearC, 0.9f);
    Color rim = palette.rim;
    Rect pb = playerBox();
    auto isSolidVis = [&](int x, int y) { Tile t = level.at(x, y); return t == Tile::Solid || t == Tile::FalseWall || t == Tile::Plank || t == Tile::Wall || t == Tile::Crumble; };
    bool snowy = palette.snow > 0.05f;
    Color wood = lerp(ground, rgb(0x2a1a10), 0.45f);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            Tile t = level.at(x, y);
            float px = x * TILE, py = y * TILE;
            switch (t) {
            case Tile::Solid: case Tile::FalseWall: {
                float a = 1;
                if (t == Tile::FalseWall) {
                    Rect area{px - TILE * 2, py - TILE * 2, TILE * 5, TILE * 5};
                    if (area.overlaps(pb)) a = 0.28f;
                }
                // depth gradient: tiles deeper below the surface get darker
                int depth = 0;
                while (depth < 4 && isSolidVis(x, y - depth - 1)) depth++;
                Color c = lerp(lip, ground, clamp01(depth / 2.f));
                r.rect({px, py, TILE + 0.5f, TILE + 0.5f}, c.withA(a));
                if (depth == 0) {
                    // exposed surface: a rim of light, a lip, and grass
                    float hseed = hashf(x, y);
                    r.rect({px, py, TILE + 0.5f, 5}, lerp(lip, rim, 0.12f).withA(a));
                    r.rect({px, py - 1, TILE + 0.5f, 1.6f}, rim.withA(0.42f * a));
                    if (level.at(x, y - 1) == Tile::Empty) {
                        int blades = 5 + (int)(hseed * 6);
                        for (int b = 0; b < blades; b++) {
                            float bx = px + hashf(x * 7 + b, y) * TILE;
                            float bh = 6 + hashf(x + b * 3, y * 5) * 14;
                            float sway = std::sin(time * 1.8f + bx * 0.03f) * (2 + palette.wind.x * 0.04f);
                            // grass parts around the player
                            float dpx = bx - player.pos.x;
                            if (std::abs(dpx) < 30 && std::abs(py - player.pos.y) < 20) sway += sign(dpx) * (30 - std::abs(dpx)) * 0.4f;
                            r.tri({bx - 1.6f, py + 1}, {bx + 1.6f, py + 1}, {bx + sway, py - bh}, palette.grass.withA(a));
                        }
                        if (hseed > 0.82f) r.circle({px + hseed * TILE, py + 2}, 5 + hseed * 4, lip.withA(a));
                    }
                }
                if (!isSolidVis(x - 1, y)) r.rect({px - 0.5f, py, 1.4f, TILE}, rim.withA(0.12f * a));
                if (!isSolidVis(x + 1, y)) r.rect({px + TILE - 1, py, 1.4f, TILE}, rim.withA(0.18f * a));
                if (snowy && depth == 0 && !isSolidVis(x, y - 1)) {
                    // a soft cap of snow on every exposed surface
                    float h = 6 + hashf(x, y * 3) * 4;
                    r.rect({px - 1, py - h * 0.5f, TILE + 2, h}, rgb(0xe8eef8, 0.92f * a));
                    r.circle({px + hashf(x * 5, y) * TILE, py - h * 0.4f}, h * 0.7f, rgb(0xe8eef8, 0.92f * a));
                }
                break;
            }
            case Tile::Crumble: {
                // weathered slabs held together by roots; they shiver when weight lands on them
                float shake = 0;
                for (auto& c : crumbling) if (c.first == y * level.width + x) shake = std::sin(time * 70) * 2;
                Color slab = lerp(lip, rgb(0x5a3a22), 0.35f);
                r.rect({px + shake, py, TILE + 0.5f, TILE * 0.7f}, slab);
                r.line({px + 8 + shake, py + 4}, {px + 20 + shake, py + 20}, 1.5f, ground);
                r.line({px + 30 + shake, py + 2}, {px + 26 + shake, py + 28}, 1.5f, ground);
                r.rect({px + shake, py - 1, TILE + 0.5f, 1.6f}, rim.withA(0.35f));
                for (int k = 0; k < 3; k++) r.line({px + 10 + k * 14.f, py + TILE * 0.7f}, {px + 8 + k * 14.f + hashf(x, k) * 6, py + TILE * 0.95f}, 1.2f, lip);
                break;
            }
            case Tile::CrumbleGone: {
                // a faint outline promises the ledge will return
                r.rect({px + 4, py + 2, TILE - 8, 1.2f}, rim.withA(0.12f));
                break;
            }
            case Tile::Bright: case Tile::Hollow: {
                // Mirror Province: stone that exists in only one of the two layers
                bool brightTile = t == Tile::Bright;
                bool real = brightTile ? level.layer == 0 : level.layer == 1;
                Color tc = brightTile ? rgb(0xe8e0d0) : rgb(0x6a4aa8);
                if (real) {
                    r.rect({px, py, TILE + 0.5f, TILE + 0.5f}, lerp(ground, tc, 0.28f));
                    if (level.at(x, y - 1) != t) r.rect({px, py - 1, TILE + 0.5f, 2}, tc.withA(0.8f));
                    r.rect({px + 3, py + 3, TILE - 6, 1}, tc.withA(0.15f));
                } else {
                    // a ghost: thread outlines only
                    float pulse = 0.25f + 0.1f * std::sin(time * 2 + x * 0.3f);
                    r.setBlend(Blend::Additive);
                    if (level.at(x, y - 1) != t) r.rect({px, py, TILE, 1.2f}, tc.withA(pulse));
                    if (level.at(x, y + 1) != t) r.rect({px, py + TILE - 1, TILE, 1.2f}, tc.withA(pulse * 0.6f));
                    if (level.at(x - 1, y) != t) r.rect({px, py, 1.2f, TILE}, tc.withA(pulse * 0.6f));
                    if (level.at(x + 1, y) != t) r.rect({px + TILE - 1, py, 1.2f, TILE}, tc.withA(pulse * 0.6f));
                    r.setBlend(Blend::Alpha);
                }
                break;
            }
            case Tile::Plank: {
                r.rect({px, py, TILE + 0.5f, TILE + 0.5f}, wood);
                for (int k = 1; k < 4; k++) r.rect({px, py + k * 12.f, TILE, 1.5f}, ground.withA(0.6f));
                if ((x + y) % 2 == 0) r.rect({px + TILE - 2, py, 2, TILE}, ground.withA(0.5f));
                if (!isSolidVis(x, y - 1)) r.rect({px, py - 1, TILE + 0.5f, 1.6f}, rim.withA(0.4f));
                break;
            }
            case Tile::Wall: {
                // timber-framed house wall; some shoji glow from fires inside
                Color wall = lerp(ground, rgb(0x1c130d), 0.55f);
                r.rect({px, py, TILE + 0.5f, TILE + 0.5f}, wall);
                r.rect({px + TILE - 5, py, 5, TILE + 0.5f}, ground);
                if (y % 3 == 0) r.rect({px, py + 2, TILE + 0.5f, 6}, ground);
                bool exposedTop = !isSolidVis(x, y - 1);
                if (exposedTop) { r.rect({px - 1, py - 4, TILE + 2, 8}, wood * 0.8f); r.rect({px - 1, py - 5, TILE + 2, 1.6f}, rim.withA(0.4f)); }
                float hseed = hashf(x * 3, y * 7);
                if (!exposedTop && y % 3 == 1 && hseed > 0.45f) {
                    Rect pane{px + 7, py + 8, TILE - 19, TILE - 18};
                    float fl = 0.7f + 0.3f * noise1(time * 3 + x, y);
                    Color g = lerp(palette.glow, rgb(0xffd890), 0.35f);
                    r.rect(pane, g.withA(0.7f * fl * std::max(0.35f, palette.glowAmt)));
                    r.rect({pane.x + pane.w / 2 - 1, pane.y, 2, pane.h}, wall);
                    r.rect({pane.x, pane.y + pane.h / 2 - 1, pane.w, 2}, wall);
                    r.setBlend(Blend::Additive);
                    r.glow(pane.center(), TILE * 1.3f, palette.glow.withA(0.18f * fl));
                    r.setBlend(Blend::Alpha);
                }
                break;
            }
            case Tile::OneWay: {
                r.rect({px - 1, py, TILE + 2, 9}, wood);
                r.rect({px - 1, py - 1, TILE + 2, 1.5f}, rim.withA(0.4f));
                if (x % 3 == 0) {
                    r.tri({px + 4, py + 9}, {px + 12, py + 9}, {px + 6, py + 22}, wood);
                }
                break;
            }
            case Tile::Spikes: {
                for (int k = 0; k < 4; k++) {
                    float sx = px + 6 + k * 11 + hashf(x, k) * 3;
                    float h = 24 + hashf(x * 3, k) * 14;
                    float lean = (hashf(x, k * 7) - 0.5f) * 8;
                    r.tri({sx - 4, py + TILE}, {sx + 4, py + TILE}, {sx + lean, py + TILE - h}, palette.nearC);
                    r.tri({sx + lean * 0.8f - 1.6f, py + TILE - h + 7}, {sx + lean * 0.8f + 1.6f, py + TILE - h + 7}, {sx + lean, py + TILE - h}, rgb(0x6e1018, 0.9f));
                }
                break;
            }
            case Tile::Breakable: {
                Color pole = lerp(palette.nearC, palette.grass, 0.5f) * 1.6f;
                for (int k = 0; k < 4; k++) {
                    float sx = px + 5 + k * 11;
                    r.rect({sx, py, 7, TILE + 0.5f}, pole);
                    r.rect({sx + 1, py + 6 + (k % 2) * 6, 5, 2}, ground);
                    r.rect({sx + 6, py, 1, TILE}, rim.withA(0.2f));
                }
                r.rect({px, py + 14, TILE, 3}, rgb(0x4a3a20, 0.8f));
                r.rect({px, py + 34, TILE, 3}, rgb(0x4a3a20, 0.8f));
                break;
            }
            case Tile::Broken: {
                if (level.solidAt(x, y + 1)) {
                    Color pole = lerp(palette.nearC, palette.grass, 0.5f) * 1.4f;
                    r.line({px + 4, py + TILE - 3}, {px + 30, py + TILE - 8}, 4, pole);
                    r.line({px + 18, py + TILE - 2}, {px + 44, py + TILE - 4}, 4, pole);
                }
                break;
            }
            default: break;
            }
        }
}

void World::drawGrass(Renderer& r, const Rect& view, bool front) {
    int x0 = std::max(0, (int)std::floor(view.x / TILE) - 1), x1 = std::min(level.width - 1, (int)std::floor(view.right() / TILE) + 1);
    int y0 = std::max(0, (int)std::floor(view.y / TILE)), y1 = std::min(level.height - 1, (int)std::floor(view.bottom() / TILE));
    Color c = front ? palette.grass * 0.6f : lerp(palette.grass, palette.mid, 0.4f);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            if (!level.grassAt(x, y)) continue;
            float base = (y + 1) * TILE;
            int n = front ? 9 : 6;
            for (int b = 0; b < n; b++) {
                float bx = x * TILE + hashf(x * 13 + b, y + (front ? 99 : 0)) * TILE;
                float h = (front ? 56 : 70) + hashf(x + b, y * 3) * 36;
                float sway = std::sin(time * 1.4f + bx * 0.02f) * 6;
                float dpx = bx - player.pos.x;
                if (std::abs(dpx) < 40 && std::abs(base - player.pos.y) < 30) sway += sign(dpx) * (40 - std::abs(dpx)) * 0.5f;
                Vec2 a{bx - 3, base}, b2{bx + 3, base}, tip{bx + sway + (hashf(b, x) - 0.5f) * 14, base - h};
                r.tri(a, b2, tip, c.withA(front ? 0.92f : 1.f));
                // seed heads catch the rim light
                if (!front && b % 3 == 0) r.line(tip, tip + Vec2{2, 8}, 2, palette.rim.withA(0.25f));
            }
        }
}

// =================================================================== props

static void drawFlame(Renderer& r, Vec2 base, float size, float t, int seed) {
    r.setBlend(Blend::Additive);
    for (int i = 0; i < 6; i++) {
        float ph = t * (5 + i) + seed * 1.7f + i;
        float h = size * (0.6f + 0.5f * noise1(ph, seed + i));
        float w = size * 0.22f * (1 - i * 0.08f);
        float ox = (hashf(seed, i) - 0.5f) * size * 0.6f;
        float sway = std::sin(ph * 1.3f) * size * 0.12f;
        Color c = lerp(rgb(0xff4a12), rgb(0xffc060), i / 6.f);
        r.tri(base + Vec2{ox - w, 0}, base + Vec2{ox + w, 0}, base + Vec2{ox + sway, -h}, c.withA(0.55f));
    }
    r.glow(base + Vec2{0, -size * 0.3f}, size * 1.6f, rgb(0xff6a20, 0.5f));
    r.setBlend(Blend::Alpha);
}

void World::drawProps(Renderer& r, const Rect& view, bool front) {
    Color ink = palette.nearC * 0.9f;
    Color rim = palette.rim;
    for (auto& pr : level.props) {
        bool isFront = pr.params.count("front") > 0;
        if (isFront != front) continue;
        float wpx = pr.w * TILE, hpx = pr.h * TILE;
        if (pr.pos.x + wpx + 400 < view.x || pr.pos.x - 400 > view.right()) continue;
        Vec2 b = pr.pos;   // bottom-left
        Rng rng(pr.seed);
        const std::string& k = pr.kind;
        if (k == "house") {
            bool burning = pr.params.count("burning") > 0;
            Color wall = lerp(ink, rgb(0x1a120c), 0.4f);
            r.rect({b.x, b.y - hpx, wpx, hpx}, wall);
            for (float x = b.x; x <= b.x + wpx; x += TILE) r.rect({x - 3, b.y - hpx, 6, hpx}, ink);
            r.rect({b.x, b.y - hpx * 0.55f, wpx, 5}, ink);
            // shoji panels glowing from inside
            int panels = (int)(pr.w);
            for (int i = 0; i < panels; i++) {
                if (hashf(pr.seed, i) < 0.35f) continue;
                Rect pane{b.x + i * TILE + 8, b.y - hpx * 0.5f + 6, TILE - 16, hpx * 0.32f};
                float fl = burning ? 0.6f + 0.4f * noise1(time * 4 + i, pr.seed) : 0.85f;
                Color glowC = burning ? rgb(0xff7a30) : rgb(0xffcf8a);
                r.rect(pane, glowC.withA(0.75f * fl));
                for (int g = 1; g < 3; g++) r.rect({pane.x + pane.w * g / 3 - 1, pane.y, 2, pane.h}, wall);
                r.rect({pane.x, pane.y + pane.h / 2 - 1, pane.w, 2}, wall);
                r.setBlend(Blend::Additive);
                r.glow(pane.center(), TILE * 1.4f, glowC.withA(0.25f * fl));
                r.setBlend(Blend::Alpha);
            }
            // irimoya roof with upturned eaves
            float over = TILE * 0.8f, roofH = hpx * 0.6f;
            float yb = b.y - hpx;
            int n = 10;
            for (int i = 0; i < n; i++) {
                float t0 = i / (float)n, t1 = (i + 1) / (float)n;
                auto edge = [&](float t, float side) {
                    float curve = std::pow(1 - t, 2.4f) * TILE * 0.5f;
                    float hw = (wpx * 0.5f + over) * (1 - t * 0.55f);
                    return Vec2{b.x + wpx * 0.5f + side * hw, yb - roofH * t - curve};
                };
                r.quad(edge(t0, -1), edge(t0, 1), edge(t1, 1), edge(t1, -1), ink);
            }
            Vec2 eL{b.x - over, yb - TILE * 0.5f}, eR{b.x + wpx + over, yb - TILE * 0.5f};
            r.line(eL + Vec2{0, -1}, eR + Vec2{0, -1}, 1.5f, rim.withA(0.25f));
            r.rect({b.x + wpx * 0.5f - wpx * 0.3f, yb - roofH - 8, wpx * 0.6f, 8}, ink);
            r.line({b.x + wpx * 0.2f, yb - roofH - 8}, {b.x + wpx * 0.8f, yb - roofH - 8}, 1.5f, rim.withA(0.35f));
            if (burning) {
                for (int f = 0; f < 3; f++) drawFlame(r, {b.x + wpx * (0.25f + f * 0.25f), yb - roofH * 0.5f}, 70, time, pr.seed + f);
                if (rng.chance(0.5f) && hashf((int)(time * 30), pr.seed) < 0.3f)
                    emit(PType::Ember, {b.x + wpx * hashf((int)(time * 60), 3), yb - roofH * 0.6f}, 1, 120, rgb(0xffa040), 3, 1.5f, 1, -PI / 2);
            }
        } else if (k == "lantern") {
            Vec2 c{b.x + TILE * 0.5f, b.y};
            r.rect({c.x - 14, c.y - 10, 28, 10}, ink);
            r.rect({c.x - 5, c.y - 48, 10, 40}, ink);
            r.rect({c.x - 16, c.y - 54, 32, 8}, ink);
            float fl = 0.8f + 0.2f * noise1(time * 6, pr.seed);
            r.rect({c.x - 11, c.y - 76, 22, 22}, ink);
            r.rect({c.x - 7, c.y - 72, 14, 14}, rgb(0xffc070, fl));
            Vec2 roofPts[4] = {{c.x - 22, c.y - 76}, {c.x + 22, c.y - 76}, {c.x + 8, c.y - 90}, {c.x - 8, c.y - 90}};
            r.polygon(roofPts, 4, ink);
            r.circle({c.x, c.y - 93}, 4, ink);
            r.line({c.x - 22, c.y - 77}, {c.x + 22, c.y - 77}, 1.5f, rim.withA(0.3f));
        } else if (k == "hanging_lantern") {
            Vec2 c{b.x + TILE * 0.5f, b.y - TILE * 0.6f};
            float sw = std::sin(time * 1.3f + pr.seed) * 3;
            r.line({c.x, b.y - TILE}, {c.x + sw, c.y - 16}, 1.5f, ink);
            float fl = 0.8f + 0.2f * noise1(time * 5, pr.seed);
            Vec2 lc = c + Vec2{sw, 0};
            r.circle(lc, 15, rgb(0xd0401a, fl));
            for (int i = -2; i <= 2; i++) r.rect({lc.x - 15 * std::sqrt(1 - i * i / 6.f), lc.y + i * 5 - 0.6f, 30 * std::sqrt(1 - i * i / 6.f), 1.2f}, rgb(0x501008, 0.6f));
            r.rect({lc.x - 8, lc.y - 18, 16, 4}, ink);
            r.rect({lc.x - 8, lc.y + 14, 16, 4}, ink);
        } else if (k == "fire") {
            Vec2 c{b.x + TILE * 0.5f, b.y};
            r.line(c + Vec2{-26, -4}, c + Vec2{22, -12}, 9, ink);
            r.line(c + Vec2{-20, -12}, c + Vec2{26, -3}, 8, ink);
            drawFlame(r, c + Vec2{0, -8}, 80 * (pr.params.count("big") ? 1.8f : 1.f), time, pr.seed);
            if (hashf((int)(time * 40), pr.seed) < 0.25f) emit(PType::Ember, c + Vec2{rng.range(-20, 20), -40}, 1, 140, rgb(0xffa040), 3, 1.6f, 0.8f, -PI / 2);
            if (hashf((int)(time * 20), pr.seed + 1) < 0.15f) {
                Particle s; s.type = PType::Smoke; s.pos = c + Vec2{0, -90}; s.vel = {palette.wind.x * 0.5f + 10, -60};
                s.life = s.maxLife = 4; s.size = 40; s.col = rgb(0x120a08, 0.35f); s.gravity = -10; s.drag = 0.2f;
                particles.push_back(s);
            }
        } else if (k == "bamboo") {
            int n = pr.params.count("n") ? std::stoi(pr.params.at("n")) : 5;
            for (int i = 0; i < n; i++) {
                float x = b.x + rng.range(0, wpx > TILE ? wpx : TILE * 3);
                float h = rng.range(8, 16) * TILE;
                float w = rng.range(7, 11);
                float sway = std::sin(time * 0.9f + x * 0.01f) * 8;
                Vec2 a{x, b.y}, t{x + sway, b.y - h};
                r.quad(a + Vec2{-w / 2, 0}, a + Vec2{w / 2, 0}, t + Vec2{w * 0.4f, 0}, t + Vec2{-w * 0.4f, 0}, ink);
                r.lineGrad(a + Vec2{w / 2 - 1, 0}, t + Vec2{w * 0.4f - 1, 0}, 1.2f, 1.2f, rim.withA(0.2f), rim.withA(0.05f));
                for (float k2 = rng.range(0.1f, 0.2f); k2 < 1; k2 += rng.range(0.12f, 0.18f)) {
                    Vec2 p = lerp(a, t, k2);
                    r.rect({p.x - w * 0.7f, p.y - 1.5f, w * 1.4f, 3}, ink);
                }
            }
        } else if (k == "pine" || k == "maple") {
            // drawn from the background builder for consistency
            BgLayer tmp; BgBuilder B{tmp};
            Rng rr(pr.seed);
            if (k == "pine") B.pine(b.x + TILE * 0.5f, b.y, pr.h * TILE, rr);
            else B.maple(b.x + TILE * 0.5f, b.y, pr.h * TILE, rr);
            Color c = k == "maple" ? lerp(ink, rgb(0x4a070c), 0.55f) : ink;
            for (size_t i = 0; i + 2 < tmp.tris.size(); i += 3) r.tri(tmp.tris[i].p, tmp.tris[i + 1].p, tmp.tris[i + 2].p, c * tmp.tris[i].shade);
            if (palette.snow > 0.05f && k == "pine") {
                // snow resting on the needle pads
                for (size_t i = 0; i + 2 < tmp.tris.size(); i += 3) {
                    Vec2 top = tmp.tris[i].p;
                    if (rng.chance(0.06f)) r.circle(top + Vec2{0, -2}, 5 + rng.range(0, 6), rgb(0xe8eef8, 0.8f));
                }
            }
        } else if (k == "boat") {
            // a flat-bottomed skiff, rocking on the water
            Vec2 c{b.x + wpx * 0.5f, b.y - TILE * 0.3f + std::sin(time * 1.4f + b.x) * 2};
            float rot = std::sin(time * 1.1f + b.x) * 0.03f;
            Vec2 d = rotate({wpx * 0.5f, 0}, rot);
            Vec2 pts[4] = {c - d + Vec2{-12, -16}, c + d + Vec2{12, -16}, c + d * 0.85f + Vec2{0, 8}, c - d * 0.85f + Vec2{0, 8}};
            r.polygon(pts, 4, lerp(ink, rgb(0x2a1a10), 0.4f));
            r.line(pts[0], pts[1], 1.5f, rim.withA(0.3f));
            if (pr.params.count("pole")) r.line(c + Vec2{wpx * 0.2f, -16}, c + Vec2{wpx * 0.4f, -160}, 3, ink);
        } else if (k == "paddy") {
            // rice stalks standing out of the flood
            for (int i = 0; i < (int)(pr.w * 5); i++) {
                float x = b.x + rng.range(0, wpx), h = rng.range(16, 34);
                float sway = std::sin(time * 1.6f + x * 0.05f) * 3;
                r.line({x, b.y - TILE * 0.35f}, {x + sway, b.y - TILE * 0.35f - h}, 1.4f, lerp(ink, rgb(0x3a4a20), 0.4f));
            }
        } else if (k == "stilts") {
            for (float x = b.x + 6; x < b.x + wpx; x += 40) r.rect({x, b.y - hpx, 7, hpx + TILE}, ink);
            r.rect({b.x, b.y - hpx, wpx, 6}, ink);
        } else if (k == "tower") {
            // a castle corner tower: stone base, white plaster, dark roof
            float cx = b.x + wpx * 0.5f;
            r.quad({b.x - 10, b.y}, {b.x + wpx + 10, b.y}, {b.x + wpx - 6, b.y - hpx * 0.4f}, {b.x + 6, b.y - hpx * 0.4f}, lerp(ink, rgb(0x2a2a30), 0.5f));
            r.rect({b.x + 6, b.y - hpx, wpx - 12, hpx * 0.6f}, lerp(ink, rgb(0xb8bcc8), 0.18f));
            Vec2 roofPts[4] = {{b.x - 30, b.y - hpx}, {b.x + wpx + 30, b.y - hpx}, {cx + wpx * 0.25f, b.y - hpx - 50}, {cx - wpx * 0.25f, b.y - hpx - 50}};
            r.polygon(roofPts, 4, ink);
            if (palette.snow > 0.05f) r.line(roofPts[3], roofPts[2], 6, rgb(0xe8eef8, 0.85f));
            r.rect({cx - 8, b.y - hpx * 0.75f, 16, 22}, rgb(0x1a1410));
        } else if (k == "loom") {
            // a beam of the great loom, threads running up out of sight
            Color wood = lerp(ink, rgb(0x3a2a1a), 0.5f);
            r.rect({b.x, b.y - hpx, wpx, 18}, wood);
            r.rect({b.x, b.y - 18, wpx, 18}, wood);
            r.setBlend(Blend::Additive);
            for (float x = b.x + 10; x < b.x + wpx; x += 14) {
                float w = std::sin(time * 1.5f + x * 0.05f) * 3;
                r.line({x, b.y - 18}, {x + w, b.y - hpx + 18}, 1, palette.thread.withA(0.35f));
            }
            r.setBlend(Blend::Alpha);
        } else if (k == "thread") {
            // a single taut thread of the Veil rising into the dark
            r.setBlend(Blend::Additive);
            float w = std::sin(time * 2 + b.x) * 4;
            r.line({b.x + TILE * 0.5f, b.y}, {b.x + TILE * 0.5f + w, b.y - hpx}, 1.6f, palette.thread.withA(0.6f));
            r.glow({b.x + TILE * 0.5f, b.y - 10}, 30, palette.thread.withA(0.2f));
            r.setBlend(Blend::Alpha);
        } else if (k == "statue") {
            // a plain stone post
            Vec2 c{b.x + TILE * 0.5f, b.y};
            r.rect({c.x - 14, c.y - 8, 28, 8}, ink);
            r.rect({c.x - 10, c.y - 44, 20, 36}, ink);
            r.rect({c.x - 14, c.y - 48, 28, 5}, ink);
            r.line({c.x + 8, c.y - 42}, {c.x + 8, c.y - 12}, 1.5f, rim.withA(0.25f));
        } else if (k == "banner") {
            Vec2 c{b.x + TILE * 0.5f, b.y};
            float h = pr.h * TILE;
            r.rect({c.x - 2, c.y - h, 4, h}, ink);
            r.rect({c.x - 2, c.y - h, 40, 3}, ink);
            int n = 8;
            for (int i = 0; i < n; i++) {
                float t0 = i / (float)n, t1 = (i + 1) / (float)n;
                float w0 = std::sin(time * 2 + t0 * 5) * 4 * t0, w1 = std::sin(time * 2 + t1 * 5) * 4 * t1;
                float y0 = c.y - h + 3 + t0 * h * 0.6f, y1 = c.y - h + 3 + t1 * h * 0.6f;
                Color cc = pr.params.count("burnt") ? lerp(ink, rgb(0x3a2a20), 0.4f) : lerp(ink, rgb(0x1e2a5a), 0.5f);
                r.quad({c.x + 2 + w0, y0}, {c.x + 38 + w0, y0}, {c.x + 38 + w1, y1}, {c.x + 2 + w1, y1}, cc);
            }
            // the Kagenui crest: a needle through a ring
            Vec2 crest{c.x + 20, c.y - h + h * 0.25f};
            r.ring(crest, 7, 9, rim.withA(0.45f));
            r.line(crest + Vec2{-10, 8}, crest + Vec2{10, -8}, 1.5f, rim.withA(0.45f));
        } else if (k == "fence") {
            for (float x = b.x; x < b.x + wpx; x += 16) r.rect({x, b.y - 40, 6, 40}, ink);
            r.rect({b.x, b.y - 32, wpx, 4}, ink);
            r.rect({b.x, b.y - 14, wpx, 4}, ink);
        } else if (k == "cart") {
            r.rect({b.x, b.y - 46, wpx, 18}, ink);
            r.circle({b.x + 24, b.y - 22}, 22, ink);
            r.ring({b.x + 24, b.y - 22}, 18, 20, rim.withA(0.2f));
            r.line({b.x + wpx, b.y - 40}, {b.x + wpx + 60, b.y - 10}, 5, ink);
        } else if (k == "rocks") {
            for (int i = 0; i < (int)pr.w * 2; i++) {
                float x = b.x + rng.range(0, wpx), s = rng.range(12, 30);
                r.circle({x, b.y - s * 0.4f}, s, ink);
                r.ring({x, b.y - s * 0.4f}, s - 1.5f, s, rim.withA(0.15f), -PI * 0.9f, -PI * 0.3f);
            }
        } else if (k == "toy") {
            // a child's spinning top, dropped in the street
            Vec2 c{b.x + TILE * 0.5f, b.y - 8};
            Vec2 pts[4] = {{c.x - 10, c.y - 4}, {c.x + 10, c.y - 4}, {c.x + 2, c.y + 8}, {c.x - 2, c.y + 8}};
            r.polygon(pts, 4, rgb(0x6a1a14));
            r.rect({c.x - 1.5f, c.y - 12, 3, 8}, ink);
            r.rect({c.x - 10, c.y - 5, 20, 2}, rgb(0xc9a040, 0.8f));
        } else if (k == "gate_arch") {
            // the Old Gate: a fortified timber gatehouse with a tiled roof, framing the boss arena
            float cx = b.x + TILE * 0.5f, h = hpx, w = wpx;
            Color c = lerp(ink, rgb(0x3a1a12), 0.45f);
            Color stone = lerp(ink, rgb(0x2a2626), 0.5f);
            for (float sx : {cx - w / 2 - 40, cx + w / 2 - 20}) {
                r.quad({sx - 18, b.y}, {sx + 78, b.y}, {sx + 66, b.y - 90}, {sx - 6, b.y - 90}, stone);
                r.rect({sx + 6, b.y - h, 48, h - 80}, c);
                r.rect({sx + 6, b.y - h, 6, h - 80}, rim.withA(0.12f));
            }
            r.rect({cx - w / 2 - 50, b.y - h - 30, w + 100, 34}, c);
            for (float x = cx - w / 2 - 30; x < cx + w / 2 + 40; x += 64) r.rect({x, b.y - h - 30, 10, 34}, ink);
            // roof with flared eaves
            Vec2 roofPts[6] = {{cx - w / 2 - 130, b.y - h - 22}, {cx + w / 2 + 130, b.y - h - 22}, {cx + w / 2 + 70, b.y - h - 70},
                               {cx + w * 0.18f, b.y - h - 120}, {cx - w * 0.18f, b.y - h - 120}, {cx - w / 2 - 70, b.y - h - 70}};
            r.polygon(roofPts, 6, ink);
            r.line(roofPts[0], roofPts[5], 3, rim.withA(0.35f));
            r.line(roofPts[5], roofPts[4], 3, rim.withA(0.35f));
            r.line(roofPts[4], roofPts[3], 3, rim.withA(0.35f));
        }
    }
}

// =================================================================== mechanisms

void World::drawMechs(Renderer& r) {
    Color ink = palette.nearC * 0.9f, rim = palette.rim;
    Color gold = rgb(0xd8b45a);
    for (auto& m : mechs) {
        switch (m.kind) {
        case MechKind::Gate: {
            float hp = m.h * TILE;
            float top = m.pos.y - hp;
            float lift = hp * ease::inOutCubic(m.t);
            Rect g{m.pos.x - TILE * 0.5f, top - lift, TILE, hp};
            Color wood = lerp(ink, rgb(0x2a160c), 0.5f);
            for (int i = 0; i < 3; i++) r.rect({g.x + 4 + i * 15.f, g.y, 10, g.h}, wood);
            for (float y = g.y + 20; y < g.bottom(); y += 52) r.rect({g.x, y, g.w, 7}, rgb(0x1a1a1e));
            r.rect({g.x + g.w - 2, g.y, 2, g.h}, rim.withA(0.25f));
            r.rect({g.x - 14, top - 18, g.w + 28, 18}, ink);
            break;
        }
        case MechKind::Bell: {
            // the iron catch: a counterweight hanging from a beam; a thrown star knocks the pin loose
            Vec2 pivot = m.pos + Vec2{0, -TILE * 0.5f};
            r.rect({pivot.x - 50, pivot.y - 8, 100, 10}, ink);
            float a = m.swing;
            auto R = [&](Vec2 v) { return pivot + rotate(v, -a); };
            Color iron = lerp(ink, rgb(0x30343a), 0.6f);
            r.line(R({0, 0}), R({0, 22}), 3, iron * 0.8f);
            Vec2 pts[4] = {R({-20, 22}), R({20, 22}), R({24, 66}), R({-24, 66})};
            r.polygon(pts, 4, iron);
            r.line(R({-20, 34}), R({20, 34}), 2, iron * 0.6f);
            r.line(R({-22, 52}), R({22, 52}), 2, iron * 0.6f);
            r.line(R({18, 24}), R({22, 64}), 2, rim.withA(0.4f));
            if (!m.active) { r.circle(R({0, 22}), 4, gold); r.setBlend(Blend::Additive); r.glow(R({0, 22}), 26, gold.withA(0.25f + 0.1f * std::sin(time * 3))); r.setBlend(Blend::Alpha); }
            break;
        }
        case MechKind::Rope: {
            float top = m.pos.y - m.h * TILE;
            Color rope = rgb(0x5a4a30);
            if (!m.active) {
                r.line({m.pos.x, top}, {m.pos.x, m.pos.y}, 4, rope);
                for (float y = top + 6; y < m.pos.y; y += 10) r.line({m.pos.x - 2, y}, {m.pos.x + 2, y + 5}, 1.2f, rope * 0.6f);
                r.setBlend(Blend::Additive); r.glow({m.pos.x, (top + m.pos.y) / 2}, 30 + 6 * std::sin(time * 3), gold.withA(0.18f)); r.setBlend(Blend::Alpha);
            } else {
                r.line({m.pos.x, top}, {m.pos.x + 6, top + TILE}, 4, rope);
            }
            break;
        }
        case MechKind::Bridge: {
            float ang = (1 - ease::outCubic(m.t)) * (PI / 2);
            Vec2 pivot{m.pos.x, m.pos.y + 12};
            Vec2 dir{std::cos(-ang), std::sin(-ang)};
            Vec2 n = dir.perp();
            float len = m.w * TILE;
            Color wood = lerp(ink, rgb(0x3a2412), 0.5f);
            r.quad(pivot + n * 12, pivot + dir * len + n * 12, pivot + dir * len - n * 12, pivot - n * 12, wood);
            for (float s = 20; s < len; s += 40) r.line(pivot + dir * s + n * 13, pivot + dir * s - n * 13, 3, rgb(0x1a1008));
            r.line(pivot + n * -12, pivot + dir * len + n * -12, 1.5f, rim.withA(0.35f));
            break;
        }
        case MechKind::Chest: {
            Vec2 c = m.pos;
            Color lacquer = lerp(ink, rgb(0x4a0a10), 0.6f);
            r.rect({c.x - 24, c.y - 30, 48, 30}, lacquer);
            r.rect({c.x - 24, c.y - 30, 48, 3}, gold.withA(0.7f));
            r.rect({c.x - 3, c.y - 22, 6, 8}, gold.withA(0.8f));
            float lid = ease::outBack(m.t) * 1.6f;
            Vec2 hinge{c.x - 26, c.y - 30};
            Vec2 a = hinge + rotate(Vec2{0, 0}, -lid), b = hinge + rotate(Vec2{52, 0}, -lid), cc = hinge + rotate(Vec2{52, -12}, -lid), d = hinge + rotate(Vec2{0, -12}, -lid);
            r.quad(a, b, cc, d, lacquer);
            if (!m.active) { r.setBlend(Blend::Additive); r.glow(c + Vec2{0, -20}, 60 + 8 * std::sin(time * 2.5f), gold.withA(0.22f)); r.setBlend(Blend::Alpha); }
            break;
        }
        case MechKind::Scroll: {
            Vec2 c = m.pos;
            r.rect({c.x - 3, c.y - 34, 6, 34}, ink);
            r.rect({c.x - 14, c.y - 38, 28, 4}, ink);
            r.rect({c.x - 10, c.y - 58, 20, 20}, rgb(0xd8ccb0, 0.9f));
            r.rect({c.x - 12, c.y - 60, 24, 3}, ink); r.rect({c.x - 12, c.y - 39, 24, 3}, ink);
            r.setBlend(Blend::Additive);
            r.glow(c + Vec2{0, -48}, 50 + 8 * std::sin(time * 3), gold.withA(0.3f));
            r.setBlend(Blend::Alpha);
            break;
        }
        case MechKind::Shrine: {
            Vec2 c = m.pos;
            Color wood = lerp(ink, rgb(0x3a1c10), 0.4f);
            r.rect({c.x - 34, c.y - 12, 68, 12}, ink);
            r.rect({c.x - 24, c.y - 64, 48, 52}, wood);
            r.rect({c.x - 14, c.y - 54, 28, 30}, rgb(0xffd27a, m.active ? 0.85f : 0.35f));
            // flat canvas awning over a wayside rest
            Vec2 roofPts[4] = {{c.x - 42, c.y - 64}, {c.x + 42, c.y - 64}, {c.x + 32, c.y - 82}, {c.x - 32, c.y - 82}};
            r.polygon(roofPts, 4, ink);
            r.line({c.x - 42, c.y - 65}, {c.x + 42, c.y - 65}, 1.5f, rim.withA(0.4f));
            r.setBlend(Blend::Additive);
            float pulse = 0.8f + 0.2f * std::sin(time * 1.5f);
            r.glow(c + Vec2{0, -40}, (m.active ? 150 : 90) * pulse, rgb(0xffc860, m.active ? 0.4f : 0.2f));
            r.setBlend(Blend::Alpha);
            if (m.active && hashf((int)(time * 10), 77) < 0.1f) {
                Particle s; s.type = PType::Smoke; s.pos = c + Vec2{30, -20}; s.vel = {6, -30};
                s.life = s.maxLife = 3; s.size = 10; s.col = rgb(0xc8c0b0, 0.15f); s.gravity = -8;
                particles.push_back(s);
            }
            break;
        }
        case MechKind::Trial: {
            // a standing wind beacon carved with a spiral; it breathes teal until the trial is won
            Vec2 c = m.pos;
            Color stone = lerp(ink, rgb(0x2a3a3c), 0.5f);
            Vec2 pts[6] = {{c.x - 26, c.y}, {c.x + 28, c.y}, {c.x + 22, c.y - 90}, {c.x + 8, c.y - 128}, {c.x - 12, c.y - 120}, {c.x - 24, c.y - 70}};
            r.polygon(pts, 6, stone);
            r.line(pts[2], pts[3], 1.5f, rim.withA(0.4f));
            r.rect({c.x - 34, c.y - 8, 68, 8}, ink);
            float pulse = m.active ? 0.6f : 0.55f + 0.45f * std::sin(time * 2.4f);
            Color teal = rgb(0x9ff0e0);
            for (int k = 0; k < 3; k++) r.ring(c + Vec2{0, -66}, 6 + k * 7.f, 8 + k * 7.f, teal.withA(0.55f * pulse), time * 0.6f + k * 2.1f, time * 0.6f + k * 2.1f + 4.2f);
            r.setBlend(Blend::Additive);
            r.glow(c + Vec2{0, -66}, 90 + 20 * pulse, teal.withA((m.active ? 0.15f : 0.3f) * pulse));
            r.setBlend(Blend::Alpha);
            if (!m.active && hashf((int)(time * 20), 41) < 0.3f) {
                Particle q; q.type = PType::Thread; q.pos = c + Vec2{hashf((int)(time * 60), 1) * 80 - 40, -20 - hashf((int)(time * 50), 2) * 100};
                q.vel = {40, -30}; q.life = q.maxLife = 0.8f; q.size = 1.5f; q.col = teal.withA(0.8f);
                particles.push_back(q);
            }
            break;
        }
        case MechKind::Sluice: {
            // a sluice wheel on a timber post
            Vec2 c = m.pos + Vec2{0, -70};
            r.rect({m.pos.x - 5, c.y, 10, 70}, ink);
            float ang = m.t * PI * 3;
            r.ring(c, 26, 31, ink);
            for (int k = 0; k < 6; k++) { Vec2 d = rotate({0, -28}, ang + k * PI / 3); r.line(c, c + d, 3, ink); r.circle(c + d * 1.18f, 4, ink); }
            r.circle(c, 6, gold.withA(m.active ? 0.3f : 0.8f));
            r.ring(c, 30, 31.5f, rim.withA(0.35f));
            if (!m.active) {
                float a = 0.5f + 0.5f * std::sin(time * 2.4f);
                r.setBlend(Blend::Additive); r.glow(c, 46, gold.withA(0.15f * a)); r.setBlend(Blend::Alpha);
            }
            break;
        }
        case MechKind::Beacon: {
            // a fire-watch brazier on a pole with a hooded lantern that sweeps the ground
            float H = m.h * TILE;
            Vec2 top = m.pos + Vec2{0, -H};
            r.rect({m.pos.x - 4, top.y, 8, H}, ink);
            r.quad(top + Vec2{-18, 0}, top + Vec2{18, 0}, top + Vec2{12, 16}, top + Vec2{-12, 16}, ink);
            if (m.active) {
                float ang = PI / 2 + std::sin(m.timer) * 0.75f;
                float len = m.range * TILE;
                Vec2 a = top + Vec2{std::cos(ang - 0.22f), std::sin(ang - 0.22f)} * len;
                Vec2 b = top + Vec2{std::cos(ang + 0.22f), std::sin(ang + 0.22f)} * len;
                r.setBlend(Blend::Additive);
                Color beam = beaconSpotted ? rgb(0xff5a3a) : rgb(0xffc070);
                r.tri(top, a, b, beam.withA(0.16f), beam.withA(0.0f), beam.withA(0.0f));
                r.glow(top, 60, beam.withA(0.5f));
                r.setBlend(Blend::Alpha);
                drawFlame(r, top, 28, time * 2, (int)m.pos.x & 7);
            } else {
                r.circle(top + Vec2{0, -4}, 6, rgb(0x303038));
            }
            break;
        }
        case MechKind::Examine: {
            float a = 0.5f + 0.5f * std::sin(time * 2.4f + m.pos.x);
            r.setBlend(Blend::Additive);
            r.glow(m.pos + Vec2{0, -30}, 22, gold.withA(0.35f * a));
            r.circle(m.pos + Vec2{0, -30}, 2.2f, gold.withA(0.8f * a));
            r.setBlend(Blend::Alpha);
            break;
        }
        default: break;
        }
    }
    // pickups
    for (auto& p : pickups) {
        Vec2 c = p.pos + Vec2{0, std::sin(p.bob * 3) * 4};
        r.setBlend(Blend::Additive);
        Color g = p.kind == "mon" ? gold : (p.kind == "bead" ? rgb(0xc8a8ff) : (p.kind == "shuriken" ? rgb(0xc8d8e8) : rgb(0xa8ffcf)));
        r.glow(c, 28, g.withA(0.35f));
        r.setBlend(Blend::Alpha);
        if (p.kind == "mon") { r.circle(c, 7, gold * 0.8f); r.rect({c.x - 2, c.y - 2, 4, 4}, ink); }
        else if (p.kind == "shuriken") { for (int k = 0; k < 4; k++) { Vec2 d = rotate({0, -9}, k * PI / 2 + time); r.tri(c + d, c + rotate(d, 0.6f) * 0.3f, c + rotate(d, -0.6f) * 0.3f, rgb(0xb8c0c8)); } }
        else if (p.kind == "bead") { for (int k = 0; k < 5; k++) r.circle(c + rotate({0, -7}, k * TAU / 5), 3, rgb(0x6a4a8a)); }
        else { r.circle(c, 6, g * 0.7f); }
    }
    // grapple anchors: brass rings on posts, glowing when in reach
    int best = (player.state != PState::Dead) ? findAnchor() : -1;
    for (size_t i = 0; i < anchors.size(); i++) {
        Vec2 a = anchors[i];
        r.ring(a, 7, 11, lerp(ink, rgb(0x8a7040), 0.7f));
        r.ring(a, 10, 11.5f, rim.withA(0.3f));
        if ((int)i == best && player.state != PState::Grapple) {
            r.setBlend(Blend::Additive);
            r.glow(a, 46 + 6 * std::sin(time * 6), rgb(0xd8c8ff, 0.45f));
            r.setBlend(Blend::Alpha);
        }
    }
}

// =================================================================== actors

static Vec2 catmull(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return (p1 * 2.f + (p2 - p0) * t + (p0 * 2.f - p1 * 5.f + p2 * 4.f - p3) * t2 + (p1 * 3.f - p0 - p2 * 3.f + p3) * t3) * 0.5f;
}

static void drawTrail(Renderer& r, const std::deque<TrailSample>& raw, float now, Color edge, float life) {
    if (raw.size() < 2) return;
    // smooth the sparse samples into a flowing arc
    std::deque<TrailSample> tr;
    for (size_t i = 0; i + 1 < raw.size(); i++) {
        const auto& a = raw[i ? i - 1 : 0]; const auto& b = raw[i]; const auto& c = raw[i + 1]; const auto& d = raw[std::min(raw.size() - 1, i + 2)];
        for (int k = 0; k < 4; k++) {
            float t = k / 4.f;
            tr.push_back({catmull(a.base, b.base, c.base, d.base, t), catmull(a.tip, b.tip, c.tip, d.tip, t), lerp(b.t, c.t, t)});
        }
    }
    tr.push_back(raw.back());
    r.setBlend(Blend::Additive);
    for (size_t i = 1; i < tr.size(); i++) {
        float a0 = clamp01(1 - (now - tr[i - 1].t) / life), a1 = clamp01(1 - (now - tr[i].t) / life);
        Color c0 = edge.withA(a0 * 0.5f), c1 = edge.withA(a1 * 0.5f);
        r.quad(tr[i - 1].base, tr[i].base, tr[i].tip, tr[i - 1].tip, c0.withA(0), c1.withA(0), c1, c0);
        // white-hot core along the edge
        Vec2 m0 = lerp(tr[i - 1].base, tr[i - 1].tip, 0.82f), m1 = lerp(tr[i].base, tr[i].tip, 0.82f);
        r.quad(m0, m1, tr[i].tip, tr[i - 1].tip, Color{1, 1, 1, a0 * 0.5f}, Color{1, 1, 1, a1 * 0.5f}, Color{1, 1, 1, a1 * 0.7f}, Color{1, 1, 1, a0 * 0.7f});
    }
    r.setBlend(Blend::Alpha);
}

static Vec2 lightDirFor(const Palette& P) { return Vec2{(P.sky.moonPos.x - 0.5f) * 2.f, -1.f}.norm(); }

void World::drawPlayer(Renderer& r, float alpha) {
    const Player& p = player;
    Vec2 pos = lerp(p.prevPos, p.pos, alpha);
    Joints j = solve(Rig{}, p.pose, pos, p.facing);
    RenderStyle st;
    st.body = rgb(0x0b0c14); st.bodyBack = rgb(0x06070c);
    st.rim = rimAt(pos + Vec2{0, -50});
    st.lightDir = lightDirFor(palette);
    if (p.hidden) st.alpha = 0.55f;
    if (p.hurtFlash > 0) { st.body = lerp(st.body, rgb(0x5a0a14), p.hurtFlash * 0.6f); }
    if (settings && settings->highContrast) { st.rimWidth = 2.6f; st.rim = lerp(st.rim, Color{1, 1, 1}, 0.4f); }
    // veil-step afterimages
    r.setBlend(Blend::Additive);
    for (auto& ai : p.afterimages) {
        RenderStyle gs = st;
        float a = ai.life / 0.28f;
        gs.body = rgb(0x6a4aff, 0.22f * a); gs.bodyBack = gs.body; gs.rim = rgb(0xd8c8ff, 0.3f * a); gs.blade = gs.rim;
        drawHumanoid(r, Rig{}, ai.j, gs, nullptr, false);
    }
    r.setBlend(Blend::Alpha);
    // Gale Stance: wind ribbons orbit Rei, streaming harder as she moves
    if (hasGale()) {
        r.setBlend(Blend::Additive);
        float speed = clamp01(std::abs(p.vel.x) / 600.f) + (p.state == PState::Dash ? 1.f : 0.f);
        Vec2 c = j.chest + Vec2{0, 6};
        for (int k = 0; k < 3; k++) {
            float a0 = time * (2.2f + k * 0.7f) + k * 2.1f;
            float rx = 34 + k * 9, ry = 46 + k * 6;
            Vec2 prev = c + Vec2{std::cos(a0) * rx, std::sin(a0) * ry * 0.55f};
            for (int s = 1; s <= 10; s++) {
                float a = a0 + s * 0.16f;
                Vec2 q = c + Vec2{std::cos(a) * rx - p.vel.x * 0.02f * s, std::sin(a) * ry * 0.55f};
                r.line(prev, q, 1.6f, rgb(0x9ff0e0, (0.10f + 0.18f * speed) * (s / 10.f) * st.alpha));
                prev = q;
            }
        }
        r.glow(c, 70, rgb(0x6fd8c8, (0.05f + 0.07f * speed) * st.alpha));
        r.setBlend(Blend::Alpha);
    }
    // Ember readiness: coals smoulder in the sword hand when Sparks suffice
    if (save->has("ember") && p.ki >= (save->skills.count("veil_well") ? 25.f : 35.f)) {
        r.setBlend(Blend::Additive);
        r.glow(j.handB, 16 + 4 * std::sin(time * 9), rgb(0xff7a2a, 0.35f * st.alpha));
        r.setBlend(Blend::Alpha);
    }
    // grapple thread
    if (p.state == PState::Grapple) {
        r.setBlend(Blend::Additive);
        float k = clamp01(p.ropeT / 0.06f);
        Vec2 tip = lerp(j.handB, p.anchor, k);
        r.line(j.handB, tip, 3, rgb(0xd8c8ff, 0.5f));
        r.line(j.handB, tip, 1.2f, rgb(0xffffff, 0.9f));
        r.glow(tip, 18, rgb(0xd8c8ff, 0.6f));
        r.setBlend(Blend::Alpha);
    }
    // scarf and headband tails behind the body
    Color crimson = rgb(0xa8101e, st.alpha);
    drawClothStrip(r, p.scarf, 9, 3, crimson, st.rim.mulA(0.8f), st.lightDir * 1.6f, 1.6f);
    drawClothStrip(r, p.tailA, 3.2f, 1.2f, st.body, st.rim, st.lightDir, 1.2f);
    drawClothStrip(r, p.tailB, 2.8f, 1.0f, st.body, st.rim, st.lightDir, 1.2f);
    bool sword = p.lockedPose != "kneel";
    drawHumanoid(r, Rig{}, j, st, [&](GearLayer layer, bool rimPass) {
        if (layer == GearLayer::Head && !rimPass) {
            // headband & the pale glint of watchful eyes
            Vec2 f = dirFromAngle(j.headAngle + PI);  // up from head
            Vec2 side{j.facing, 0};
            r.line(j.head - side * 8 + f * 2, j.head + side * 8 + f * 2, 3.2f, rgb(0x1a1a2a, st.alpha));
            r.line(j.head + side * 3 - f * 1, j.head + side * 7.5f - f * 1.5f, 1.4f, rgb(0xcfd8ff, 0.65f * st.alpha));
        }
        if (layer == GearLayer::Torso && !rimPass) {
            // crimson scarf wrap at the neck
            r.capsule(j.neck + Vec2{-j.facing * 3, 1}, j.neck + Vec2{j.facing * 4, 2}, 5, 5, crimson);
        }
    }, sword);
    drawTrail(r, p.trail, time, lerp(st.rim, Color{1, 1, 1}, 0.4f), 0.14f);
}

// Each foe reads at a glance from silhouette alone: headgear, cape and weapon carry the identity.
namespace {
enum class Hat { Jingasa, Hood, Kabuto, Topknot, Beak, Wolf, Blank, Bare, Old, WideKasa };
enum class Weapon { Sword, Spear, Bow, Sickle, Gun, Staff, Threads, Hook, Glaive };
enum class Cape { None, Mino, Feathers, Pelt, Threads, Banner, Robe };
struct Look { Hat hat; Weapon weapon; Cape cape; uint32_t tint; float tintAmt; uint32_t accent; };
Look lookOf(EType t) {
    switch (t) {
    case EType::Swordsman: return {Hat::Jingasa, Weapon::Sword, Cape::None, 0, 0, 0x3a0c10};
    case EType::Spearman: return {Hat::Jingasa, Weapon::Spear, Cape::Banner, 0, 0, 0x3a0c10};
    case EType::Archer: return {Hat::Bare, Weapon::Bow, Cape::None, 0, 0, 0x3a0c10};
    case EType::Tetsugen: return {Hat::Kabuto, Weapon::Sword, Cape::Banner, 0, 0, 0x3a0c10};
    case EType::Reaver: return {Hat::Jingasa, Weapon::Sickle, Cape::Mino, 0x2a2416, 0.35f, 0x4a3a1a};
    case EType::Lurker: return {Hat::Hood, Weapon::Spear, Cape::None, 0x0c2a30, 0.55f, 0x1a4a50};
    case EType::TideWarden: return {Hat::WideKasa, Weapon::Hook, Cape::Mino, 0x10282c, 0.45f, 0x2a6a7a};
    case EType::Duelist: return {Hat::Topknot, Weapon::Sword, Cape::None, 0x1a0c0c, 0.3f, 0x8a1a14};
    case EType::CrowScout: return {Hat::Beak, Weapon::Sword, Cape::Feathers, 0x101014, 0.2f, 0x2a2a34};
    case EType::CrowKing: return {Hat::Beak, Weapon::Sword, Cape::Feathers, 0x101014, 0.2f, 0xc06a20};
    case EType::Gunner: return {Hat::Jingasa, Weapon::Gun, Cape::None, 0x1a1a20, 0.3f, 0x3a3a48};
    case EType::Hakuro: return {Hat::Wolf, Weapon::Sword, Cape::Pelt, 0xc8ccd8, 0.55f, 0xdfe8ff};
    case EType::HollowBlade: return {Hat::Blank, Weapon::Sword, Cape::Threads, 0x1c1430, 0.6f, 0xb8a0ff};
    case EType::HollowSnare: return {Hat::Hood, Weapon::Threads, Cape::Threads, 0x1c1430, 0.6f, 0xb8a0ff};
    case EType::Kagami: return {Hat::Blank, Weapon::Sword, Cape::Robe, 0xd8d0f0, 0.55f, 0xe8dcff};
    case EType::Sentinel: return {Hat::Kabuto, Weapon::Glaive, Cape::Threads, 0x2a2212, 0.5f, 0xffd27a};
    case EType::Odaka: return {Hat::Old, Weapon::Staff, Cape::Robe, 0x1a1a24, 0.3f, 0xffd890};
    default: return {Hat::Jingasa, Weapon::Sword, Cape::None, 0, 0, 0x3a0c10};
    }
}
}

void World::drawHound(Renderer& r, const Enemy& e, float alpha) {
    Vec2 pos = lerp(e.prevPos, e.pos, alpha);
    float f = e.facing;
    float a = e.st == EState::Dead ? clamp01(1 - e.dissolve) : 1.f;
    Color body = rgb(0x0e0d0c, a);
    if (e.flash > 0) body = lerp(body, rgb(0x6a6a70, a), e.flash * 0.6f);
    Color rim = rimAt(pos + Vec2{0, -30}).mulA(a);
    float speed = std::abs(e.vel.x);
    float ph = time * (6 + speed * 0.03f) + e.id;
    float gait = std::min(1.f, speed / 200.f);
    bool lunge = e.st == EState::Attack, crouch = e.st == EState::Windup, dead = e.st == EState::Dead;
    float bodyY = pos.y - 30 + (crouch ? 6 : 0) + (dead ? 16 : 0);
    Vec2 hip{pos.x - f * 22, bodyY}, sh{pos.x + f * 18, bodyY - 4 + (lunge ? -6 : 0)};
    // legs: two pairs, opposite phase
    for (int k = 0; k < 4; k++) {
        bool front = k >= 2;
        Vec2 root = front ? sh : hip;
        float off = (k % 2 ? PI : 0) + (front ? PI * 0.5f : 0);
        float swing = dead ? 0.9f : std::sin(ph + off) * 0.6f * gait;
        Vec2 knee = root + Vec2{f * std::sin(swing) * 10 + (front ? 0 : -f * 4), 14};
        Vec2 paw = knee + Vec2{f * std::sin(swing + 0.4f) * 12, dead ? 4.f : 16.f - std::max(0.f, std::cos(ph + off)) * 5 * gait};
        Color lc = k % 2 ? body * 0.7f : body;
        r.line(root, knee, 5, lc);
        r.line(knee, paw, 4, lc);
    }
    r.capsule(hip, sh, 12, 13, body);
    r.line(hip + Vec2{0, -6}, sh + Vec2{0, -8}, 1.2f, rim.mulA(0.5f));
    // tail
    Vec2 t0 = hip + Vec2{-f * 8, -4}, t1 = t0 + Vec2{-f * 18, -10 + std::sin(time * 8) * 3};
    r.line(t0, t1, 4, body);
    // neck & head with snout and ears
    Vec2 head = sh + Vec2{f * 16, -14.f + (lunge ? 6.f : 0.f) + (crouch ? 8.f : 0.f)};
    r.line(sh, head, 9, body);
    r.circle(head, 9, body);
    r.quad(head + Vec2{f * 4, -4}, head + Vec2{f * 20, 0}, head + Vec2{f * 20, 5}, head + Vec2{f * 4, 6}, body);
    r.tri(head + Vec2{-f * 4, -6}, head + Vec2{f * 2, -7}, head + Vec2{-f * 3, -18}, body);
    // snow-white collar of Castle Hakuro's kennels
    r.line(sh + Vec2{f * 4, -12}, sh + Vec2{f * 10, 2}, 3, rgb(0xd8dce8, 0.8f * a));
    if (e.alive()) {
        Color eyeC = e.awareness >= 1 ? rgb(0xff3a2a) : (e.awareness > 0.3f ? rgb(0xffb040) : rgb(0x000000, 0));
        if (eyeC.a > 0) {
            r.setBlend(Blend::Additive);
            r.circle(head + Vec2{f * 6, -3}, 1.6f, eyeC);
            r.glow(head + Vec2{f * 6, -3}, 9, eyeC.withA(0.5f));
            r.setBlend(Blend::Alpha);
        }
        if (e.st == EState::Engage && e.awareness < 1 && e.def->sniffs && hashf((int)(time * 4), e.id) > 0.7f)
            r.circle(head + Vec2{f * 22, 0}, 2, rgb(0xffffff, 0.3f));
    }
}

void World::drawEnemy(Renderer& r, const Enemy& e, float alpha) {
    if (e.st == EState::Dead && e.dissolve >= 1) return;
    if (e.type == EType::Hound) { drawHound(r, e, alpha); return; }
    const EDef& D = *e.def;
    const Look L = lookOf(e.type);
    Vec2 pos = lerp(e.prevPos, e.pos, alpha);
    if (e.lurking) {
        // only a pair of eyes and a ring of ripples betray the lurker
        float k = 0.5f + 0.5f * std::sin(time * 2 + e.id);
        int wy = (int)std::floor((pos.y - 40) / TILE);
        float surface = pos.y - 40;
        while (wy > 0 && level.waterAt((int)std::floor(pos.x / TILE), wy - 1)) { wy--; surface = wy * (float)TILE; }
        r.ring({pos.x, surface + 4}, 14 + k * 10, 15.5f + k * 10, rgb(0xb8e8f0, 0.25f * (1 - k)));
        if (e.awareness > 0.2f) {
            r.setBlend(Blend::Additive);
            r.circle({pos.x - 5, surface + 8}, 1.5f, rgb(0xffb040, 0.8f));
            r.circle({pos.x + 5, surface + 8}, 1.5f, rgb(0xffb040, 0.8f));
            r.setBlend(Blend::Alpha);
        }
        return;
    }
    Rig rig; rig.scale = D.scale;
    if (e.type == EType::Tetsugen || e.type == EType::TideWarden || e.type == EType::Sentinel) { rig.shoulderW = 10; rig.chestR = 12.5f; rig.bladeLen = 92; rig.hipR = 9; }
    if (e.type == EType::Hakuro || e.type == EType::CrowKing) { rig.shoulderW = 8; rig.chestR = 11.5f; rig.bladeLen = 96; }
    if (e.type == EType::Duelist || e.type == EType::Kagami || e.type == EType::HollowBlade) rig.bladeLen = 84;
    if (e.type == EType::CrowScout) rig.bladeLen = 44;
    bool sword = L.weapon == Weapon::Sword;
    if (!sword) rig.bladeLen = 0;
    Joints j = solve(rig, e.pose, pos, e.facing);
    RenderStyle st;
    st.body = rgb(0x0f0d0c); st.bodyBack = rgb(0x080706);
    if (L.tintAmt > 0) { st.body = lerp(st.body, rgb(L.tint), L.tintAmt); st.bodyBack = lerp(st.bodyBack, rgb(L.tint), L.tintAmt * 0.7f); }
    st.rim = rimAt(pos + Vec2{0, -50});
    st.lightDir = lightDirFor(palette);
    st.blade = rgb(0xa0a4ac);
    if (e.type == EType::HollowBlade || e.type == EType::Kagami) st.blade = rgb(0xd8ccff);
    if (e.flash > 0) st.body = lerp(st.body, rgb(0x6a6a70), e.flash * 0.6f);
    if (e.chill > 0) st.body = lerp(st.body, rgb(0x6a9ab0), 0.35f);
    if (e.st == EState::Dead) st.alpha = clamp01(1 - e.dissolve);
    bool hollow = e.type == EType::HollowBlade || e.type == EType::HollowSnare;
    if (hollow) st.alpha *= 0.82f + 0.12f * std::sin(time * 3 + e.id);
    if (settings && settings->highContrast) { st.rimWidth = 3; st.rim = rgb(0xff6a5a); }
    const float s = D.scale;
    Color accent = rgb(L.accent);
    Color lacquer = lerp(st.body, accent, 0.7f).withA(st.alpha);
    bool burning = e.phase2 && e.type == EType::Tetsugen;
    bool empowered = e.phase2 && e.boss && !burning;
    Color sideCol = st.body.withA(st.alpha);
    // ---- back layer: banners, capes, wings
    if (L.cape == Cape::Banner) {
        Vec2 pole0 = j.chest + Vec2{-e.facing * 8 * s, 0}, pole1 = pole0 + Vec2{-e.facing * 4 * s, -54 * s};
        r.line(pole0, pole1, 2.5f * s, sideCol);
        drawClothStrip(r, e.sash, 14 * s, 8 * s, lerp(st.body, rgb(0x2a0a0c), 0.5f).withA(st.alpha), st.rim.mulA(st.alpha * 0.6f), st.lightDir, 1.4f);
    } else if (L.cape == Cape::Mino) {
        // straw rain-cape: a shaggy cone from the shoulders
        Vec2 top = j.chest + Vec2{0, -6 * s};
        for (int k = 0; k < 9; k++) {
            float u = (k - 4) / 4.f;
            Vec2 b = j.hip + Vec2{u * 20 * s - e.facing * 4 * s, 16 * s + std::abs(u) * -6 * s + std::sin(time * 3 + k) * 1.5f};
            r.tri(top + Vec2{u * 6 * s, 0}, top + Vec2{u * 6 * s + 5 * s, 0}, b, lerp(sideCol, rgb(0x4a3a1a, st.alpha), 0.35f));
        }
    } else if (L.cape == Cape::Feathers) {
        // a cloak of black feathers; the Crow King's flares like wings when he leaps
        float spread = e.type == EType::CrowKing ? (e.vel.y < -100 || e.st == EState::Windup ? 1.f : 0.45f) : 0.3f;
        for (float side : {-1.f, 1.f}) {
            Vec2 root = j.shoulderB + Vec2{0, 2 * s};
            for (int k = 0; k < 6; k++) {
                float ang = PI * 0.5f + side * (0.25f + k * 0.18f * (0.4f + spread)) - e.facing * 0.2f;
                float len = (46 + k * 6) * s * (0.6f + spread * 0.6f);
                Vec2 tip = root + Vec2{std::cos(ang), std::sin(ang)} * len;
                Vec2 n = (tip - root).norm().perp() * 5 * s;
                r.quad(root - n * 0.4f, root + n * 0.4f, tip + n * 0.2f, tip - n, sideCol);
            }
        }
    } else if (L.cape == Cape::Pelt) {
        // a white wolf's pelt over the shoulders, trailing behind
        drawClothStrip(r, e.sash, 22 * s, 10 * s, rgb(0xe8ecf4, st.alpha * 0.95f), st.rim.mulA(st.alpha * 0.5f), st.lightDir, 1.4f);
    } else if (L.cape == Cape::Robe) {
        Vec2 a = j.chest + Vec2{-e.facing * 6 * s, -4 * s};
        Vec2 b = pos + Vec2{-e.facing * (22 + 3 * std::sin(time * 2)) * s, -2};
        r.quad(a + Vec2{-6 * s, 0}, a + Vec2{6 * s, 0}, b + Vec2{8 * s, 0}, b + Vec2{-10 * s, 0}, lerp(sideCol, accent.withA(st.alpha), 0.15f));
    }
    if (L.cape == Cape::Threads || hollow || e.type == EType::Sentinel) {
        r.setBlend(Blend::Additive);
        for (int k = 0; k < 5; k++) {
            float u = hashf(k, e.id);
            Vec2 a = lerp(j.hip, j.chest, u) + Vec2{-e.facing * 6 * s, 0};
            Vec2 b = a + Vec2{-e.facing * (30 + 20 * u) * s, std::sin(time * 2.5f + k) * 10 * s + 10 * s};
            r.line(a, b, 1.1f, accent.withA(0.35f * st.alpha));
        }
        r.setBlend(Blend::Alpha);
    }
    drawHumanoid(r, rig, j, st, [&](GearLayer layer, bool rimPass) {
        Color c = rimPass ? st.rim.mulA(st.alpha) : st.body.withA(st.alpha);
        Vec2 o = rimPass ? st.lightDir * (st.rimWidth * s) : Vec2{0, 0};
        Vec2 side{e.facing, 0};
        if (layer == GearLayer::Head) {
            Vec2 hc = j.head + o;
            switch (L.hat) {
            case Hat::Jingasa: r.tri(hc + Vec2{-21 * s, -1 * s}, hc + Vec2{21 * s, -1 * s}, hc + Vec2{0, -14 * s}, c); break;
            case Hat::WideKasa: r.tri(hc + Vec2{-30 * s, 0}, hc + Vec2{30 * s, 0}, hc + Vec2{0, -16 * s}, c); r.line(hc + Vec2{-30 * s, 0}, hc + Vec2{30 * s, 0}, 2, c); break;
            case Hat::Hood:
                r.circle(hc + Vec2{-e.facing * 2 * s, -2 * s}, 12 * s, c);
                r.quad(hc + Vec2{-e.facing * 4 * s, 4 * s}, hc + Vec2{-e.facing * 12 * s, 2 * s}, hc + Vec2{-e.facing * 16 * s, 18 * s}, hc + Vec2{-e.facing * 2 * s, 14 * s}, c);
                break;
            case Hat::Kabuto:
                r.circle(hc + Vec2{0, -3 * s}, 11 * s, c);
                r.quad(hc + Vec2{-14 * s, 0}, hc + Vec2{14 * s, 0}, hc + Vec2{18 * s, 10 * s}, hc + Vec2{-18 * s, 10 * s}, c);
                if (!rimPass) r.ring(hc + Vec2{side.x * 4 * s, -16 * s}, 12 * s, 15 * s, (e.type == EType::Sentinel ? accent : rgb(0xc8a040)).withA(st.alpha), -PI * 0.95f, -PI * 0.05f);
                break;
            case Hat::Topknot:
                r.circle(hc + Vec2{-e.facing * 3 * s, -11 * s}, 4 * s, c);
                r.line(hc + Vec2{-e.facing * 3 * s, -12 * s}, hc + Vec2{e.facing * 4 * s, -15 * s}, 3 * s, c);
                break;
            case Hat::Beak:
                r.circle(hc + Vec2{0, -2 * s}, 11 * s, c);
                r.tri(hc + Vec2{e.facing * 6 * s, -5 * s}, hc + Vec2{e.facing * 6 * s, 3 * s}, hc + Vec2{e.facing * 24 * s, 2 * s}, c);
                if (e.type == EType::CrowKing) for (int k = 0; k < 4; k++) r.tri(hc + Vec2{(k - 1.5f) * 5 * s, -10 * s}, hc + Vec2{(k - 1.5f) * 5 * s + 4 * s, -10 * s}, hc + Vec2{(k - 1.5f) * 9 * s - e.facing * 6 * s, -30 * s}, c);
                break;
            case Hat::Wolf:
                // a wolf-head hood: muzzle forward, ears up
                r.circle(hc + Vec2{0, -4 * s}, 12 * s, c);
                r.quad(hc + Vec2{e.facing * 4 * s, -12 * s}, hc + Vec2{e.facing * 20 * s, -6 * s}, hc + Vec2{e.facing * 18 * s, -1 * s}, hc + Vec2{e.facing * 4 * s, -3 * s}, c);
                r.tri(hc + Vec2{-e.facing * 6 * s, -12 * s}, hc + Vec2{0, -13 * s}, hc + Vec2{-e.facing * 5 * s, -26 * s}, c);
                r.tri(hc + Vec2{e.facing * 2 * s, -14 * s}, hc + Vec2{e.facing * 8 * s, -13 * s}, hc + Vec2{e.facing * 4 * s, -27 * s}, c);
                break;
            case Hat::Blank:
                r.circle(hc + Vec2{0, -1 * s}, 10.5f * s, c);
                if (!rimPass) r.circle(hc + side * 3 * s, 7.5f * s, rgb(0xf0ecf8, 0.85f * st.alpha));   // a smooth white face-plate
                break;
            case Hat::Old:
                r.circle(hc + Vec2{0, -1 * s}, 10 * s, c);
                if (!rimPass) r.line(hc + Vec2{-10 * s, -5 * s}, hc + Vec2{10 * s, -5 * s}, 2.5f * s, accent.withA(0.8f * st.alpha));
                r.tri(hc + Vec2{e.facing * 2 * s, 6 * s}, hc + Vec2{e.facing * 9 * s, 6 * s}, hc + Vec2{e.facing * 4 * s, 22 * s}, rimPass ? c : rgb(0xb8b4ac, st.alpha));   // beard
                break;
            case Hat::Bare: r.circle(hc + Vec2{-e.facing * 3 * s, -2 * s}, 10 * s, c); break;
            }
            if (!rimPass && e.alive()) {
                Color eyeC = e.awareness >= 1 ? rgb(0xff3a2a) : (e.awareness > 0.3f ? rgb(0xffb040) : rgb(0x000000, 0));
                if (e.type == EType::Tetsugen) { r.circle(j.head + side * 2 * s, 7 * s, lacquer); eyeC = burning ? rgb(0xffa040) : rgb(0xff3a2a); }
                if (e.boss && e.type != EType::Tetsugen) eyeC = accent;
                if (hollow) eyeC = rgb(0xd8c8ff);
                if (eyeC.a > 0) {
                    r.setBlend(Blend::Additive);
                    Vec2 ep = j.head + side * 5 * s + Vec2{0, -1 * s};
                    r.circle(ep, 1.6f * s, eyeC.withA(st.alpha));
                    r.glow(ep, 10 * s, eyeC.withA(0.5f * st.alpha));
                    r.setBlend(Blend::Alpha);
                }
            }
        }
        if (layer == GearLayer::Torso && !rimPass) {
            Vec2 a = lerp(j.hip, j.chest, 0.3f), b = lerp(j.hip, j.chest, 0.95f);
            r.capsule(a, b, rig.hipR * 0.9f * s, rig.chestR * 0.85f * s, lacquer);
            for (int k = 1; k < 4; k++) {
                Vec2 m = lerp(a, b, k / 4.f);
                Vec2 n = (b - a).norm().perp();
                r.line(m - n * 9 * s, m + n * 9 * s, 1.2f, st.body.withA(st.alpha * 0.8f));
            }
            if (e.type == EType::Tetsugen || e.type == EType::TideWarden || e.type == EType::Hakuro || e.type == EType::Sentinel) {
                for (Vec2 sh : {j.shoulderF, j.shoulderB})
                    r.quad(sh + Vec2{-11 * s, -4 * s}, sh + Vec2{11 * s, -4 * s}, sh + Vec2{13 * s, 20 * s}, sh + Vec2{-13 * s, 20 * s}, lacquer);
                r.quad(j.hip + Vec2{-14 * s, -4 * s}, j.hip + Vec2{14 * s, -4 * s}, j.hip + Vec2{18 * s, 22 * s}, j.hip + Vec2{-18 * s, 22 * s}, lacquer);
            }
            if (e.type == EType::Duelist) r.line(j.hip + Vec2{-10 * s, -2 * s}, j.hip + Vec2{-e.facing * 26 * s, 14 * s + std::sin(time * 4) * 3}, 3 * s, accent.withA(st.alpha));
            if (e.type == EType::Archer) {
                Vec2 q0 = j.chest + Vec2{-e.facing * 10 * s, -6 * s}, q1 = j.hip + Vec2{-e.facing * 14 * s, 0};
                r.line(q0, q1, 6 * s, st.body.withA(st.alpha));
                for (int k = 0; k < 3; k++) r.line(q0 + Vec2{(k - 1) * 3.f, 0}, q0 + Vec2{(k - 1) * 4.f - e.facing * 2, -10 * s}, 1.2f, rgb(0xb8b0a0, 0.6f * st.alpha));
            }
        }
        if (layer == GearLayer::Weapon) {
            Vec2 d = dirFromAngle(j.foreArmFAngle + e.pose.sword * e.facing);
            Color dark = st.body.withA(st.alpha), steel = st.blade.withA(st.alpha);
            switch (L.weapon) {
            case Weapon::Spear: case Weapon::Glaive: {
                Vec2 butt = j.handF - d * 70 * s, tip = j.handF + d * 110 * s;
                r.line(butt, tip, 3 * s, dark);
                if (L.weapon == Weapon::Glaive) {
                    Vec2 n = d.perp();
                    r.quad(tip - n * 3 * s, tip + n * 6 * s, tip + d * 34 * s + n * 4 * s, tip + d * 4 * s - n * 3 * s, accent.withA(st.alpha * 0.9f));
                } else r.tri(tip + d.perp() * 3.5f * s, tip - d.perp() * 3.5f * s, tip + d * 22 * s, steel);
                break;
            }
            case Weapon::Bow: {
                Vec2 hand = j.handF;
                float k = e.st == EState::Windup ? clamp01(e.stT / 0.6f) : 0;
                Vec2 top = hand + Vec2{-e.facing * 6 * s, -46 * s}, bot = hand + Vec2{-e.facing * 6 * s, 40 * s};
                Vec2 ctrl = hand + Vec2{e.facing * 14 * s, 0};
                Vec2 prev = top;
                for (int i = 1; i <= 10; i++) {
                    float t = i / 10.f;
                    Vec2 q = lerp(lerp(top, ctrl, t), lerp(ctrl, bot, t), t);
                    r.line(prev, q, 2.6f * s, dark);
                    prev = q;
                }
                Vec2 nock = lerp(hand, j.handB, k);
                r.line(top, nock, 0.8f, rgb(0xb8b0a0, 0.6f * st.alpha));
                r.line(nock, bot, 0.8f, rgb(0xb8b0a0, 0.6f * st.alpha));
                if (k > 0) r.line(nock, hand + Vec2{e.facing * 20 * s, 0}, 1.4f, rgb(0xb8b0a0, st.alpha));
                break;
            }
            case Weapon::Sickle: {
                Vec2 end = j.handF + d * 30 * s;
                r.line(j.handF - d * 6 * s, end, 3 * s, dark);
                Vec2 n = d.perp() * e.facing;
                Vec2 m = end + n * -10 * s + d * 4 * s, tip = end + n * -26 * s - d * 8 * s;
                r.tri(end, m, tip, steel);
                break;
            }
            case Weapon::Hook: {
                // the Warden's iron boat-hook on a length of chain
                Vec2 end = j.handF + d * 80 * s;
                r.line(j.handF - d * 20 * s, end, 4 * s, dark);
                r.ring(end + d.perp() * e.facing * -8 * s, 8 * s, 11 * s, steel, 0, PI * 1.3f);
                if (!rimPass) for (int k = 0; k < 5; k++) r.circle(lerp(j.handB, j.hip, k / 5.f) + Vec2{0, 6.f * std::sin(time * 3 + k)}, 2.2f * s, rgb(0x5a6068, st.alpha));
                break;
            }
            case Weapon::Gun: {
                // matchlock: long barrel along the forearm, a glowing match
                Vec2 stock = j.handB - d * 10 * s, muzzle = j.handF + d * 74 * s;
                r.line(stock, muzzle, 3.4f * s, dark);
                r.line(stock, stock - d * 18 * s + d.perp() * e.facing * 5 * s, 5 * s, dark);
                if (!rimPass && e.alive()) {
                    r.setBlend(Blend::Additive);
                    Vec2 m = j.handF + d * 8 * s;
                    float fl = 0.6f + 0.4f * std::sin(time * 20 + e.id);
                    r.glow(m, 8 * s, rgb(0xff8a3a, 0.6f * fl * st.alpha));
                    r.setBlend(Blend::Alpha);
                    if (e.st == EState::Windup) {
                        // the aim line: a thin red warning thread to the target
                        Vec2 tgt = player.pos + Vec2{0, -50};
                        r.setBlend(Blend::Additive);
                        r.line(muzzle, lerp(muzzle, tgt, clamp01(e.stT / 0.6f)), 1.2f, rgb(0xff3a2a, 0.45f * clamp01(e.stT * 3)));
                        r.setBlend(Blend::Alpha);
                    }
                }
                break;
            }
            case Weapon::Staff: {
                Vec2 a = j.handF - d * 80 * s, b = j.handF + d * 80 * s;
                r.line(a, b, 3.2f * s, rgb(0x2a1e14, st.alpha));
                if (!rimPass) { r.setBlend(Blend::Additive); r.glow(b, 10 * s, accent.withA(0.3f * st.alpha)); r.setBlend(Blend::Alpha); }
                break;
            }
            case Weapon::Threads: {
                if (!rimPass) {
                    r.setBlend(Blend::Additive);
                    for (Vec2 h : {j.handF, j.handB})
                        for (int k = 0; k < 3; k++) r.line(h, h + Vec2{e.facing * (20 + k * 8.f), 14.f * std::sin(time * 3 + k) + k * 6.f} * s, 1, accent.withA(0.6f * st.alpha));
                    r.setBlend(Blend::Alpha);
                }
                break;
            }
            case Weapon::Sword: break;
            }
            if ((burning || empowered) && sword && !rimPass) {
                Color gc = burning ? rgb(0xff6a20) : accent;
                r.setBlend(Blend::Additive);
                for (int i = 0; i < 6; i++) {
                    Vec2 q = lerp(j.handF, j.swordTip, (i + 1) / 7.f);
                    r.glow(q, (16 + 6 * std::sin(time * 12 + i)) * s, gc.withA(0.3f));
                }
                r.setBlend(Blend::Alpha);
            }
        }
    }, sword);
    Color edge = burning ? rgb(0xff8a3a) : (empowered ? accent : lerp(st.rim, Color{1, 1, 1}, 0.3f));
    drawTrail(r, e.trail, time, edge, 0.12f);
    if (burning && e.alive() && hashf((int)(time * 60), e.id) < 0.5f)
        const_cast<World*>(this)->emit(PType::Ember, lerp(j.handF, j.swordTip, hashf((int)(time * 90), 3)), 1, 80, rgb(0xff8a3a), 3, 0.6f, 1.0f, -PI / 2);
    if (e.chill > 0) { r.setBlend(Blend::Additive); r.glow(pos + Vec2{0, -e.h() * 0.5f}, 50 * s, rgb(0x9fd8ff, 0.15f)); r.setBlend(Blend::Alpha); }
    // weapon glint telegraph
    if (e.glint > 0) {
        r.setBlend(Blend::Additive);
        Vec2 g = j.swordTip;
        if (!sword) g = j.handF + dirFromAngle(j.foreArmFAngle + e.pose.sword * e.facing) * (L.weapon == Weapon::Spear || L.weapon == Weapon::Glaive ? 120.f : 40.f) * s;
        float k = e.glint;
        r.line(g + Vec2{-26 * k, 0}, g + Vec2{26 * k, 0}, 2.2f, Color{1, 1, 0.9f, k});
        r.line(g + Vec2{0, -18 * k}, g + Vec2{0, 18 * k}, 2.2f, Color{1, 1, 0.9f, k});
        r.glow(g, 40 * k, Color{1, 0.95f, 0.8f, 0.6f * k});
        r.setBlend(Blend::Alpha);
    }
}

Pose npcPose(const std::string& kind, const std::string& name, float t) {
    if (name == "kneel") return P(-4, 30, 2, 8, 30, 32, 24, 36, 60, -150, 54, -146, -20);
    if (name == "point") return P(0, 2, 2, -6, 92, 0, 10, 20, 6, -4, -8, -4, 0);
    if (name == "staff") return P(0, 2, 4, -2, 30, 50, 10, 20, 8, -4, -8, -4, 0);
    if (kind == "fuki") {
        Pose p = P(-2, 6, 28, 18, 50, 60, 10, 50, 10, -14, -12, -10, 0);
        p.hipY += std::sin(t * 1.5f) * 0.6f;
        return p;
    }
    Pose p = P(0, 2, 2, 0, 12, 14, -6, 12, 6, -4, -6, -4, 0);
    p.hipY += std::sin(t * 1.8f) * 0.8f;
    return p;
}

void World::drawNpc(Renderer& r, const Npc& n, float alpha) {
    if (!n.visible || n.alpha <= 0) return;
    Vec2 pos = lerp(n.prevPos, n.pos, alpha);
    Rig rig;
    if (n.kind == "fuki") { rig.scale = 0.86f; rig.headR = 9; }
    if (n.kind == "odaka") { rig.scale = 1.12f; rig.shoulderW = 4; rig.bladeLen = 0; }
    if (n.kind == "hana") { rig.scale = 0.9f; rig.headR = 9.5f; }
    if (n.kind == "child") { rig.scale = 0.62f; rig.headR = 10; }
    if (n.kind == "sayo") { rig.scale = 1.0f; }
    if (n.kind == "refugee" || n.kind == "ferryman" || n.kind == "mason") rig.scale = 0.96f + hashf(n.id.size(), 3) * 0.08f;
    Pose pose = npcPose(n.kind, n.pose, n.t);
    if (n.walking) {
        float ph = n.t * 6;
        pose.legFU = (8 + 24 * std::sin(ph)) * DEG; pose.legBU = (8 - 24 * std::sin(ph)) * DEG;
        pose.legFL = (-10 - 30 * std::max(0.f, std::cos(ph))) * DEG; pose.legBL = (-10 - 30 * std::max(0.f, -std::cos(ph))) * DEG;
    }
    Joints j = solve(rig, pose, pos, n.facing);
    RenderStyle st;
    st.body = rgb(0x0e0d10); st.bodyBack = rgb(0x070609);
    st.rim = rimAt(pos + Vec2{0, -50}); st.lightDir = lightDirFor(palette); st.alpha = n.alpha;
    const float s = rig.scale;
    bool sayo = n.kind == "sayo";
    if (sayo) {
        // the Weaver: a figure of dark cloth threaded through with gold, the threads running off into the dark
        st.body = rgb(0x1a1410); st.bodyBack = rgb(0x0e0a08);
        st.rim = rgb(0xffd890);
        r.setBlend(Blend::Additive);
        for (int k = 0; k < 14; k++) {
            float u = hashf(k, 7), ang = -PI / 2 + (u - 0.5f) * 2.6f;
            Vec2 a = pos + Vec2{(hashf(k, 3) - 0.5f) * 24, -40 - hashf(k, 5) * 50};
            Vec2 b = a + Vec2{std::cos(ang), std::sin(ang)} * (300 + 200 * u) + Vec2{std::sin(n.t * 0.7f + k) * 12, 0};
            r.line(a, b, 1.2f, rgb(0xffd27a, 0.35f * n.alpha));
        }
        r.glow(pos + Vec2{0, -60}, 140, rgb(0xffc870, 0.18f * n.alpha));
        r.setBlend(Blend::Alpha);
    }
    if (n.kind == "odaka") {
        // straw cloak (mino) drawn as layered fringe
        Cloth c = n.cloth;
        drawClothStrip(r, c, 30 * s, 22 * s, lerp(st.body, rgb(0x2a2414), 0.5f).withA(n.alpha), st.rim.mulA(0.5f * n.alpha), st.lightDir, 1.5f);
    }
    drawHumanoid(r, rig, j, st, [&](GearLayer layer, bool rimPass) {
        Color c = rimPass ? st.rim.mulA(n.alpha) : st.body.withA(n.alpha);
        Vec2 o = rimPass ? st.lightDir * (st.rimWidth * s) : Vec2{0, 0};
        if (layer == GearLayer::Head) {
            if (n.kind == "odaka") {
                Vec2 hc = j.head + o + Vec2{0, -6 * s};
                r.tri(hc + Vec2{-26 * s, 5 * s}, hc + Vec2{26 * s, 5 * s}, hc + Vec2{0, -12 * s}, c);
            } else if (n.kind == "fuki") {
                r.circle(j.head + o + Vec2{-n.facing * 4 * s, -3 * s}, 9 * s, c);   // hair bun under a shawl
            } else if (n.kind == "hana") {
                // a high tail of hair tied with a ribbon
                Vec2 base = j.head + o + Vec2{-n.facing * 7 * s, -6 * s};
                r.line(base, base + Vec2{-n.facing * 10 * s, 14 * s + std::sin(n.t * 2) * 2}, 5 * s, c);
                if (!rimPass) r.circle(base, 3 * s, rgb(0xc0607a, n.alpha));
            } else if (n.kind == "sayo") {
                // hair worn long and loose; a band of gold thread across the brow
                r.quad(j.head + o + Vec2{-n.facing * 6 * s, -6 * s}, j.head + o + Vec2{-n.facing * 12 * s, -2 * s},
                       j.head + o + Vec2{-n.facing * 16 * s, 34 * s}, j.head + o + Vec2{-n.facing * 6 * s, 30 * s}, c);
                if (!rimPass) r.line(j.head + Vec2{-9 * s, -5 * s}, j.head + Vec2{9 * s, -5 * s}, 1.6f, rgb(0xffd27a, n.alpha));
            } else if (n.kind == "refugee" || n.kind == "mason") {
                Vec2 hc = j.head + o + Vec2{0, -4 * s};
                r.tri(hc + Vec2{-17 * s, 3 * s}, hc + Vec2{17 * s, 3 * s}, hc + Vec2{0, -9 * s}, c);
            } else if (n.kind == "ferryman") {
                Vec2 hc = j.head + o + Vec2{0, -4 * s};
                r.tri(hc + Vec2{-26 * s, 4 * s}, hc + Vec2{26 * s, 4 * s}, hc + Vec2{0, -10 * s}, c);
            }
        }
        if (layer == GearLayer::Torso && !rimPass) {
            if (n.kind == "refugee" || n.kind == "mason") {
                // a bundle carried on the back
                Vec2 b = j.chest + Vec2{-n.facing * 12 * s, 4 * s};
                r.circle(b, 11 * s, lerp(st.body, rgb(0x4a3a2a), 0.4f).withA(n.alpha));
            }
            if (n.kind == "hana") r.line(j.hip + Vec2{-8 * s, -2 * s}, j.hip + Vec2{8 * s, -2 * s}, 4 * s, rgb(0xc0607a, n.alpha));
            if (sayo) {
                Vec2 a = lerp(j.hip, j.chest, 0.2f), b = j.chest;
                for (int k = 0; k < 4; k++) r.line(lerp(a, b, k / 4.f) + Vec2{-8 * s, 0}, lerp(a, b, k / 4.f) + Vec2{8 * s, 2 * s}, 1, rgb(0xffd27a, 0.7f * n.alpha));
            }
        }
        if (layer == GearLayer::Weapon && !rimPass) {
            if (n.kind == "ferryman") r.line(j.handF + Vec2{0, -70 * s}, j.handF + Vec2{0, 70 * s}, 2.6f * s, st.body.withA(n.alpha));
            if (n.kind == "odaka") {
                Vec2 d{0, 1};
                r.line(j.handF + d * -60 * s, j.handF + d * 60 * s, 3 * s, st.body.withA(n.alpha));
            }
            if (n.kind == "fuki") {
                Vec2 stickEnd = j.handF + Vec2{n.facing * 26 * s, -18 * s};
                r.line(j.handF, stickEnd, 2, st.body.withA(n.alpha));
                Vec2 lp = stickEnd + Vec2{0, 16};
                r.line(stickEnd, lp - Vec2{0, 8}, 1, st.body.withA(n.alpha));
                r.circle(lp, 8, rgb(0xffb050, n.alpha));
                r.setBlend(Blend::Additive); r.glow(lp, 80, rgb(0xffb050, 0.35f * n.alpha)); r.setBlend(Blend::Alpha);
            }
        }
    }, false);
}

void World::drawProjectiles(Renderer& r, float alpha) {
    for (auto& p : projectiles) {
        Vec2 pos = lerp(p.prevPos, p.pos, alpha);
        switch (p.type) {
        case ProjType::Shuriken: {
            for (int k = 0; k < 4; k++) {
                Vec2 d = rotate({0, -10}, p.rot + k * PI / 2);
                r.tri(pos + d, pos + rotate(d, 0.7f) * 0.32f, pos + rotate(d, -0.7f) * 0.32f, rgb(0xb8c0cc));
            }
            r.circle(pos, 2.5f, rgb(0x202020));
            if (!p.stuck) { r.setBlend(Blend::Additive); r.line(pos, pos - p.vel.norm() * 40, 2, rgb(0xd0d8ff, 0.25f)); r.setBlend(Blend::Alpha); }
            break;
        }
        case ProjType::Arrow: {
            Vec2 d = p.vel.len() > 1 ? p.vel.norm() : Vec2{1, 0};
            Color c = p.reflected ? rgb(0xffe0a0) : palette.nearC * 2.5f;
            r.line(pos - d * 34, pos, 2, c);
            r.tri(pos + d * 8, pos + d.perp() * 3, pos - d.perp() * 3, rgb(0xa0a4ac));
            r.tri(pos - d * 34, pos - d * 26 + d.perp() * 4, pos - d * 28, rgb(0xd8d0c0, 0.7f));
            r.tri(pos - d * 34, pos - d * 26 - d.perp() * 4, pos - d * 28, rgb(0xd8d0c0, 0.7f));
            if (p.reflected) { r.setBlend(Blend::Additive); r.glow(pos, 20, rgb(0xffe0a0, 0.5f)); r.setBlend(Blend::Alpha); }
            break;
        }
        case ProjType::Ember: {
            r.setBlend(Blend::Additive);
            float dir = sign(p.vel.x);
            float base = dir > 0 ? -PI * 0.5f : PI * 0.5f;
            r.ring(pos, 34, 52, rgb(0xff6a20, 0.6f), base - 1.2f, base + 1.2f);
            r.ring(pos, 44, 52, rgb(0xffd080, 0.9f), base - 1.0f, base + 1.0f);
            r.glow(pos, 90, rgb(0xff6a20, 0.45f));
            r.setBlend(Blend::Alpha);
            break;
        }
        case ProjType::Gale: {
            r.setBlend(Blend::Additive);
            float dir = sign(p.vel.x);
            float base = dir > 0 ? -PI * 0.5f : PI * 0.5f;
            float k = clamp01(p.life / 0.42f);
            r.ring(pos, 30, 44, rgb(0x6fd8c8, 0.45f * k), base - 1.3f, base + 1.3f);
            r.ring(pos, 40, 45, rgb(0xe8fffa, 0.9f * k), base - 1.1f, base + 1.1f);
            r.ring(pos - Vec2{dir * 18, 0}, 26, 30, rgb(0x9ff0e0, 0.35f * k), base - 1.0f, base + 1.0f);
            r.glow(pos, 70, rgb(0x6fd8c8, 0.3f * k));
            r.setBlend(Blend::Alpha);
            break;
        }
        case ProjType::FlameWave: {
            drawFlame(r, pos, 110, time * 1.6f, (int)(p.pos.x * 0.1f) & 7);
            drawFlame(r, pos + Vec2{-sign(p.vel.x) * 30, 0}, 80, time * 1.4f, 3);
            break;
        }
        case ProjType::TideWave: {
            // a curling wall of water
            float f = sign(p.vel.x);
            Color deep = palette.water * 2.2f, foam = rgb(0xd8f4f8, 0.85f);
            for (int k = 0; k < 7; k++) {
                float u = k / 6.f, h = 90 * std::sin(u * PI * 0.9f + 0.2f);
                float x = pos.x - f * (60 - u * 70);
                r.rect({x - 6, pos.y - h, 12, h}, deep.withA(0.75f));
            }
            Vec2 crest = pos + Vec2{f * 8, -86};
            r.ring(crest + Vec2{-f * 14, 14}, 12, 18, foam, f > 0 ? -PI : 0, f > 0 ? 0 : PI);
            r.setBlend(Blend::Additive); r.glow(pos + Vec2{0, -40}, 70, rgb(0x80e0f0, 0.18f)); r.setBlend(Blend::Alpha);
            break;
        }
        case ProjType::ThreadWave: {
            r.setBlend(Blend::Additive);
            for (int k = 0; k < 9; k++) {
                float u = k / 8.f;
                Vec2 a{pos.x + std::sin(time * 9 + k) * 10, pos.y};
                Vec2 b{pos.x - sign(p.vel.x) * 40 + std::sin(time * 7 + k * 2) * 14, pos.y - 30 - u * 70};
                r.line(a, b, 1.5f, palette.thread.withA(0.8f));
            }
            r.glow(pos + Vec2{0, -40}, 60, palette.thread.withA(0.25f));
            r.setBlend(Blend::Alpha);
            break;
        }
        case ProjType::Kunai: {
            Vec2 d = p.vel.norm();
            r.line(pos - d * 14, pos, 2.2f, rgb(0x20202a));
            r.tri(pos + d.perp() * 3, pos - d.perp() * 3, pos + d * 9, rgb(0xb8c0c8));
            break;
        }
        case ProjType::Ball: {
            Vec2 d = p.vel.norm();
            r.setBlend(Blend::Additive);
            r.line(pos - d * 60, pos, 2.5f, rgb(0xffd080, 0.5f));
            r.setBlend(Blend::Alpha);
            r.circle(pos, 3.5f, rgb(0x303038));
            break;
        }
        case ProjType::Snare: {
            r.setBlend(Blend::Additive);
            Vec2 d = p.vel.norm();
            for (int k = 0; k < 3; k++) r.line(pos - d * 40 + d.perp() * (k - 1) * 4.f, pos, 1.2f, rgb(0xd8c8ff, 0.7f));
            r.ring(pos, 6, 8, rgb(0xd8c8ff, 0.9f));
            r.setBlend(Blend::Alpha);
            break;
        }
        }
    }
}

void World::drawParticles(Renderer& r, bool fg) {
    for (auto& p : particles) {
        if (p.fg != fg) continue;
        float k = clamp01(p.life / p.maxLife);
        switch (p.type) {
        case PType::Spark: {
            r.setBlend(Blend::Additive);
            Vec2 d = p.vel * 0.022f;
            r.line(p.pos - d, p.pos, p.size * k, p.col.withA(k));
            r.setBlend(Blend::Alpha);
            break;
        }
        case PType::Ink: {
            Vec2 d = p.vel * 0.012f;
            r.capsule(p.pos - d, p.pos, p.size * (0.4f + 0.6f * k), p.size * k, p.col.withA(p.col.a * std::min(1.f, k * 2)));
            break;
        }
        case PType::Ember: {
            r.setBlend(Blend::Additive);
            float fl = 0.6f + 0.4f * std::sin(time * 20 + p.rot * 10);
            r.circle(p.pos, p.size * 0.5f, p.col.withA(k * fl));
            r.glow(p.pos, p.size * 3, p.col.withA(0.35f * k * fl));
            r.setBlend(Blend::Alpha);
            break;
        }
        case PType::Ash: {
            Vec2 d = rotate({p.size, 0}, p.rot);
            r.quad(p.pos - d, p.pos + d.perp() * 0.5f, p.pos + d, p.pos - d.perp() * 0.5f, p.col.withA(p.col.a * std::min(1.f, k * 3)));
            break;
        }
        case PType::Leaf: {
            Vec2 d = rotate({p.size, 0}, p.rot), n = d.perp() * 0.18f;
            r.quad(p.pos - d, p.pos + n, p.pos + d, p.pos - n, p.col.withA(std::min(1.f, k * 3)));
            break;
        }
        case PType::Petal: {
            Vec2 d = rotate({p.size, 0}, p.rot), n = d.perp() * 0.5f;
            r.quad(p.pos - d * 0.6f, p.pos + n, p.pos + d, p.pos - n, p.col.withA(std::min(1.f, k * 3)));
            break;
        }
        case PType::Firefly: {
            r.setBlend(Blend::Additive);
            float pulse = std::pow(0.5f + 0.5f * std::sin(time * 2.2f + p.rot * 7), 3.f);
            float a = pulse * std::min(1.f, k * 3) * std::min(1.f, (1 - k) * 5);
            r.circle(p.pos, p.size * 0.5f, p.col.withA(a));
            r.glow(p.pos, p.size * 6, p.col.withA(a * 0.4f));
            r.setBlend(Blend::Alpha);
            break;
        }
        case PType::Smoke: {
            float grow = 1 + (1 - k) * 1.5f;
            r.glow(p.pos, p.size * grow, p.col.withA(p.col.a * k));
            break;
        }
        case PType::Dust: {
            float grow = 1 + (1 - k) * 1.2f;
            r.glow(p.pos, p.size * grow, p.col.withA(p.col.a * k));
            break;
        }
        case PType::Flash: {
            r.setBlend(Blend::Additive);
            r.glow(p.pos, p.size * (1.5f - k * 0.5f), p.col.withA(k));
            r.setBlend(Blend::Alpha);
            break;
        }
        case PType::Mist: {
            float a = std::sin(k * PI);
            r.textured(r.noiseTex(), true, {p.pos.x - p.size, p.pos.y - p.size * 0.3f, p.size * 2, p.size * 0.6f},
                       {p.pos.x * 0.0005f, p.rot * 0.1f, 0.4f, 0.2f}, p.col.withA(p.col.a * a));
            break;
        }
        case PType::Thread: {
            r.setBlend(Blend::Additive);
            Vec2 d = p.vel.norm() * 10;
            r.line(p.pos - d, p.pos + d, 1.2f, p.col.withA(k));
            r.setBlend(Blend::Alpha);
            break;
        }
        case PType::Rain: {
            Vec2 d = p.vel.norm() * p.size;
            r.line(p.pos - d, p.pos, 1.3f, p.col);
            break;
        }
        case PType::Snow: r.circle(p.pos, p.size, p.col.withA(p.col.a * std::min(1.f, k * 3))); break;
        case PType::Splash: r.circle(p.pos, p.size * (0.5f + 0.5f * k), p.col.withA(p.col.a * k)); break;
        case PType::Feather: {
            Vec2 d = rotate({7, 0}, p.rot);
            r.quad(p.pos - d, p.pos + d.perp() * 0.25f, p.pos + d, p.pos - d.perp() * 0.25f, p.col.withA(p.col.a * k));
            break;
        }
        case PType::Frost: {
            r.setBlend(Blend::Additive);
            r.line(p.pos + Vec2{-p.size, 0}, p.pos + Vec2{p.size, 0}, 1, p.col.withA(k));
            r.line(p.pos + Vec2{0, -p.size}, p.pos + Vec2{0, p.size}, 1, p.col.withA(k));
            r.setBlend(Blend::Alpha);
            break;
        }
        }
    }
}

void World::drawForeground(Renderer& r, Vec2 camPos, float zoom) {
    const float par = 1.4f;
    r.setWorldView(camPos, zoom, par);
    Vec2 vs = r.viewSize();
    float cx = camPos.x * par, cy = camPos.y * par;
    Color ink = lerp(palette.nearC * 0.4f, palette.fog, 0.08f);
    int ri = level.roomAt(camPos.x);
    BgStyle style = level.rooms[ri].style;
    float top = cy - vs.y * 0.5f, bottom = cy + vs.y * 0.5f;
    // sparse framing elements that never cover the middle of the screen
    float spacing = 1500;
    int i0 = (int)std::floor((cx - vs.x) / spacing), i1 = (int)std::floor((cx + vs.x) / spacing);
    for (int i = i0; i <= i1; i++) {
        float h = hashf(i, 5 + (int)style);
        if (h < 0.3f) continue;
        float x = i * spacing + hashf(i, 9) * spacing * 0.7f;
        float sway = std::sin(time * 0.8f + i) * 6;
        if (style == BgStyle::Grove || style == BgStyle::Cliff) {
            // leaf sprays hanging into the frame
            int n = 6 + (int)(h * 8);
            for (int k = 0; k < n; k++) {
                Vec2 base{x + (hashf(i, k) - 0.5f) * 300, top - 20};
                float len = 50 + hashf(k, i) * 90;
                Vec2 tip = base + Vec2{(hashf(i + k, 3) - 0.5f) * 140 + sway, len};
                Vec2 perp = (tip - base).norm().perp() * (4 + hashf(k, 7) * 5);
                Vec2 mid = lerp(base, tip, 0.45f);
                r.quad(base, mid + perp, tip, mid - perp, ink);
            }
        } else if (style == BgStyle::Gate) {
            for (int k = 0; k < 6; k++) {
                Vec2 c{x + (hashf(i, k) - 0.5f) * 320 + sway, top + hashf(k, i) * 70 - 20};
                r.circle(c, 50 + hashf(i + k, 2) * 50, lerp(ink, rgb(0x3a0408), 0.5f));
            }
        } else if (style == BgStyle::Village) {
            // fair lantern strings sagging across the top of the frame
            float x0 = x - 420, x1 = x + 420, y0 = top + 20 + h * 40;
            Vec2 prev{x0, y0};
            for (int k = 1; k <= 24; k++) {
                float t = k / 24.f;
                Vec2 q{lerp(x0, x1, t), y0 + std::sin(t * PI) * 110 + sway * t};
                r.line(prev, q, 3, ink);
                prev = q;
                if (k % 3 == 0 && k < 24) {
                    Vec2 lp = q + Vec2{0, 26};
                    float fl = 0.75f + 0.25f * noise1(time * 4 + k, i);
                    r.line(q, lp - Vec2{0, 12}, 1.5f, ink);
                    r.circle(lp, 13, lerp(rgb(0xc8401a), rgb(0xffa040), 0.3f).withA(fl));
                    r.rect({lp.x - 7, lp.y - 15, 14, 4}, ink); r.rect({lp.x - 7, lp.y + 11, 14, 4}, ink);
                    r.setBlend(Blend::Additive); r.glow(lp, 60, rgb(0xff8a3a, 0.3f * fl)); r.setBlend(Blend::Alpha);
                }
            }
        }
        // dark grass tufts at the very bottom of the frame
        for (int k = 0; k < 14; k++) {
            float bx = x + (hashf(k, i * 3) - 0.5f) * 500;
            float bh = 40 + hashf(i, k * 5) * 90;
            float sw = std::sin(time * 1.3f + bx * 0.01f) * 8;
            r.tri({bx - 7, bottom + 10}, {bx + 7, bottom + 10}, {bx + sw, bottom - bh}, ink);
        }
    }
}

// =================================================================== frame

// =================================================================== water, wind & stance effects

void World::drawWater(Renderer& r, const Rect& view) {
    int x0 = std::max(0, (int)std::floor(view.x / TILE) - 1), x1 = std::min(level.width - 1, (int)std::floor(view.right() / TILE) + 1);
    int y0 = std::max(0, (int)std::floor(view.y / TILE) - 1), y1 = std::min(level.height - 1, (int)std::floor(view.bottom() / TILE) + 1);
    Color body = palette.water;
    Color hi = lerp(palette.water, palette.rim, 0.6f);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            if (!level.waterAt(x, y)) continue;
            float px = x * (float)TILE, py = y * (float)TILE;
            bool surface = !level.waterAt(x, y - 1) && !level.solidAt(x, y - 1);
            int depth = 0;
            while (depth < 5 && level.waterAt(x, y - depth - 1)) depth++;
            float a = 0.5f + 0.08f * depth;
            if (surface) {
                // gently moving surface, a bright edge and a sheen band
                float w0 = std::sin(time * 2.2f + px * 0.05f) * 2.5f, w1 = std::sin(time * 2.2f + (px + TILE) * 0.05f) * 2.5f;
                r.quad({px, py + 6 + w0}, {px + TILE + 0.5f, py + 6 + w1}, {px + TILE + 0.5f, py + TILE + 0.5f}, {px, py + TILE + 0.5f}, body.withA(a));
                r.line({px, py + 6 + w0}, {px + TILE + 0.5f, py + 6 + w1}, 1.6f, hi.withA(0.7f));
                r.setBlend(Blend::Additive);
                r.rectGrad({px, py + 6, TILE + 0.5f, 18}, hi.withA(0.12f), hi.withA(0));
                if (palette.rain > 0.05f && hashf(x, (int)(time * 6)) > 0.8f) {
                    float k = std::fmod(time * 6, 1.f);
                    r.ring({px + hashf(x * 3, (int)(time * 6)) * TILE, py + 7}, 2 + k * 9, 3 + k * 9, hi.withA(0.4f * (1 - k)));
                }
                r.setBlend(Blend::Alpha);
            } else r.rect({px, py, TILE + 0.5f, TILE + 0.5f}, body.withA(a));
            // slow caustic light
            if (depth < 3 && hashf(x, y) > 0.5f) {
                r.setBlend(Blend::Additive);
                float k = 0.5f + 0.5f * std::sin(time * 1.3f + x * 1.7f + y);
                r.line({px + 6, py + 20 + k * 10}, {px + TILE - 8, py + 26 - k * 6}, 1, hi.withA(0.08f));
                r.setBlend(Blend::Alpha);
            }
        }
}

void World::drawUpdrafts(Renderer& r, const Rect& view) {
    int x0 = std::max(0, (int)std::floor(view.x / TILE) - 1), x1 = std::min(level.width - 1, (int)std::floor(view.right() / TILE) + 1);
    int y0 = std::max(0, (int)std::floor(view.y / TILE) - 1), y1 = std::min(level.height - 1, (int)std::floor(view.bottom() / TILE) + 1);
    r.setBlend(Blend::Additive);
    Color c = lerp(palette.rim, rgb(0xffffff), 0.4f);
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++) {
            if (!level.updraftAt(x, y)) continue;
            float px = x * (float)TILE, py = y * (float)TILE;
            for (int k = 0; k < 3; k++) {
                float u = hashf(x * 7 + k, y);
                float yy = py + TILE - std::fmod(time * (160 + u * 120) + u * TILE, (float)TILE);
                float xx = px + 6 + u * (TILE - 12) + std::sin(time * 3 + u * 9) * 3;
                r.line({xx, yy}, {xx, yy - 14}, 1.2f, c.withA(0.25f));
            }
            if (!level.updraftAt(x, y + 1)) {
                // a vent of stirred leaves at the column's foot
                r.glow({px + TILE / 2, py + TILE}, 40, c.withA(0.08f));
            }
        }
    r.setBlend(Blend::Alpha);
}

void World::drawEffects(Renderer& r) {
    // thunder stance bolts
    r.setBlend(Blend::Additive);
    for (auto& b : bolts) {
        float k = clamp01(b.life / 0.35f);
        Vec2 prev = b.from;
        int seg = 9;
        for (int i = 1; i <= seg; i++) {
            float t = i / (float)seg;
            Vec2 q = lerp(b.from, b.to, t);
            if (i < seg) q.x += (hashf(b.seed + i, (int)(b.life * 30)) - 0.5f) * 60;
            r.line(prev, q, 3.5f * k, rgb(0xe8f6ff, k));
            r.line(prev, q, 10 * k, rgb(0x80c0ff, 0.25f * k));
            prev = q;
        }
        r.glow(b.to, 90 * k, rgb(0xa8d8ff, 0.5f * k));
    }
    r.setBlend(Blend::Alpha);
    // Mirror Step decoy
    if (decoyT > 0) {
        float a = clamp01(decoyT / 1.6f);
        RenderStyle gs;
        gs.body = rgb(0xe8dcff, 0.35f * a); gs.bodyBack = gs.body; gs.rim = rgb(0xffffff, 0.5f * a); gs.blade = gs.rim;
        Joints j = solve(Rig{}, npcPose("", "", time), decoyPos, player.facing);
        r.setBlend(Blend::Additive);
        drawHumanoid(r, Rig{}, j, gs, nullptr, true);
        r.glow(decoyPos + Vec2{0, -50}, 70, rgb(0xe8dcff, 0.15f * a));
        r.setBlend(Blend::Alpha);
    }
    // the Unravelling: everything behind the edge is coming apart
    if (chaseActive) {
        Vec2 vs = r.viewSize();
        float top = cam.pos.y - vs.y, bot = cam.pos.y + vs.y;
        r.rect({chaseX - 4000, top, 4000, bot - top}, rgb(0x08060c, 0.94f));
        r.rectGradH({chaseX - 160, top, 180, bot - top}, rgb(0x08060c, 0.0f), rgb(0x08060c, 0.9f));
        r.setBlend(Blend::Additive);
        for (int k = 0; k < 40; k++) {
            float y = top + hashf(k, 3) * (bot - top);
            float len = 40 + hashf(k, 9) * 120;
            float x = chaseX + std::sin(time * 4 + k) * 16;
            r.line({x, y}, {x - len, y + std::sin(time * 3 + k) * 20}, 1.2f, palette.thread.withA(0.6f));
        }
        r.glow({chaseX, player.pos.y - 60}, 260, palette.thread.withA(0.12f));
        r.setBlend(Blend::Alpha);
    }
}

void World::render(Renderer& r, float alpha) {
    Vec2 camPos = lerp(cam.prevPos, cam.pos, alpha) + cam.shakeOffset;
    float zoom = cam.zoom * kBaseZoom;
    SkyParams sky = palette.sky;
    sky.time = time; sky.scroll = camPos.x;
    r.drawSky(sky);
    drawBackground(r, camPos, zoom);
    r.setWorldView(camPos, zoom, 1.0f, cam.shakeRot);
    Vec2 vs = r.viewSize();
    Rect view{camPos.x - vs.x / 2, camPos.y - vs.y / 2, vs.x, vs.y};
    drawProps(r, view, false);
    // gates sit behind ceiling tiles so they can slide up into them
    drawMechs(r);
    drawTiles(r, view);
    drawGrass(r, view, false);
    drawParticles(r, false);
    for (auto& n : npcs) drawNpc(r, n, alpha);
    for (auto& e : enemies) drawEnemy(r, e, alpha);
    if (player.state != PState::Dead || player.deathT < 10) drawPlayer(r, alpha);
    drawProjectiles(r, alpha);
    drawWater(r, view);
    drawUpdrafts(r, view);
    drawEffects(r);
    drawGrass(r, view, true);
    drawProps(r, view, true);
    drawParticles(r, true);
    // light glows
    r.setBlend(Blend::Additive);
    for (auto& L : lights) {
        if (L.pos.x < view.x - L.radius || L.pos.x > view.right() + L.radius) continue;
        float fl = 1 - L.flicker * noise1(time * 7 + L.pos.x * 0.01f, 5);
        r.glow(L.pos, L.radius * 0.6f, L.col.withA(0.22f * fl));
        r.glow(L.pos, L.radius * 0.15f, L.col.withA(0.5f * fl));
    }
    r.setBlend(Blend::Alpha);
    drawForeground(r, camPos, zoom);
}

void World::renderWorldUI(Renderer& r, float alpha) {
    Vec2 camPos = lerp(cam.prevPos, cam.pos, alpha) + cam.shakeOffset;
    r.setWorldView(camPos, cam.zoom * kBaseZoom, 1.0f, cam.shakeRot);
    TextSystem& T = r.text();
    T.setPixelScale(r.height() / r.viewSize().y);
    for (auto& e : enemies) {
        if (!e.alive() && e.st != EState::BeingKilled) continue;
        Vec2 pos = lerp(e.prevPos, e.pos, alpha);
        Vec2 top = pos + Vec2{0, -e.h() - 30};
        // awareness
        if (e.awareness > 0.05f && e.awareness < 1 && e.st != EState::Engage) {
            float a = e.awareness;
            r.ring(top, 10, 13, rgb(0x000000, 0.4f));
            r.ring(top, 10, 13, rgb(0xffc860, 0.9f), -PI / 2, -PI / 2 + TAU * a);
            T.draw(r, Font::Bold, 16, top + Vec2{0, 6}, "?", rgb(0xffe0a0), Align::Center);
        }
        if (e.alertIcon > 0) {
            float a = clamp01(e.alertIcon * 2);
            T.draw(r, Font::Bold, 30, top + Vec2{0, 8 - (1 - e.alertIcon) * 10}, "!", rgb(0xff4a3a, a), Align::Center);
        }
        // perilous telegraph: red kanji inside a diamond (shape-coded for colour-blind players)
        if (e.st == EState::Windup && e.atk >= 0 && e.def->attacks[e.atk].perilous) {
            float k = clamp01(e.stT / 0.15f);
            Vec2 c = top + Vec2{0, -16};
            float sz = 26 * (1.3f - 0.3f * k);
            Vec2 d[4] = {c + Vec2{0, -sz}, c + Vec2{sz, 0}, c + Vec2{0, sz}, c + Vec2{-sz, 0}};
            r.polygon(d, 4, rgb(0x2a0004, 0.85f * k));
            for (int i = 0; i < 4; i++) r.line(d[i], d[(i + 1) % 4], 2.5f, rgb(0xff2a2a, k));
            T.draw(r, Font::Kanji, 26, c + Vec2{0, 9}, "危", rgb(0xff5040, k), Align::Center);
        }
        // posture & vitality for engaged regular enemies
        if (!e.boss && e.awareness >= 1 && e.alive()) {
            float w = 70;
            float hpF = clamp01(e.hp / (e.def->hp * e.hpScale));
            r.rect({pos.x - w / 2, top.y + 6, w, 4}, rgb(0x000000, 0.6f));
            r.rect({pos.x - w / 2, top.y + 6, w * hpF, 4}, rgb(0xb8c0c8, 0.9f));
            float pf = clamp01(e.posture / e.def->posture);
            if (pf > 0.02f) {
                Color pc = lerp(rgb(0xffd060), rgb(0xff3a20), pf);
                r.rect({pos.x - w / 2 * pf, top.y - 2, w * pf, 5}, pc);
            }
        }
        if (e.st == EState::Broken || e.st == EState::BeingKilled) {
            // deathblow seal: a vermilion ring around the broken foe
            Vec2 c = pos + Vec2{0, -e.h() * 0.55f};
            float pulse = 0.5f + 0.5f * std::sin(time * 8);
            r.setBlend(Blend::Additive);
            r.ring(c, 26, 30, rgb(0xff3a20, 0.7f + 0.3f * pulse));
            r.glow(c, 50, rgb(0xff3a20, 0.3f * pulse));
            r.setBlend(Blend::Alpha);
        }
    }
    // contextual prompt
    if (!prompt.empty() && !controlLocked) {
        float w = measureRich(r, *input, Font::Bold, 18, prompt) + 30;
        Vec2 c = promptPos;
        float pulse = 0.9f + 0.1f * std::sin(time * 5);
        r.textured(r.brushTex(), true, {c.x - w / 2 - 14, c.y - 28, w + 28, 40}, {0, 0, 1, 1}, rgb(0x000000, 0.78f));
        drawRich(r, *input, Font::Bold, 18, c, prompt, rgb(0xf0e6d0, pulse), Align::Center);
    }
}

} // namespace sv
