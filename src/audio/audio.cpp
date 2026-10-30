#include "audio/audio.h"
#include "core/math.h"
#include "core/util.h"
#include <cstring>
#include <array>

namespace sv {

namespace {
constexpr float SR = (float)kSampleRate;

struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set(int type, float f, float q) {  // 0 lowpass, 1 highpass, 2 bandpass
        f = clampf(f, 10.f, SR * 0.45f);
        float w = TAU * f / SR, c = std::cos(w), s = std::sin(w), al = s / (2 * q);
        float a0 = 1 + al;
        if (type == 0) { b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2; }
        else if (type == 1) { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; }
        else { b0 = al; b1 = 0; b2 = -al; }
        a1 = -2 * c; a2 = 1 - al;
        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    }
    float process(float x) {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

inline float env(float t, float atk, float dec) { return t < atk ? t / atk : std::exp(-(t - atk) / dec); }
inline float semis(float s) { return std::pow(2.f, s / 12.f); }

// Additive metallic/bell voice: partial ratios with individual decays.
void addPartials(std::vector<float>& out, float f0, const std::vector<std::array<float, 3>>& partials, float gain, float startT = 0) {
    for (size_t i = 0; i < out.size(); i++) {
        float t = i / SR - startT;
        if (t < 0) continue;
        float v = 0;
        for (auto& p : partials) v += std::sin(TAU * f0 * p[0] * t) * p[1] * std::exp(-t / p[2]);
        out[i] += v * gain * std::min(1.f, t * 2000.f);
    }
}

void normalize(std::vector<float>& b, float peak) {
    float m = 1e-6f;
    for (float v : b) m = std::max(m, std::abs(v));
    for (float& v : b) v *= peak / m;
}

// Plucked string via Karplus–Strong (used offline for pickups and alerts).
void ksPluck(std::vector<float>& out, float f, float gain, float bright, Rng& rng, float startT = 0, float decay = 0.996f) {
    int n = std::max(2, (int)(SR / f));
    std::vector<float> line(n);
    for (auto& v : line) v = rng.range(-1, 1);
    for (int k = 0; k < 2; k++) for (int i = 1; i < n; i++) line[i] = lerp(line[i], line[i - 1], 1 - bright);
    int idx = 0;
    size_t start = (size_t)(startT * SR);
    for (size_t i = start; i < out.size(); i++) {
        int nx = (idx + 1) % n;
        float y = line[idx];
        line[idx] = decay * 0.5f * (line[idx] + line[nx]);
        idx = nx;
        out[i] += y * gain;
    }
}
} // namespace

std::vector<float> synthesizeSfx(Sfx s, int variant) {
    Rng rng(1000 + (int)s * 97 + variant * 13);
    auto buf = [](float secs) { return std::vector<float>((size_t)(secs * SR), 0.f); };
    std::vector<float> b;
    Biquad bq, bq2;
    float vr = 1.f + (variant - 1) * 0.06f;
    switch (s) {
    case Sfx::Slash: case Sfx::SlashHeavy: case Sfx::Swoosh: {
        bool heavy = s == Sfx::SlashHeavy, soft = s == Sfx::Swoosh;
        float dur = heavy ? 0.32f : 0.22f;
        b = buf(dur);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR, p = t / dur;
            float fc = lerp(heavy ? 4200.f : 6500.f, heavy ? 700.f : 1400.f, ease::outQuad(p)) * vr;
            if (i % 32 == 0) bq.set(2, fc, heavy ? 1.2f : 1.8f);
            float n = bq.process(rng.range(-1, 1));
            float e = env(t, soft ? 0.04f : 0.012f, dur * 0.28f);
            float shing = soft ? 0 : std::sin(TAU * lerp(2400.f, 1100.f, p) * vr * t) * 0.08f * std::exp(-t * 18);
            b[i] = n * e + shing;
        }
        normalize(b, soft ? 0.35f : (heavy ? 0.9f : 0.75f));
        break;
    }
    case Sfx::Clang: case Sfx::Deflect: {
        bool def = s == Sfx::Deflect;
        b = buf(def ? 1.6f : 0.7f);
        float f0 = (def ? 1180.f : 980.f) * vr;
        addPartials(b, f0, {{1.0f, 0.5f, 0.35f}, {1.56f, 0.4f, 0.28f}, {2.38f, 0.35f, 0.2f}, {3.55f, 0.25f, 0.12f},
                            {4.71f, 0.18f, 0.08f}, {def ? 6.9f : 5.2f, def ? 0.25f : 0.1f, def ? 0.6f : 0.05f}}, 0.4f);
        for (size_t i = 0; i < b.size() && i < SR * 0.03f; i++) b[i] += rng.range(-1, 1) * std::exp(-(i / SR) * 160) * 0.8f;
        if (def) for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] += std::sin(TAU * 70 * t) * std::exp(-t * 14) * 0.6f; }
        normalize(b, def ? 0.95f : 0.7f);
        break;
    }
    case Sfx::Hit: case Sfx::HitHeavy: case Sfx::Kill: case Sfx::Hurt: case Sfx::Thud: {
        bool heavy = s != Sfx::Hit;
        b = buf(heavy ? 0.45f : 0.25f);
        bq.set(0, s == Sfx::Hurt ? 2400.f : 1600.f, 0.8f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            float f = lerp(heavy ? 110.f : 150.f, 45.f, clamp01(t * 8)) * vr;
            float thump = std::sin(TAU * f * t) * std::exp(-t * (heavy ? 9.f : 16.f));
            float crunch = bq.process(rng.range(-1, 1)) * std::exp(-t * (heavy ? 22.f : 35.f));
            b[i] = thump * 0.9f + crunch * (s == Sfx::Thud ? 0.3f : 0.9f);
        }
        if (s == Sfx::Kill) {  // the drawn-out cut of a killing blow
            auto sl = synthesizeSfx(Sfx::SlashHeavy, variant);
            for (size_t i = 0; i < b.size() && i < sl.size(); i++) b[i] += sl[i] * 0.6f;
        }
        normalize(b, s == Sfx::Thud ? 0.6f : 0.9f);
        break;
    }
    case Sfx::Step: {
        b = buf(0.07f);
        bq.set(0, 900.f * vr, 0.7f);
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] = bq.process(rng.range(-1, 1)) * env(t, 0.002f, 0.018f); }
        normalize(b, 0.3f);
        break;
    }
    case Sfx::Land: {
        b = buf(0.18f);
        bq.set(0, 600.f, 0.7f);
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] = bq.process(rng.range(-1, 1)) * env(t, 0.002f, 0.04f) + std::sin(TAU * 80 * t) * std::exp(-t * 25) * 0.6f; }
        normalize(b, 0.45f);
        break;
    }
    case Sfx::Jump: {
        b = buf(0.16f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            if (i % 32 == 0) bq.set(2, lerp(500.f, 1600.f, t / 0.16f) * vr, 1.4f);
            b[i] = bq.process(rng.range(-1, 1)) * env(t, 0.02f, 0.05f);
        }
        normalize(b, 0.3f);
        break;
    }
    case Sfx::Dash: {
        b = buf(0.5f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            if (i % 32 == 0) bq.set(2, lerp(3000.f, 500.f, clamp01(t / 0.3f)), 1.0f);
            float wh = bq.process(rng.range(-1, 1)) * env(t, 0.015f, 0.09f);
            float chime = (std::sin(TAU * 1568 * vr * t) + 0.5f * std::sin(TAU * 2349 * vr * t)) * smoothstep(0, 0.12f, t) * std::exp(-t * 7) * 0.18f;
            b[i] = wh + chime;
        }
        normalize(b, 0.65f);
        break;
    }
    case Sfx::Grapple: {
        b = buf(0.4f);
        for (int k = 0; k < 7; k++) {
            float st = k * 0.035f + rng.range(0, 0.01f);
            addPartials(b, rng.range(2800, 4200), {{1, 1, 0.02f}, {1.7f, 0.5f, 0.015f}}, 0.25f * (1 - k / 9.f), st);
        }
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; if (i % 32 == 0) bq.set(2, lerp(1200.f, 4000.f, clamp01(t / 0.2f)), 2); b[i] += bq.process(rng.range(-1, 1)) * env(t, 0.01f, 0.06f) * 0.6f; }
        normalize(b, 0.55f);
        break;
    }
    case Sfx::Throw: case Sfx::Arrow: {
        b = buf(0.3f);
        bool arrow = s == Sfx::Arrow;
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            if (i % 32 == 0) bq.set(2, (arrow ? 2200.f : 4500.f) * vr, 3);
            b[i] = bq.process(rng.range(-1, 1)) * env(t, 0.01f, 0.08f) + std::sin(TAU * lerp(arrow ? 1900.f : 3200.f, 1500.f, t / 0.3f) * t) * env(t, 0.005f, 0.07f) * 0.25f;
        }
        normalize(b, 0.4f);
        break;
    }
    case Sfx::ShurikenHit: {
        b = buf(0.25f);
        addPartials(b, 2600 * vr, {{1, 1, 0.05f}, {1.48f, 0.6f, 0.04f}, {2.2f, 0.4f, 0.03f}}, 0.5f);
        for (size_t i = 0; i < 400; i++) b[i] += rng.range(-1, 1) * std::exp(-(i / SR) * 300) * 0.6f;
        normalize(b, 0.45f);
        break;
    }
    case Sfx::Execute: {
        b = buf(1.8f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            b[i] = std::sin(TAU * lerp(70.f, 38.f, clamp01(t)) * t) * std::exp(-t * 2.2f) * 0.9f;
            if (i % 32 == 0) bq.set(2, lerp(200.f, 5000.f, clamp01(t / 0.35f)), 1.2f);
            if (t < 0.35f) b[i] += bq.process(rng.range(-1, 1)) * smoothstep(0, 0.35f, t) * 0.5f;
        }
        { auto sl = synthesizeSfx(Sfx::Deflect, 1); size_t off = (size_t)(0.33f * SR); for (size_t i = 0; i + off < b.size() && i < sl.size(); i++) b[i + off] += sl[i] * 0.7f; }
        { auto k = synthesizeSfx(Sfx::Kill, 1); size_t off = (size_t)(0.35f * SR); for (size_t i = 0; i + off < b.size() && i < k.size(); i++) b[i + off] += k[i] * 0.8f; }
        normalize(b, 1.0f);
        break;
    }
    case Sfx::Ember: case Sfx::Ignite: {
        b = buf(s == Sfx::Ignite ? 1.2f : 0.7f);
        bq.set(0, 900, 0.7f); bq2.set(2, 3000, 1.5f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            float roar = bq.process(rng.range(-1, 1)) * env(t, 0.06f, 0.25f);
            float crackle = rng.chance(0.0015f) ? rng.range(-1, 1) * 3 : 0;
            b[i] = roar + bq2.process(crackle) * env(t, 0.02f, 0.4f) + std::sin(TAU * 95 * t) * env(t, 0.05f, 0.2f) * 0.4f;
        }
        normalize(b, 0.75f);
        break;
    }
    case Sfx::Heal: {
        b = buf(1.0f);
        for (int k = 0; k < 4; k++) {
            float st = k * 0.09f;
            for (size_t i = (size_t)(st * SR); i < b.size(); i++) {
                float t = i / SR - st;
                b[i] += std::sin(TAU * (220 + k * 40 + t * 600) * t) * env(t, 0.005f, 0.05f) * 0.4f;
            }
        }
        addPartials(b, 1318, {{1, 1, 0.6f}, {2.0f, 0.3f, 0.4f}, {3.01f, 0.15f, 0.3f}}, 0.25f, 0.35f);
        normalize(b, 0.55f);
        break;
    }
    case Sfx::Death: {
        b = buf(3.0f);
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] = std::sin(TAU * 55 * t) * std::exp(-t * 1.4f) * 0.8f; }
        addPartials(b, 220, {{0.5f, 0.6f, 2.0f}, {1.0f, 0.8f, 1.6f}, {1.19f, 0.4f, 1.2f}, {1.51f, 0.3f, 1.0f}, {2.0f, 0.3f, 0.8f}}, 0.4f, 0.1f);
        normalize(b, 0.8f);
        break;
    }
    case Sfx::UiMove: {
        b = buf(0.09f);
        addPartials(b, 880 * vr, {{1, 1, 0.025f}, {2.03f, 0.3f, 0.012f}}, 0.6f);
        normalize(b, 0.3f);
        break;
    }
    case Sfx::UiBack: {
        b = buf(0.12f);
        addPartials(b, 520, {{1, 1, 0.035f}, {2.1f, 0.3f, 0.015f}}, 0.6f);
        normalize(b, 0.3f);
        break;
    }
    case Sfx::UiConfirm: {
        b = buf(0.9f);
        addPartials(b, 1760, {{1, 1, 0.35f}, {2.76f, 0.4f, 0.2f}, {5.4f, 0.2f, 0.1f}}, 0.5f);
        addPartials(b, 880, {{1, 0.6f, 0.04f}}, 0.4f);
        normalize(b, 0.38f);
        break;
    }
    case Sfx::Bell: {
        b = buf(2.5f);
        addPartials(b, 620, {{1, 1, 1.2f}, {1.002f, 0.6f, 1.1f}, {2.0f, 0.5f, 0.8f}, {2.76f, 0.4f, 0.6f}, {3.9f, 0.25f, 0.4f}, {5.4f, 0.15f, 0.25f}}, 0.4f);
        normalize(b, 0.75f);
        break;
    }
    case Sfx::ShrineBell: {
        b = buf(6.0f);
        // Bonshō: low fundamental, strongly beating partial pairs, a soft wooden strike.
        addPartials(b, 98, {{0.5f, 0.5f, 4.5f}, {1.0f, 1.0f, 4.0f}, {1.004f, 0.8f, 3.8f}, {1.18f, 0.5f, 2.6f}, {1.5f, 0.45f, 2.2f},
                            {2.0f, 0.35f, 1.8f}, {2.006f, 0.3f, 1.7f}, {2.51f, 0.25f, 1.3f}, {3.0f, 0.2f, 1.0f}, {4.07f, 0.12f, 0.7f}}, 0.35f);
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] += std::sin(TAU * 60 * t) * std::exp(-t * 30) * 0.5f; }
        normalize(b, 0.85f);
        break;
    }
    case Sfx::Pickup: {
        b = buf(1.2f);
        const float notes[] = {0, 5, 7, 12};
        for (int k = 0; k < 4; k++) ksPluck(b, 587.33f * semis(notes[k]), 0.4f, 0.6f, rng, k * 0.06f);
        normalize(b, 0.45f);
        break;
    }
    case Sfx::Alert: {
        b = buf(0.6f);
        ksPluck(b, 880, 0.6f, 0.9f, rng, 0, 0.993f);
        ksPluck(b, 932.3f, 0.4f, 0.9f, rng, 0.0f, 0.993f);
        normalize(b, 0.5f);
        break;
    }
    case Sfx::Perilous: {
        b = buf(1.4f);
        addPartials(b, 140, {{1, 1, 0.9f}, {1.41f, 0.6f, 0.7f}, {2.13f, 0.5f, 0.5f}, {2.97f, 0.3f, 0.4f}}, 0.4f);
        addPartials(b, 1650, {{1, 1, 0.3f}, {1.5f, 0.5f, 0.2f}}, 0.25f);
        for (size_t i = 0; i < b.size(); i++) b[i] *= smoothstep(0, 0.02f, i / SR);
        normalize(b, 0.8f);
        break;
    }
    case Sfx::Break: {
        b = buf(0.6f);
        bq.set(2, 1800, 0.9f);
        for (int k = 0; k < 6; k++) {
            float st = rng.range(0, 0.12f);
            for (size_t i = (size_t)(st * SR); i < b.size(); i++) { float t = i / SR - st; b[i] += rng.range(-1, 1) * std::exp(-t * 60) * 0.5f; }
        }
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] = bq.process(b[i]) + std::sin(TAU * 90 * t) * std::exp(-t * 12) * 0.6f; }
        normalize(b, 0.7f);
        break;
    }
    case Sfx::Gate: {
        b = buf(2.2f);
        bq.set(0, 220, 0.8f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            b[i] = bq.process(rng.range(-1, 1)) * smoothstep(0, 0.2f, t) * smoothstep(2.2f, 1.5f, t) * 1.5f;
            if (t > 1.6f) b[i] += std::sin(TAU * 70 * t) * std::exp(-(t - 1.6f) * 10) * 0.8f;
        }
        normalize(b, 0.7f);
        break;
    }
    case Sfx::BowDraw: {
        b = buf(0.5f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            float saw = std::fmod(t * lerp(90.f, 140.f, t * 2), 1.f) * 2 - 1;
            if (i % 32 == 0) bq.set(2, 1400, 4);
            b[i] = bq.process(saw * (0.5f + 0.5f * rng.range(0, 1))) * smoothstep(0, 0.1f, t) * smoothstep(0.5f, 0.35f, t);
        }
        normalize(b, 0.3f);
        break;
    }
    default: b = buf(0.05f); break;
    }
    return b;
}

