// Renders a trailer score with the game's own synthesizer from a cue sheet.
//   sv_score <cues.txt> <out.f32>      (48 kHz stereo float32)
// Cue lines:  <t> music <mode> | <t> intensity <0..1> | <t> sfx <name> [vol] [pitch] [pan]
//             <t> amb <wind> <fire> <crickets> <rumble> | <t> lowpass <0..1> | <t> volume <music> | end <t>
#include "audio/audio.h"
#include "core/util.h"
#include <cstdio>
#include <map>
#include <algorithm>

using namespace sv;

static MusicMode mode(const std::string& n) {
    static const std::map<std::string, MusicMode> m = {{"silent", MusicMode::Silent}, {"title", MusicMode::Title}, {"explore", MusicMode::Explore},
        {"tension", MusicMode::Tension}, {"combat", MusicMode::Combat}, {"boss", MusicMode::Boss}, {"camp", MusicMode::Shrine}, {"elegy", MusicMode::Requiem}};
    auto it = m.find(n);
    return it == m.end() ? MusicMode::Silent : it->second;
}

static Sfx sfx(const std::string& n) {
    static const std::map<std::string, Sfx> m = {
        {"bell", Sfx::Bell}, {"campbell", Sfx::ShrineBell}, {"gate", Sfx::Gate}, {"ignite", Sfx::Ignite}, {"perilous", Sfx::Perilous},
        {"clang", Sfx::Clang}, {"deflect", Sfx::Deflect}, {"execute", Sfx::Execute}, {"swoosh", Sfx::Swoosh}, {"thud", Sfx::Thud},
        {"dash", Sfx::Dash}, {"death", Sfx::Death}, {"slash", Sfx::Slash}, {"heavy", Sfx::SlashHeavy}, {"kill", Sfx::Kill},
        {"ember", Sfx::Ember}, {"alert", Sfx::Alert}, {"confirm", Sfx::UiConfirm}, {"hit", Sfx::HitHeavy}};
    auto it = m.find(n);
    return it == m.end() ? Sfx::UiConfirm : it->second;
}

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: sv_score <cues.txt> <out.f32>\n"); return 1; }
    auto txt = readFile(argv[1]);
    if (!txt) { std::fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }
    struct Cue { float t; std::vector<std::string> a; };
    std::vector<Cue> cues;
    float end = 60;
    for (auto& raw : split(*txt, '\n')) {
        auto l = trim(raw);
        if (l.empty() || l[0] == '#') continue;
        auto tok = tokenize(l);
        if (tok[0] == "end") { end = std::stof(tok[1]); continue; }
        cues.push_back({std::stof(tok[0]), std::vector<std::string>(tok.begin() + 1, tok.end())});
    }
    std::stable_sort(cues.begin(), cues.end(), [](const Cue& x, const Cue& y) { return x.t < y.t; });
    Audio audio;
    audio.setVolumes(1.0f, 0.9f, 1.0f);
    FILE* out = std::fopen(argv[2], "wb");
    if (!out) return 1;
    const int block = 480;   // 10 ms resolution for cue timing
    std::vector<float> buf(block * 2);
    size_t ci = 0;
    long total = (long)(end * kSampleRate);
    for (long s = 0; s < total; s += block) {
        float t = s / (float)kSampleRate;
        while (ci < cues.size() && cues[ci].t <= t) {
            auto& a = cues[ci].a;
            auto f = [&](size_t i, float d) { return i < a.size() ? std::stof(a[i]) : d; };
            if (a[0] == "music") audio.setMusic(mode(a[1]));
            else if (a[0] == "intensity") audio.setIntensity(f(1, 0));
            else if (a[0] == "sfx") audio.play(sfx(a[1]), f(2, 1), f(3, 1), f(4, 0));
            else if (a[0] == "amb") { AmbienceParams p; p.wind = f(1, 0); p.fire = f(2, 0); p.crickets = f(3, 0); p.rumble = f(4, 0); audio.setAmbience(p); }
            else if (a[0] == "lowpass") audio.setLowpass(f(1, 0));
            else if (a[0] == "volume") audio.setVolumes(1.0f, f(1, 0.9f), 1.0f);
            ci++;
        }
        audio.renderOffline(buf.data(), block);
        std::fwrite(buf.data(), sizeof(float), buf.size(), out);
    }
    std::fclose(out);
    std::printf("score: %.1f s\n", end);
    return 0;
}
