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

// Additive metallic voice (steel on steel, coins): partial ratios with individual decays.
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
        // a soft wooden tick
        b = buf(0.06f);
        bq.set(2, 1900 * vr, 3.0f);
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] = bq.process(rng.range(-1, 1)) * std::exp(-t * 140); }
        normalize(b, 0.28f);
        break;
    }
    case Sfx::UiBack: {
        b = buf(0.1f);
        bq.set(2, 1100, 2.5f);
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] = bq.process(rng.range(-1, 1)) * std::exp(-t * 90); }
        normalize(b, 0.28f);
        break;
    }
    case Sfx::UiConfirm: {
        // a paper-and-wood "tok": a struck block with a short brush of air
        b = buf(0.35f);
        bq.set(2, 1250, 4.0f); bq2.set(2, 3200, 1.0f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            b[i] = bq.process(rng.range(-1, 1)) * std::exp(-t * 60) + bq2.process(rng.range(-1, 1)) * env(t, 0.02f, 0.06f) * 0.25f;
        }
        normalize(b, 0.34f);
        break;
    }
    case Sfx::Catch: {
        // the iron catch gives way: a heavy clunk and a short chain rattle (no ringing tone)
        b = buf(0.8f);
        bq.set(0, 500, 0.9f); bq2.set(2, 2600, 1.5f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            float thunk = std::sin(TAU * lerp(130.f, 70.f, clamp01(t * 10)) * t) * std::exp(-t * 14);
            float body = bq.process(rng.range(-1, 1)) * std::exp(-t * 30);
            float rattle = bq2.process(rng.range(-1, 1)) * (0.5f + 0.5f * std::sin(t * TAU * 38)) * smoothstep(0.05f, 0.1f, t) * std::exp(-t * 7) * 0.4f;
            b[i] = thunk + body * 0.6f + rattle;
        }
        normalize(b, 0.75f);
        break;
    }
    case Sfx::CampRest: {
        // resting at a camp: embers catch, a warm breath of fire and wind
        b = buf(2.6f);
        Biquad lp; lp.set(0, 420, 0.7f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            float whoosh = lp.process(rng.range(-1, 1)) * env(t, 0.5f, 0.9f) * 1.8f;
            if (rng.chance(0.0025f * std::exp(-t))) for (size_t k = i; k < std::min(b.size(), i + 300); k++) b[k] += rng.range(-1, 1) * std::exp(-(float)(k - i) / 40.f) * 0.5f;
            b[i] += whoosh;
        }
        normalize(b, 0.6f);
        break;
    }
    case Sfx::Pickup: {
        // coins and cloth: a few small metallic clinks
        b = buf(0.5f);
        for (int k = 0; k < 3; k++) {
            float st = k * 0.05f + rng.range(0, 0.02f);
            addPartials(b, rng.range(3800, 5200), {{1, 1, 0.03f}, {1.53f, 0.5f, 0.02f}}, 0.4f, st);
        }
        bq.set(2, 2400, 0.8f);
        for (size_t i = 0; i < b.size(); i++) { float t = i / SR; b[i] += bq.process(rng.range(-1, 1)) * env(t, 0.01f, 0.05f) * 0.2f; }
        normalize(b, 0.42f);
        break;
    }
    case Sfx::Alert: {
        // a blade leaving its scabbard
        b = buf(0.55f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            if (i % 32 == 0) bq.set(2, lerp(3000.f, 7500.f, clamp01(t / 0.3f)), 3.0f);
            b[i] = bq.process(rng.range(-1, 1)) * smoothstep(0, 0.05f, t) * smoothstep(0.5f, 0.25f, t);
        }
        addPartials(b, 2900, {{1, 0.5f, 0.12f}, {1.37f, 0.3f, 0.08f}}, 0.15f, 0.28f);
        normalize(b, 0.5f);
        break;
    }
    case Sfx::Perilous: {
        // danger: a harsh rising steel scrape over a deep impact
        b = buf(1.0f);
        for (size_t i = 0; i < b.size(); i++) {
            float t = i / SR;
            if (i % 32 == 0) bq.set(2, lerp(900.f, 4200.f, clamp01(t / 0.35f)), 5.0f);
            float scrape = bq.process(rng.range(-1, 1)) * smoothstep(0, 0.03f, t) * smoothstep(0.6f, 0.2f, t) * 1.6f;
            float boom = std::sin(TAU * lerp(90.f, 40.f, clamp01(t * 3)) * t) * std::exp(-t * 6);
            b[i] = scrape + boom * 0.9f;
        }
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

// A sung voice: a band-limited glottal pulse shaped by three vowel formants. The score is voice, breath and wind only.
struct SingVoice {
    float f = 220, gain = 0, pan = 0, t = 0, dur = 1, atk = 0.3f, rel = 0.6f, ph = 0, vibPh = 0, breath = 0.15f;
    int vowel = 0;
    bool active = false, hum = false;
    Biquad f1, f2, f3, lp;
};
// Wind through a gap: narrow band-passed noise that wanders around a pitch.
struct WindTone { float f = 400, gain = 0, t = 0, dur = 4, pan = 0; bool active = false; Biquad bp; };

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

struct Cmd { enum T { Play, Music, Intensity, Amb, Vol, Pause, Lowpass } t; Sfx sfx{}; float a = 0, b = 0, c = 0, d = 0, e = 0, f = 0; MusicMode mode{}; };

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
        case Cmd::Amb: amb = {c.a, c.b, c.c, c.d, c.e, c.f}; break;
        case Cmd::Vol: master = c.a; musicVol = c.b; sfxVol = c.c; break;
        case Cmd::Pause: lpTarget = c.a > 0.5f ? 0.85f : lowpassReq; paused = c.a > 0.5f; break;
        case Cmd::Lowpass: lowpassReq = clamp01(c.a); if (!paused) lpTarget = lowpassReq; break;
        }
    }

    // ---------------- music: voices, breath and wind (no instruments)
    static constexpr int kScale[5] = {0, 2, 3, 7, 8};   // a sombre pentatonic on D (hirajoshi colour)
    float degFreq(int deg, float base = 146.83f) const {
        int oct = (int)std::floor(deg / 5.0f);
        int d = deg - oct * 5;
        return base * semis((float)(oct * 12 + kScale[d]));
    }
    static void formants(int vowel, bool high, float& a, float& b, float& c) {
        // 0 ah, 1 oh, 2 oo, 3 eh
        static const float male[4][3] = {{730, 1090, 2440}, {570, 840, 2410}, {300, 870, 2240}, {530, 1840, 2480}};
        static const float fem[4][3] = {{850, 1220, 2810}, {590, 920, 2710}, {370, 950, 2670}, {610, 2330, 2990}};
        const float* f = high ? fem[vowel & 3] : male[vowel & 3];
        a = f[0]; b = f[1]; c = f[2];
    }
    // sing(note, gain, pan, duration, vowel, attack, release, hum)
    void sing(float f, float gain, float pan, float dur, int vowel, float atk = 0.35f, float rel = 0.8f, bool hum = false) {
        SingVoice* v = nullptr;
        for (auto& x : voicesV) if (!x.active) { v = &x; break; }
        if (!v) { v = &voicesV[0]; for (auto& x : voicesV) if (x.t > v->t) v = &x; }
        *v = SingVoice();
        v->f = f; v->gain = gain; v->pan = pan; v->dur = dur; v->vowel = vowel; v->atk = atk; v->rel = rel; v->hum = hum; v->active = true;
        v->ph = rng.range(0, 1); v->vibPh = rng.range(0, TAU);
        float a, b, c; formants(vowel, f > 260, a, b, c);
        v->f1.set(2, a, 6); v->f2.set(2, b, 8); v->f3.set(2, c, 10);
        v->lp.set(0, hum ? 520.f : 3600.f, 0.7f);
        v->breath = hum ? 0.05f : 0.12f;
    }
    void chant(float f, float gain, float pan, int vowel) { sing(f, gain * 4.6f, pan, 0.12f, vowel, 0.012f, 0.09f); }
    void wind(float f, float gain, float dur, float pan) {
        WindTone* w = nullptr;
        for (auto& x : winds) if (!x.active) { w = &x; break; }
        if (!w) w = &winds[rng.irange(0, (int)winds.size() - 1)];
        w->f = f; w->gain = gain; w->dur = dur; w->pan = pan; w->t = 0; w->active = true;
        w->bp = Biquad(); w->bp.set(2, f, 40);
    }
    void breathSwell(float gain, float dur) { brAmp = 0; brTarget = gain; brT = 0; brDur = dur; }

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
            bool camp = mode == MusicMode::Shrine, title = mode == MusicMode::Title;
            // a long low "oo" bed that changes every four bars
            if (s == 0 && bar % 4 == 0) {
                int root = (bar / 4) % 2 == 0 ? -5 : -3;
                sing(degFreq(root), 0.22f, -0.2f, 13.f, 2, 2.5f, 3.0f, true);
                sing(degFreq(root + 3), 0.12f, 0.25f, 12.f, 2, 3.0f, 3.0f, true);
            }
            // a solo humming phrase, sparse and unhurried
            if (s == 0 && (bar % (camp ? 2 : 3) == 1 || (title && bar % 2 == 0)) && rng.chance(camp ? 0.9f : 0.75f)) {
                phrase = rng.irange(3, 5); phraseGap = 0;
            }
            if (phrase > 0 && s % 4 == 0) {
                if (phraseGap-- <= 0) {
                    int d = deg(camp ? 4 : 3, camp ? 10 : 11);
                    float dur = rng.range(0.7f, 1.5f) * (camp ? 1.4f : 1.f);
                    sing(degFreq(d), camp ? 0.2f : 0.17f, rng.range(-0.3f, 0.3f), dur, rng.chance(0.6f) ? 2 : 1, 0.18f, 0.7f, true);
                    phrase--; phraseGap = rng.irange(0, 1);
                }
            }
            // wind singing through the reeds
            if (s == 8 && rng.chance(title ? 0.5f : 0.3f)) wind(degFreq(deg(8, 14)), 0.5f, rng.range(3, 6), rng.range(-0.7f, 0.7f));
            if (mode == MusicMode::Explore && I > 0.2f && s == 0 && bar % 2 == 0) breathSwell(0.3f * I, 3.0f);
            break;
        }
        case MusicMode::Tension: {
            if (s == 0 && bar % 2 == 0) {
                sing(degFreq(-5), 0.24f, -0.2f, 7.5f, 2, 1.5f, 1.5f, true);
                sing(degFreq(-4), 0.12f, 0.3f, 7.0f, 1, 2.0f, 1.5f, true);   // a close, uneasy second
            }
            if (s == 0) breathSwell(0.35f, 1.6f);
            if (s == 8 && rng.chance(0.4f)) wind(degFreq(deg(10, 15)), 0.6f, 2.5f, rng.range(-0.8f, 0.8f));
            break;
        }
        case MusicMode::Combat: case MusicMode::Boss: {
            bool boss = mode == MusicMode::Boss;
            // a sung ostinato in short syllables
            static const int ost[8] = {0, 0, 3, 0, 4, 3, 1, 0};
            static const uint16_t pat = 0b1010101010110110, patBoss = 0b1011101010111011;
            if ((boss ? patBoss : pat) & (1 << (15 - s))) chant(degFreq(ost[(s / 2 + bar) % 8] - 5), (boss ? 0.34f : 0.28f) * (0.6f + 0.4f * I), -0.15f, s % 4 == 0 ? 0 : 1);
            // sustained chords above, a new one every two bars
            if (s == 0 && bar % 2 == 0) {
                int r = (bar / 2) % 4;
                static const int prog[4] = {0, -2, 1, -1};
                int root = prog[r];
                sing(degFreq(root), 0.26f, -0.35f, 3.8f, 0, 0.25f, 0.8f);
                sing(degFreq(root + 2), 0.21f, 0.35f, 3.8f, 0, 0.3f, 0.8f);
                if (boss || I > 0.6f) sing(degFreq(root + 5), 0.18f, 0.0f, 3.8f, 0, 0.35f, 0.8f);
                if (boss) sing(degFreq(root - 5), 0.2f, 0.0f, 3.8f, 1, 0.2f, 0.8f);
            }
            if (s == 0) breathSwell(boss ? 0.4f : 0.25f, 0.9f);
            if (boss && s == 8 && bar % 4 == 3) wind(degFreq(deg(10, 14)), 0.7f, 2.0f, rng.range(-0.6f, 0.6f));
            break;
        }
        case MusicMode::Requiem: {
            if (s == 0 && bar % 2 == 0) sing(degFreq(deg(5, 10)), 0.22f, 0.1f, 3.4f, 0, 0.6f, 1.6f);
            if (s == 0 && bar % 4 == 0) sing(degFreq(-5), 0.22f, 0, 9.f, 2, 2.0f, 2.5f, true);
            break;
        }
        }
    }

    float tempoFor(MusicMode m) const {
        switch (m) { case MusicMode::Combat: return 96; case MusicMode::Boss: return 108; case MusicMode::Shrine: case MusicMode::Requiem: return 60; default: return 72; }
    }

    static float polyBlep(float t, float dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1; }
        if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
        return 0;
    }

    void musicSample(float& L, float& R) {
        float stepLen = SR * 60.f / tempoFor(mode) / 4.f;
        if (++stepPos >= stepLen) { stepPos = 0; step++; onStep(); }
        float l = 0, r = 0;
        for (auto& v : voicesV) {
            if (!v.active) continue;
            v.t += 1 / SR;
            float e = v.t < v.atk ? v.t / v.atk : (v.t < v.dur ? 1.f : std::max(0.f, 1 - (v.t - v.dur) / v.rel));
            if (v.t > v.dur + v.rel) { v.active = false; continue; }
            float vib = 1 + std::sin(v.vibPh + v.t * TAU * 5.0f) * 0.007f * smoothstep(0.25f, 0.9f, v.t);
            float f = v.f * vib;
            float dtp = f / SR;
            v.ph += dtp; if (v.ph >= 1) v.ph -= 1;
            float saw = 2 * v.ph - 1 - polyBlep(v.ph, dtp);
            float src = saw * 0.6f + rng.range(-1, 1) * v.breath;
            float y = v.hum ? v.lp.process(src) * 0.9f + v.f1.process(src) * 0.3f
                            : v.f1.process(src) + v.f2.process(src) * 0.55f + v.f3.process(src) * 0.25f;
            y *= e * v.gain * (v.hum ? 1.7f : 2.6f);
            l += y * (1 - v.pan) * 0.6f; r += y * (1 + v.pan) * 0.6f;
        }
        for (auto& w : winds) {
            if (!w.active) continue;
            w.t += 1 / SR;
            if (w.t > w.dur) { w.active = false; continue; }
            float e = std::sin(PI * w.t / w.dur);
            e *= e;
            float y = w.bp.process(rng.range(-1, 1)) * e * w.gain * 0.9f;
            l += y * (1 - w.pan); r += y * (1 + w.pan);
        }
        if (brTarget > 0 || brAmp > 0.0001f) {
            brT += 1 / SR;
            float k = brT / std::max(0.1f, brDur);
            brAmp = brTarget * (k < 0.5f ? smoothstep(0, 0.5f, k) : smoothstep(1, 0.5f, k));
            if (k >= 1) { brTarget = 0; brAmp = 0; }
            brLp += 0.02f * (rng.range(-1, 1) - brLp);
            l += brLp * brAmp * 1.4f; r += brLp * brAmp * 1.4f;
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
        if (amb.rain > 0) {
            // rain: bright hiss (high-passed noise) plus scattered close drops
            float nL = rng.range(-1, 1), nR = rng.range(-1, 1);
            rainLpL += 0.35f * (nL - rainLpL); rainLpR += 0.35f * (nR - rainLpR);
            float hissL = nL - rainLpL, hissR = nR - rainLpR;
            float swell = 0.75f + 0.25f * fbm1(ambT * 0.3f, 17, 2);
            L += hissL * 0.09f * amb.rain * swell; R += hissR * 0.09f * amb.rain * swell;
            if (rng.chance(0.004f * amb.rain)) { drop = rng.range(0.3f, 1.f); dropPan = rng.range(-0.9f, 0.9f); dropPh = 0; dropF = rng.range(1800, 3800); }
            if (drop > 0.002f) {
                dropPh += dropF / SR; drop *= 0.9975f;
                float d = std::sin(dropPh * TAU) * drop * drop * 0.05f;
                L += d * (1 - dropPan); R += d * (1 + dropPan);
            }
        }
        if (amb.water > 0) {
            // running water: low burbling band, slowly modulated
            waterLp += 0.02f * (rng.range(-1, 1) - waterLp);
            waterLp2 += 0.08f * (waterLp - waterLp2);
            float burble = (waterLp - waterLp2) * (0.6f + 0.4f * std::sin(ambT * 3.1f + fbm1(ambT * 0.7f, 4, 2) * 6));
            L += burble * 2.2f * amb.water; R += burble * 2.0f * amb.water;
        }
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
    std::array<SingVoice, 14> voicesV{};
    std::array<WindTone, 4> winds{};
    int phrase = 0, phraseGap = 0;
    float brAmp = 0, brTarget = 0, brT = 0, brDur = 1, brLp = 0;
    // ambience
    AmbienceParams amb;
    float ambT = 0, windL = 0, windR = 0, fireLow = 0, crackle = 0, cracklePan = 0, cricketEnv = 0, rumbleV = 0;
    float rainLpL = 0, rainLpR = 0, drop = 0, dropPan = 0, dropPh = 0, dropF = 2000, waterLp = 0, waterLp2 = 0;

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
void Audio::setAmbience(const AmbienceParams& a) { Cmd c{Cmd::Amb}; c.a = a.wind; c.b = a.fire; c.c = a.crickets; c.d = a.rumble; c.e = a.rain; c.f = a.water; m_synth->push(c); }
void Audio::setVolumes(float master, float music, float sfx) { Cmd c{Cmd::Vol}; c.a = master; c.b = music; c.c = sfx; m_synth->push(c); }
void Audio::setPaused(bool p) { Cmd c{Cmd::Pause}; c.a = p ? 1.f : 0.f; m_synth->push(c); }
void Audio::setLowpass(float a) { Cmd c{Cmd::Lowpass}; c.a = a; m_synth->push(c); }

} // namespace sv