// ======================================================================= real-time synth

struct KotoVoice {
    std::vector<float> line; int idx = 0; float gain = 0, pan = 0, decay = 0.997f; bool active = false; int age = 0;
};

struct TaikoVoice { float t = 0, f0 = 60, gain = 0, ph = 0; bool rim = false, active = false; Biquad bp; };

struct SfxVoice { const std::vector<float>* buf = nullptr; double pos = 0; float rate = 1, gainL = 0, gainR = 0; bool active = false; };

struct Comb {
    std::vector<float> b; int i = 0; float store = 0;
    float process(float x, float fb, float damp) {
        float y = b[i];
        store = y * (1 - damp) + store * damp;
        b[i] = x + store * fb;
        if (++i >= (int)b.size()) i = 0;
        return y;
    }
};
struct Allpass {
    std::vector<float> b; int i = 0;
    float process(float x) {
        float y = b[i];
        b[i] = x + y * 0.5f;
        if (++i >= (int)b.size()) i = 0;
        return y - x;
    }
};

struct Cmd { enum T { Play, Music, Intensity, Amb, Vol, Pause, Lowpass } t; Sfx sfx{}; float a = 0, b = 0, c = 0, d = 0; MusicMode mode{}; };

class Synth {
public:
    Synth() {
        for (int s = 0; s < (int)Sfx::Count; s++)
            for (int v = 0; v < 3; v++) bank[s][v] = synthesizeSfx((Sfx)s, v);
        const int combL[4] = {1116, 1277, 1422, 1557}, ap[2] = {556, 341};
        for (int ch = 0; ch < 2; ch++) {
            for (int k = 0; k < 4; k++) combs[ch][k].b.assign((int)((combL[k] + ch * 23) * SR / 44100.f), 0.f);
            for (int k = 0; k < 2; k++) aps[ch][k].b.assign((int)((ap[k] + ch * 23) * SR / 44100.f), 0.f);
        }
        rng.seedWith(77);
    }

