#pragma once
// Small, header-only math kit. World space is y-down (screen convention), units are pixels at 1080p.
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace sv {

constexpr float PI = 3.14159265358979f;
constexpr float TAU = 6.28318530717959f;
constexpr float DEG = PI / 180.0f;

struct Vec2 {
    float x = 0, y = 0;
    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}
    constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    float len() const { return std::sqrt(x * x + y * y); }
    float len2() const { return x * x + y * y; }
    Vec2 norm() const { float l = len(); return l > 1e-6f ? Vec2{x / l, y / l} : Vec2{0, 0}; }
    Vec2 perp() const { return {-y, x}; }
};
inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
// Angle convention: dir(0) points down (+y); positive angles rotate toward +x.
inline Vec2 dirFromAngle(float a) { return {std::sin(a), std::cos(a)}; }
inline Vec2 rotate(Vec2 v, float a) { float c = std::cos(a), s = std::sin(a); return {v.x * c - v.y * s, v.x * s + v.y * c}; }

struct Color {
    float r = 1, g = 1, b = 1, a = 1;
    constexpr Color() = default;
    constexpr Color(float r_, float g_, float b_, float a_ = 1.f) : r(r_), g(g_), b(b_), a(a_) {}
    constexpr Color operator*(float s) const { return {r * s, g * s, b * s, a}; }
    constexpr Color withA(float na) const { return {r, g, b, na}; }
    constexpr Color mulA(float m) const { return {r, g, b, a * m}; }
};
constexpr Color rgb(uint32_t hex, float a = 1.f) {
    return {((hex >> 16) & 255) / 255.f, ((hex >> 8) & 255) / 255.f, (hex & 255) / 255.f, a};
}
inline Color lerp(Color a, Color b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline Vec2 lerp(Vec2 a, Vec2 b, float t) { return a + (b - a) * t; }
inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float sign(float v) { return v > 0 ? 1.f : (v < 0 ? -1.f : 0.f); }
inline float approach(float v, float target, float step) {
    return v < target ? std::min(v + step, target) : std::max(v - step, target);
}
// Frame-rate independent exponential smoothing: fraction of distance covered per second ~ rate.
inline float damp(float a, float b, float rate, float dt) { return lerp(a, b, 1.f - std::exp(-rate * dt)); }
inline Vec2 damp(Vec2 a, Vec2 b, float rate, float dt) { return lerp(a, b, 1.f - std::exp(-rate * dt)); }
inline float wrapAngle(float a) { while (a > PI) a -= TAU; while (a < -PI) a += TAU; return a; }
inline float lerpAngle(float a, float b, float t) { return a + wrapAngle(b - a) * t; }
inline float smoothstep(float e0, float e1, float x) { float t = clamp01((x - e0) / (e1 - e0)); return t * t * (3 - 2 * t); }

namespace ease {
inline float inQuad(float t) { return t * t; }
inline float outQuad(float t) { return 1 - (1 - t) * (1 - t); }
inline float inOutQuad(float t) { return t < .5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2.f) / 2; }
inline float outCubic(float t) { return 1 - std::pow(1 - t, 3.f); }
inline float inCubic(float t) { return t * t * t; }
inline float inOutCubic(float t) { return t < .5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3.f) / 2; }
inline float outExpo(float t) { return t >= 1 ? 1 : 1 - std::pow(2.f, -10 * t); }
inline float outBack(float t) { const float c1 = 1.70158f, c3 = c1 + 1; return 1 + c3 * std::pow(t - 1, 3.f) + c1 * std::pow(t - 1, 2.f); }
}

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    float right() const { return x + w; }
    float bottom() const { return y + h; }
    Vec2 center() const { return {x + w * .5f, y + h * .5f}; }
    bool overlaps(const Rect& o) const { return x < o.x + o.w && x + w > o.x && y < o.y + o.h && y + h > o.y; }
    bool contains(Vec2 p) const { return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h; }
};

// PCG32: small, fast, deterministic. Used for gameplay (seeded) and visuals.
struct Rng {
    uint64_t state = 0x853c49e6748fea9bULL, inc = 0xda3e39cb94b95bdbULL;
    Rng() = default;
    explicit Rng(uint64_t seed) { seedWith(seed); }
    void seedWith(uint64_t seed) { state = 0; inc = (seed << 1u) | 1u; next(); state += seed; next(); }
    uint32_t next() {
        uint64_t old = state;
        state = old * 6364136223846793005ULL + inc;
        uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = (uint32_t)(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }
    float uniform() { return (next() >> 8) * (1.0f / 16777216.0f); }
    float range(float a, float b) { return a + (b - a) * uniform(); }
    int irange(int a, int b) { return a + (int)(next() % (uint32_t)(b - a + 1)); }
    bool chance(float p) { return uniform() < p; }
};

inline uint32_t hash32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x;
}
inline float hashf(int a, int b = 0) { return (hash32((uint32_t)a * 73856093u ^ hash32((uint32_t)b * 19349663u)) & 0xffffff) / 16777216.0f; }
// 1D value noise in [0,1], smooth.
inline float noise1(float x, int seed = 0) {
    int i = (int)std::floor(x); float f = x - i; float u = f * f * (3 - 2 * f);
    return lerp(hashf(i, seed), hashf(i + 1, seed), u);
}
inline float fbm1(float x, int seed = 0, int oct = 4) {
    float v = 0, a = .5f, tot = 0;
    for (int i = 0; i < oct; i++) { v += noise1(x, seed + i * 31) * a; tot += a; x *= 2.03f; a *= .5f; }
    return v / tot;
}

} // namespace sv
