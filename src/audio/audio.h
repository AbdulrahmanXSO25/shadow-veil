#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include <cstdint>

namespace sv {

enum class Sfx {
    Slash, SlashHeavy, Clang, Deflect, Hit, HitHeavy, Step, Land, Jump, Dash, Grapple, Throw, ShurikenHit,
    Kill, Execute, Ember, Heal, Hurt, Death, UiMove, UiConfirm, UiBack, Catch, CampRest, Pickup, Alert,
    Perilous, Break, Gate, Arrow, BowDraw, Swoosh, Thud, Ignite, Count
};

enum class MusicMode { Silent, Title, Explore, Tension, Combat, Boss, Shrine, Requiem };

struct AmbienceParams { float wind = 0.3f, fire = 0, crickets = 0, rumble = 0, rain = 0, water = 0; };

class Synth;

class Audio {
public:
    Audio();
    ~Audio();
    bool init();          // opens the device; safe to skip (headless tests)
    void shutdown();
    void play(Sfx s, float volume = 1.f, float pitch = 1.f, float pan = 0.f);
    void setMusic(MusicMode m);
    void setIntensity(float i);         // 0..1 combat layering
    void setAmbience(const AmbienceParams& a);
    void setVolumes(float master, float music, float sfx);
    void setPaused(bool paused);        // muffles everything (pause menu)
    void setLowpass(float amount);      // 0..1 dulls the mix (execution slow-mo, death)
    // Render without a device (tests / offline checks). Interleaved stereo.
    void renderOffline(float* out, int frames);
    bool deviceOpen() const { return m_stream != nullptr; }

private:
    static void SDLCALL callback(void* user, SDL_AudioStream* stream, int additional, int total);
    std::unique_ptr<Synth> m_synth;
    SDL_AudioStream* m_stream = nullptr;
    std::vector<float> m_buf;
};

// Exposed for tests: synthesizes a sound effect buffer (mono, 48 kHz).
std::vector<float> synthesizeSfx(Sfx s, int variant);
constexpr int kSampleRate = 48000;

} // namespace sv