    void push(const Cmd& c) { std::lock_guard<std::mutex> lk(mtx); queue.push_back(c); }

    void render(float* out, int frames) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            pending.swap(queue);
        }
        for (auto& c : pending) apply(c);
        pending.clear();
        for (int i = 0; i < frames; i++) {
            float mL = 0, mR = 0, sL = 0, sR = 0;
            musicSample(mL, mR);
            ambienceSample(sL, sR);
            float fxL = 0, fxR = 0;
            for (auto& v : voices) {
                if (!v.active) continue;
                size_t p = (size_t)v.pos;
                if (p + 1 >= v.buf->size()) { v.active = false; continue; }
                float fr = (float)(v.pos - p);
                float x = lerp((*v.buf)[p], (*v.buf)[p + 1], fr);
                fxL += x * v.gainL; fxR += x * v.gainR;
                v.pos += v.rate;
            }
            float music = musicVol * musicDuck;
            float dryL = mL * music + (sL + fxL) * sfxVol;
            float dryR = mR * music + (sR + fxR) * sfxVol;
            float sendL = mL * music * 0.55f + fxL * sfxVol * 0.22f;
            float sendR = mR * music * 0.55f + fxR * sfxVol * 0.22f;
            float wetL = 0, wetR = 0;
            for (int k = 0; k < 4; k++) { wetL += combs[0][k].process(sendL * 0.25f, 0.86f, 0.25f); wetR += combs[1][k].process(sendR * 0.25f, 0.86f, 0.25f); }
            for (int k = 0; k < 2; k++) { wetL = aps[0][k].process(wetL); wetR = aps[1][k].process(wetR); }
            float L = (dryL + wetL * 0.5f) * master, R = (dryR + wetR * 0.5f) * master;
            // master low-pass (pause muffling / slow-motion)
            lpCur = lerp(lpCur, lpTarget, 0.0004f);
            float a = lerp(1.0f, 0.06f, lpCur);
            lpL += a * (L - lpL); lpR += a * (R - lpR);
            L = lpL; R = lpR;
            // soft limiter
            out[i * 2] = std::tanh(L * 0.9f);
            out[i * 2 + 1] = std::tanh(R * 0.9f);
        }
    }

private:
    void apply(const Cmd& c) {
        switch (c.t) {
        case Cmd::Play: {
            SfxVoice* slot = nullptr;
            for (auto& v : voices) if (!v.active) { slot = &v; break; }
            if (!slot) { slot = &voices[stealIdx]; stealIdx = (stealIdx + 1) % voices.size(); }
            int var = rng.irange(0, 2);
            slot->buf = &bank[(int)c.sfx][var];
            slot->pos = 0; slot->rate = c.b; slot->active = true;
            float pan = clampf(c.c, -1, 1);
            slot->gainL = c.a * std::sqrt(0.5f * (1 - pan));
            slot->gainR = c.a * std::sqrt(0.5f * (1 + pan));
            break;
        }
        case Cmd::Music:
            if (c.mode != mode) { mode = c.mode; phraseCounter = 0; }
            break;
        case Cmd::Intensity: intensity = clamp01(c.a); break;
        case Cmd::Amb: amb = {c.a, c.b, c.c, c.d}; break;
        case Cmd::Vol: master = c.a; musicVol = c.b; sfxVol = c.c; break;
        case Cmd::Pause: lpTarget = c.a > 0.5f ? 0.85f : lowpassReq; paused = c.a > 0.5f; break;
        case Cmd::Lowpass: lowpassReq = clamp01(c.a); if (!paused) lpTarget = lowpassReq; break;
        }
    }

    // ---------------- music
    static constexpr int kScale[5] = {0, 1, 5, 7, 8};   // In (Miyako-bushi) on D
    float degFreq(int deg) const {
        int oct = (int)std::floor(deg / 5.0f);
        int d = deg - oct * 5;
        return 146.83f * semis((float)(oct * 12 + kScale[d]));
    }

    void pluck(float f, float gain, float pan, float bright = 0.55f, float decay = 0.9965f) {
        KotoVoice* v = nullptr;
        for (auto& k : koto) if (!k.active) { v = &k; break; }
        if (!v) { v = &koto[0]; for (auto& k : koto) if (k.age > v->age) v = &k; }
        int n = std::max(2, (int)(SR / f));
        v->line.assign(n, 0.f);
        for (auto& x : v->line) x = rng.range(-1, 1);
        for (int pass = 0; pass < 2; pass++) for (int i = 1; i < n; i++) v->line[i] = lerp(v->line[i], v->line[i - 1], 1 - bright);
        v->idx = 0; v->gain = gain; v->pan = pan; v->decay = decay; v->active = true; v->age = 0;
    }
    void drum(float f0, float gain, bool rim) {
        TaikoVoice* v = nullptr;
        for (auto& d : taiko) if (!d.active) { v = &d; break; }
        if (!v) v = &taiko[rng.irange(0, (int)taiko.size() - 1)];
        v->t = 0; v->ph = 0; v->f0 = f0; v->gain = gain; v->rim = rim; v->active = true;
        v->bp = Biquad(); v->bp.set(2, rim ? 2600.f : 300.f, rim ? 2.f : 0.8f);
    }
    void shaku(float f, float dur) { shF = f; shTarget = 1; shT = 0; shDur = dur; shBend = -0.06f; }

    void onStep() {
        int s = step % 16;
        int bar = step / 16;
        float I = intensity;
        auto deg = [&](int lo, int hi) {
            walk += rng.irange(-2, 2);
            if (walk < lo) walk = lo + 1;
            if (walk > hi) walk = hi - 1;
            return walk;
        };
        switch (mode) {
        case MusicMode::Silent: break;
        case MusicMode::Title: case MusicMode::Explore: case MusicMode::Shrine: {
            bool shrine = mode == MusicMode::Shrine;
            static const uint16_t rhythms[] = {0b1000001000100000, 0b1000100000001000, 0b1000000010100000, 0b1010000010000000, 0b1000000000001000};
            if (s == 0) currentRhythm = rhythms[rng.irange(0, 4)];
            if (currentRhythm & (1 << (15 - s))) {
                if (!(shrine && rng.chance(0.4f))) {
                    int d = deg(shrine ? 6 : 2, shrine ? 13 : 10);
                    pluck(degFreq(d), 0.32f, rng.range(-0.5f, 0.5f), 0.5f);
                    if (s == 0 && rng.chance(0.5f)) pluck(degFreq(d - 3), 0.2f, -0.3f, 0.4f);
                }
            }
            if (s == 0 && bar % 4 == 0) pluck(degFreq(-5), 0.3f, 0, 0.35f, 0.998f);
            if ((mode == MusicMode::Title || shrine) && s == 8 && bar % 4 == 1 && rng.chance(0.7f)) shaku(degFreq(deg(5, 11)), rng.range(2.0f, 3.5f));
            if (mode == MusicMode::Explore && I > 0.2f && s % 8 == 0) drum(58, 0.25f * I, false);
            break;
        }
        case MusicMode::Tension: {
            if (s == 0 || s == 3) drum(52, s == 0 ? 0.45f : 0.25f, false);
            if (s == 8 && rng.chance(0.6f)) pluck(degFreq(deg(0, 6)), 0.28f, rng.range(-0.4f, 0.4f), 0.45f);
            if (s == 0 && bar % 2 == 0) pluck(degFreq(-4), 0.25f, 0, 0.3f, 0.998f);
            break;
        }
        case MusicMode::Combat: case MusicMode::Boss: {
            bool boss = mode == MusicMode::Boss;
            static const uint16_t big = 0b1000000010100000, bigBoss = 0b1001001010100010;
            static const uint16_t rim = 0b0010100000101010;
            uint16_t bp = boss ? bigBoss : big;
            if (bp & (1 << (15 - s))) drum(boss ? 48 : 54, 0.75f, false);
            if ((rim & (1 << (15 - s))) && I > 0.3f) drum(0, 0.25f, true);
            // driving low ostinato
            static const int ost[8] = {0, 0, 3, 0, 4, 3, 1, 0};
            if (s % 2 == 0) pluck(degFreq(ost[(s / 2 + bar) % 8] - 5), 0.26f, -0.2f, 0.6f, 0.993f);
            if (boss && s % 2 == 1 && I > 0.6f) pluck(degFreq(ost[(s / 2) % 8]), 0.12f, 0.3f, 0.7f, 0.99f);
            if (s == 0 && bar % 2 == 0 && boss) shaku(degFreq(deg(6, 12)), 2.6f);
            if (s == 0 && bar % 4 == 2 && !boss && rng.chance(0.5f)) shaku(degFreq(deg(5, 10)), 2.0f);
            break;
        }
        case MusicMode::Requiem: {
            if (s == 0 && bar % 2 == 0) shaku(degFreq(deg(4, 10)), 3.2f);
            if (s == 0 && bar % 4 == 0) pluck(degFreq(-5), 0.3f, 0, 0.3f, 0.998f);
            break;
        }
        }
    }

    float tempoFor(MusicMode m) const {
        switch (m) { case MusicMode::Combat: return 96; case MusicMode::Boss: return 108; case MusicMode::Shrine: case MusicMode::Requiem: return 64; default: return 76; }
    }

    void musicSample(float& L, float& R) {
        float stepLen = SR * 60.f / tempoFor(mode) / 4.f;
        if (++stepPos >= stepLen) { stepPos = 0; step++; onStep(); }
        // layer gains glide toward the mode's mix
        float droneT = 0.0f;
        switch (mode) {
        case MusicMode::Silent: droneT = 0; break;
        case MusicMode::Title: droneT = 0.8f; break;
        case MusicMode::Explore: droneT = 0.55f; break;
        case MusicMode::Tension: droneT = 0.9f; break;
        case MusicMode::Combat: droneT = 0.5f; break;
        case MusicMode::Boss: droneT = 0.8f; break;
        case MusicMode::Shrine: droneT = 1.0f; break;
        case MusicMode::Requiem: droneT = 0.9f; break;
        }
        droneGain = lerp(droneGain, droneT, 0.00003f);
        dronePhase += 1.f / SR;
        float t = dronePhase;
        bool dark = mode == MusicMode::Tension || mode == MusicMode::Boss;
        float d1 = dark ? 73.42f : 146.83f, d2 = dark ? 77.78f : 220.f, d3 = dark ? 110.f : 329.63f;
        droneLfo = 0.5f + 0.5f * std::sin(t * 0.21f);
        float drone = (std::sin(TAU * d1 * t) * 0.5f + std::sin(TAU * d1 * 1.003f * t) * 0.3f + std::sin(TAU * d2 * t) * 0.25f * droneLfo +
                       std::sin(TAU * d3 * t) * 0.12f * (1 - droneLfo) + std::sin(TAU * d3 * 1.5f * t) * 0.05f);
        drone *= droneGain * 0.11f;
        float l = drone, r = drone;
        for (auto& v : koto) {
            if (!v.active) continue;
            int n = (int)v.line.size(), nx = (v.idx + 1) % n;
            float y = v.line[v.idx];
            v.line[v.idx] = v.decay * 0.5f * (v.line[v.idx] + v.line[nx]);
            v.idx = nx;
            if (++v.age > SR * 6) v.active = false;
            float g = v.gain * 0.6f;
            l += y * g * (1 - v.pan) * 0.7f; r += y * g * (1 + v.pan) * 0.7f;
        }
        for (auto& d : taiko) {
            if (!d.active) continue;
            d.t += 1 / SR;
            float x;
            if (d.rim) x = d.bp.process(rng.range(-1, 1)) * std::exp(-d.t * 70) * 1.5f;
            else {
                float f = d.f0 * (1 + 1.4f * std::exp(-d.t * 28));
                d.ph += TAU * f / SR;
                x = std::sin(d.ph) * std::exp(-d.t * 5.5f) + d.bp.process(rng.range(-1, 1)) * std::exp(-d.t * 45) * 0.5f;
            }
            if (d.t > 1.5f) d.active = false;
            l += x * d.gain * 0.5f; r += x * d.gain * 0.5f;
        }
        // shakuhachi
        if (shTarget > 0 || shAmp > 0.0001f) {
            shT += 1 / SR;
            if (shT > shDur) shTarget = 0;
            shAmp = lerp(shAmp, shTarget, shTarget > 0 ? 0.00012f : 0.00006f);
            shBend = lerp(shBend, 0.f, 0.00008f);
            float vib = 1 + std::sin(shT * TAU * 5.2f) * 0.006f * smoothstep(0.4f, 1.2f, shT);
            float f = shF * vib * (1 + shBend);
            shPh += TAU * f / SR;
            if (shPh > TAU) shPh -= TAU;
            if (std::abs(shBpF - f) > 4) { shBp.set(2, f * 2, 3.5f); shBpF = f; }
            float tone = std::sin(shPh) + 0.22f * std::sin(2 * shPh) + 0.07f * std::sin(3 * shPh);
            float breath = shBp.process(rng.range(-1, 1)) * (0.6f + 0.4f * (1 - shAmp));
            float x = (tone * 0.5f + breath * 0.35f) * shAmp * 0.2f;
            l += x * 0.8f; r += x;
        }
        L = l; R = r;
    }

    // ---------------- ambience
    void ambienceSample(float& L, float& R) {
        ambT += 1 / SR;
        float windMod = 0.5f + 0.5f * fbm1(ambT * 0.15f, 3, 3);
        float cutoff = 0.004f + 0.02f * windMod;
        windL += cutoff * (rng.range(-1, 1) - windL);
        windR += cutoff * (rng.range(-1, 1) - windR);
        float w = amb.wind * (0.6f + 0.8f * windMod) * 1.6f;
        L = windL * w; R = windR * w;
        if (amb.fire > 0) {
            fireLow += 0.01f * (rng.range(-1, 1) - fireLow);
            if (rng.chance(0.0009f * amb.fire)) { crackle = rng.range(0.3f, 1.0f); cracklePan = rng.range(-0.8f, 0.8f); }
            crackle *= 0.9985f;
            float c = (crackle > 0.01f ? rng.range(-1, 1) * crackle * crackle : 0) * 0.35f;
            L += (fireLow * 1.2f) * amb.fire * 0.6f + c * (1 - cracklePan) * amb.fire;
            R += (fireLow * 1.2f) * amb.fire * 0.6f + c * (1 + cracklePan) * amb.fire;
        }
        if (amb.crickets > 0) {
            float gate = noise1(ambT * 2.5f, 9) > 0.55f ? 1.f : 0.f;
            cricketEnv = lerp(cricketEnv, gate, 0.002f);
            float pulse = std::sin(ambT * TAU * 32) > 0.2f ? 1.f : 0.f;
            float c = std::sin(ambT * TAU * 4400) * pulse * cricketEnv * 0.02f * amb.crickets;
            float c2 = std::sin(ambT * TAU * 4900) * (std::sin(ambT * TAU * 27 + 1) > 0.3f ? 1.f : 0.f) * (1 - cricketEnv) * 0.012f * amb.crickets;
            L += c; R += c2;
        }
        if (amb.rumble > 0) { rumbleV += 0.002f * (rng.range(-1, 1) - rumbleV); L += rumbleV * amb.rumble * 3; R += rumbleV * amb.rumble * 3; }
    }

public:
    std::vector<float> bank[(int)Sfx::Count][3];
private:
    std::mutex mtx;
    std::vector<Cmd> queue, pending;
    std::array<SfxVoice, 48> voices{};
    size_t stealIdx = 0;
    Rng rng;
    float master = 0.8f, musicVol = 0.6f, sfxVol = 0.9f, musicDuck = 1.0f;
    float lpCur = 0, lpTarget = 0, lpL = 0, lpR = 0, lowpassReq = 0;
    bool paused = false;
    Comb combs[2][4];
    Allpass aps[2][2];
    // music state
    MusicMode mode = MusicMode::Silent;
    float intensity = 0;
    long step = 0; float stepPos = 0; int walk = 5; uint16_t currentRhythm = 0; int phraseCounter = 0;
    std::array<KotoVoice, 16> koto{};
    std::array<TaikoVoice, 8> taiko{};
    float droneGain = 0, dronePhase = 0, droneLfo = 0;
    float shF = 440, shTarget = 0, shAmp = 0, shT = 0, shDur = 0, shPh = 0, shBend = 0, shBpF = 0;
    Biquad shBp;
    // ambience
    AmbienceParams amb;
    float ambT = 0, windL = 0, windR = 0, fireLow = 0, crackle = 0, cracklePan = 0, cricketEnv = 0, rumbleV = 0;

    friend class Audio;
};

// ======================================================================= Audio

Audio::Audio() : m_synth(std::make_unique<Synth>()) {}
Audio::~Audio() { shutdown(); }

bool Audio::init() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) { logWarn("Audio unavailable: %s", SDL_GetError()); return false; }
    SDL_AudioSpec spec{SDL_AUDIO_F32, 2, kSampleRate};
    m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &Audio::callback, this);
    if (!m_stream) { logWarn("No audio device: %s", SDL_GetError()); return false; }
    SDL_ResumeAudioStreamDevice(m_stream);
    logInfo("Audio: %d Hz stereo, procedural synthesis", kSampleRate);
    return true;
}

void Audio::shutdown() {
    if (m_stream) { SDL_DestroyAudioStream(m_stream); m_stream = nullptr; }
}

void SDLCALL Audio::callback(void* user, SDL_AudioStream* stream, int additional, int) {
    auto* self = (Audio*)user;
    int frames = additional / (int)(sizeof(float) * 2);
    if (frames <= 0) return;
    self->m_buf.resize(frames * 2);
    self->m_synth->render(self->m_buf.data(), frames);
    SDL_PutAudioStreamData(stream, self->m_buf.data(), frames * 2 * (int)sizeof(float));
}

void Audio::renderOffline(float* out, int frames) { m_synth->render(out, frames); }

void Audio::play(Sfx s, float volume, float pitch, float pan) {
    Cmd c{Cmd::Play}; c.sfx = s; c.a = volume; c.b = pitch; c.c = pan;
    m_synth->push(c);
}
void Audio::setMusic(MusicMode m) { Cmd c{Cmd::Music}; c.mode = m; m_synth->push(c); }
void Audio::setIntensity(float i) { Cmd c{Cmd::Intensity}; c.a = i; m_synth->push(c); }
void Audio::setAmbience(const AmbienceParams& a) { Cmd c{Cmd::Amb}; c.a = a.wind; c.b = a.fire; c.c = a.crickets; c.d = a.rumble; m_synth->push(c); }
void Audio::setVolumes(float master, float music, float sfx) { Cmd c{Cmd::Vol}; c.a = master; c.b = music; c.c = sfx; m_synth->push(c); }
void Audio::setPaused(bool p) { Cmd c{Cmd::Pause}; c.a = p ? 1.f : 0.f; m_synth->push(c); }
void Audio::setLowpass(float a) { Cmd c{Cmd::Lowpass}; c.a = a; m_synth->push(c); }

} // namespace sv
