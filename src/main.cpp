// SHADOW VEIL — entry point: window, GL context, fixed-step loop, autoplay harness.
#include "render/renderer.h"
#include "audio/audio.h"
#include "core/input.h"
#include "core/util.h"
#include "game/game.h"
#include "game/director.h"
#include "platform/platform.h"
#include "game/progression.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   // provides WinMain on Windows
#include <cstring>
#include <map>
#ifdef _MSC_VER
#define popen _popen    // trailer recording (--record) pipes frames into ffmpeg
#define pclose _pclose
#endif

using namespace sv;

namespace {

// Scripted input for automated play-testing and screenshots.
// Lines: hold <act> <t0> <t1> | press <act> <t> | shot <t> <file> | teleport <t> <target> | quit <t> | log <t>
struct Autoplay {
    struct Hold { Act act; float t0, t1; };
    struct Shot { float t; std::string path; bool done = false; };
    struct Tp { float t; std::string target; bool done = false; };
    std::vector<Hold> holds;
    std::vector<Shot> shots;
    std::vector<Tp> tps;
    std::vector<float> logs;
    float quitAt = -1;
    bool active = false;
    Director director;

    static bool actByName(const std::string& n, Act& out) {
        static const std::map<std::string, Act> m = {
            {"left", Act::Left}, {"right", Act::Right}, {"up", Act::Up}, {"down", Act::Down}, {"jump", Act::Jump},
            {"attack", Act::Attack}, {"guard", Act::Guard}, {"dash", Act::Dash}, {"grapple", Act::Grapple},
            {"shuriken", Act::Shuriken}, {"art", Act::Art}, {"heal", Act::Heal}, {"interact", Act::Interact},
            {"pause", Act::Pause}, {"menu", Act::Menu}, {"confirm", Act::Confirm}, {"cancel", Act::Cancel}};
        auto it = m.find(n);
        if (it == m.end()) return false;
        out = it->second;
        return true;
    }
    bool load(const std::string& path) {
        auto txt = readFile(path);
        if (!txt) { logError("autoplay: cannot read %s", path.c_str()); return false; }
        for (auto& raw : split(*txt, '\n')) {
            auto l = trim(raw);
            if (l.empty() || l[0] == '#') continue;
            auto t = tokenize(l);
            Act a;
            if (t[0] == "hold" && t.size() >= 4 && actByName(t[1], a)) holds.push_back({a, std::stof(t[2]), std::stof(t[3])});
            else if (t[0] == "press" && t.size() >= 3 && actByName(t[1], a)) holds.push_back({a, std::stof(t[2]), std::stof(t[2]) + 0.05f});
            else if (t[0] == "shot" && t.size() >= 3) shots.push_back({std::stof(t[1]), t[2]});
            else if (t[0] == "teleport" && t.size() >= 3) tps.push_back({std::stof(t[1]), t[2]});
            else if (t[0] == "quit" && t.size() >= 2) quitAt = std::stof(t[1]);
            else if (t[0] == "log" && t.size() >= 2) logs.push_back(std::stof(t[1]));
            else if (director.parseLine(t)) {}
            else logWarn("autoplay: bad line '%s'", l.c_str());
        }
        active = true;
        return true;
    }
    InputFrame frame(float t) const {
        InputFrame f;
        for (auto& h : holds) if (t >= h.t0 && t < h.t1) f.down[(size_t)h.act] = true;
        f.moveX = (f.held(Act::Right) ? 1.f : 0.f) - (f.held(Act::Left) ? 1.f : 0.f);
        f.moveY = (f.held(Act::Down) ? 1.f : 0.f) - (f.held(Act::Up) ? 1.f : 0.f);
        if (f.held(Act::Jump) || f.held(Act::Attack)) f.down[(size_t)Act::Confirm] = true;
        return f;
    }
};

void printHelp() {
    std::printf(
        "SHADOW VEIL %s\n"
        "  --fullscreen            start fullscreen\n"
        "  --size WxH              window size (default 1600x900)\n"
        "  --play                  skip the title screen (continue or new game)\n"
        "  --fresh                 ignore the existing save (with --play)\n"
        "  --chapter <n>           begin in chapter n (1-6)\n"
        "  --start <room|spawn>    begin at a room id or spawn id (with --play)\n"
        "  --autoplay <file>       drive input from a script (testing)\n"
        "  --user-dir <dir>        use a different save/settings directory\n"
        "  --x11                   prefer XWayland over native Wayland\n"
        "  --stats                 print frame timing every 5 seconds\n"
        "  --render-size WxH       render at a fixed internal resolution\n"
        "  --record <file.mp4>     capture an autoplay run to video (needs --render-size, writes <file>.f32 audio)\n"
        "  --export-achievements <file.csv>  write the achievement list for the store and exit\n"
        "  --no-music              mute the score (sound effects and ambience remain)\n", SV_VERSION);
}

} // namespace

// A fatal start-up problem: log it and tell the player in a dialog (release builds have no console).
static int fatal(SDL_Window* win, const std::string& what) {
    logError("%s", what.c_str());
    std::string msg = what + "\n\nA log was written to:\n" + userDir() + "/shadow-veil.log";
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Shadow Veil", msg.c_str(), win);
    return 1;
}

int main(int argc, char** argv) {
    GameOptions opt;
    std::string autoplayPath;
    int winW = 1600, winH = 900;
    bool fullscreen = false, preferX11 = false, stats = false;
    int renderW = 0, renderH = 0;
    std::string recordPath;
    bool noMusic = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (a == "--help" || a == "-h") { printHelp(); return 0; }
        else if (a == "--fullscreen") fullscreen = true;
        else if (a == "--size") { auto s = split(next(), 'x'); if (s.size() == 2) { winW = std::stoi(s[0]); winH = std::stoi(s[1]); } }
        else if (a == "--play") opt.skipTitle = true;
        else if (a == "--fresh") opt.freshSave = true;
        else if (a == "--chapter") { opt.chapter = std::atoi(next().c_str()); opt.skipTitle = true; }
        else if (a == "--start") { opt.startAt = next(); opt.skipTitle = true; }
        else if (a == "--autoplay") { autoplayPath = next(); opt.autoCutscenes = true; }
        else if (a == "--user-dir") setUserDirOverride(next());
        else if (a == "--x11") preferX11 = true;
        else if (a == "--stats") stats = true;
        else if (a == "--record") recordPath = next();
        else if (a == "--no-music") noMusic = true;
        else if (a == "--export-achievements") {
            // CSV for the Steamworks achievement page: api name, display name, description, hidden
            std::string out = next(), csv = "api_name,display_name,description,hidden\n";
            auto q = [](const std::string& s) { std::string r = "\""; for (char c : s) { if (c == '"') r += '"'; r += c; } return r + "\""; };
            for (auto& d : achievementDefs()) csv += d.id + "," + q(d.name) + "," + q(d.desc) + "," + (d.hidden ? "1" : "0") + "\n";
            if (!writeFileAtomic(out, csv)) { logError("cannot write %s", out.c_str()); return 1; }
            std::printf("%zu achievements -> %s\n", achievementDefs().size(), out.c_str());
            return 0;
        }
        else if (a == "--render-size") { auto s = split(next(), 'x'); if (s.size() == 2) { renderW = std::stoi(s[0]); renderH = std::stoi(s[1]); } }
        else { logWarn("unknown argument %s", a.c_str()); }
    }

#if defined(__linux__) || defined(__FreeBSD__)
    // native Wayland first, X11 as the fallback (other platforms use SDL's default driver)
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, preferX11 ? "x11,wayland" : "wayland,x11");
#else
    (void)preferX11;
#endif
    SDL_SetHint(SDL_HINT_APP_ID, "dev.shadowveil.game");   // matches the .desktop file, so docks show the icon
    SDL_SetAppMetadata("Shadow Veil", SV_VERSION, "dev.shadowveil.game");
    if (!platform::init()) return 0;   // the store relaunched us
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) return fatal(nullptr, strfmt("Could not start the video system: %s", SDL_GetError()));
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* win = SDL_CreateWindow("Shadow Veil", winW, winH, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    if (!win) return fatal(nullptr, strfmt("Could not open the game window: %s\n\nShadow Veil needs a graphics card with OpenGL 3.3. Please install the latest driver from your GPU vendor (AMD, NVIDIA or Intel).", SDL_GetError()));
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) return fatal(win, strfmt("Could not create an OpenGL 3.3 context: %s\n\nShadow Veil needs a graphics card with OpenGL 3.3. Please install the latest driver from your GPU vendor (AMD, NVIDIA or Intel).", SDL_GetError()));
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);
    logInfo("video driver: %s | GL %s | %s", SDL_GetCurrentVideoDriver(), (const char*)glGetString(GL_VERSION), (const char*)glGetString(GL_RENDERER));
    SDL_HideCursor();

    int pw, ph;
    SDL_GetWindowSizeInPixels(win, &pw, &ph);
    Renderer renderer;
    if (!renderer.init(pw, ph)) return fatal(win, strfmt("Your graphics driver does not support OpenGL 3.3 (it reports \"%s\").\n\nPlease install the latest driver from your GPU vendor (AMD, NVIDIA or Intel). Remote desktops and virtual machines often lack 3D support.", (const char*)glGetString(GL_RENDERER)));
    if (renderW > 0) renderer.setFixedResolution(renderW, renderH);
    Audio audio;
    // Recording renders audio offline in lockstep with frames; live play uses the device.
    if (recordPath.empty()) audio.init();
    FILE* video = nullptr;
    FILE* audioOut = nullptr;
    std::vector<uint8_t> frameBuf;
    std::vector<float> audioBuf(800 * 2);
    if (!recordPath.empty()) {
        if (renderW <= 0 || autoplayPath.empty()) { logError("--record needs --render-size and --autoplay"); return 1; }
        std::string cmd = strfmt("ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgba -s %dx%d -r 60 -i - -vf vflip "
                                 "-c:v libx264 -preset medium -crf 15 -pix_fmt yuv420p '%s'", renderW, renderH, recordPath.c_str());
        video = popen(cmd.c_str(), "w");
        audioOut = std::fopen((recordPath + ".f32").c_str(), "wb");
        if (!video || !audioOut) { logError("cannot start recording"); return 1; }
        logInfo("recording to %s", recordPath.c_str());
    }
    Input input;
    input.init();

    Autoplay autoplay;
    if (!autoplayPath.empty() && !autoplay.load(autoplayPath)) return 1;

    Game game;
    if (!game.init(&renderer, &audio, &input, win, opt)) return fatal(win, "The game data could not be loaded. Please reinstall Shadow Veil (the data and assets folders must sit next to the program).");
    platform::syncAchievements(game.profile().achievements);
    if (fullscreen) { game.settings().fullscreen = true; }
    if (noMusic) game.setMusicVolume(0);

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();
    double acc = 0;
    float simTime = 0;
    double statT = 0; int statFrames = 0; double worstFrame = 0, cpuAcc = 0;
    // GPU timing (ring of queries so we never stall waiting for results)
    GLuint gpuQ[4]; glGenQueries(4, gpuQ); int qi = 0, qIssued = 0; double gpuAcc = 0, gpuWorst = 0; int gpuN = 0;
    bool running = true;
    while (running && !game.quitRequested()) {
        SDL_Event e;
        platform::tick();
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) { running = false; logInfo("quit requested by the window system"); }
            else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) renderer.resize(e.window.data1, e.window.data2);
            else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_F11) {
                game.settings().fullscreen = !game.settings().fullscreen;
                game.applySettings();
            } else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_F12) {
                std::string path = userDir() + strfmt("/screenshot_%llu.png", (unsigned long long)SDL_GetTicks());
                if (renderer.saveScreenshot(path)) logInfo("screenshot: %s", path.c_str());
            }
            input.handleEvent(e);
        }
        Uint64 now = SDL_GetPerformanceCounter();
        double frameDt = (double)(now - last) / freq;
        last = now;
        if (frameDt > 0.25) frameDt = 0.25;   // stalls (window drag, breakpoint) shouldn't explode the sim
        if (autoplay.active) frameDt = kDt * 2;  // deterministic: two sim ticks per rendered frame
        acc += frameDt * game.gameSpeed();
        Uint64 cpu0 = SDL_GetPerformanceCounter();
        int steps = 0;
        while (acc >= kDt && steps < 8) {
            if (autoplay.active) {
                autoplay.director.tick(game, simTime);
                InputFrame frame = autoplay.frame(simTime);
                autoplay.director.drive(game, simTime, frame);
                input.injectFrame(frame);
                for (auto& tp : autoplay.tps) if (!tp.done && simTime >= tp.t) { tp.done = true; game.teleport(tp.target); }
                for (float lt : autoplay.logs) if (simTime >= lt && simTime - kDt < lt && game.world()) {
                    auto& p = game.world()->player;
                    const Enemy* ne = nullptr;
                    for (auto& e : game.world()->enemies) if (e.alive() && (!ne || std::abs(e.pos.x - p.pos.x) < std::abs(ne->pos.x - p.pos.x))) ne = &e;
                    logInfo("t=%.2f player=(%.0f,%.0f) state=%d hp=%.0f room=%d screen=%d enemies=%zu | nearest dx=%.0f st=%d aware=%.2f face=%.0f",
                            simTime, p.pos.x, p.pos.y, (int)p.state, p.hp, game.world()->curRoom, (int)game.screen(), game.world()->enemies.size(),
                            ne ? ne->pos.x - p.pos.x : 0.f, ne ? (int)ne->st : -1, ne ? ne->awareness : 0.f, ne ? ne->facing : 0.f);
                }
            }
            input.tick(kDt);
            game.tick(kDt);
            acc -= kDt;
            simTime += kDt;
            steps++;
        }
        if (steps == 8) acc = 0;
        if (stats) glBeginQuery(GL_TIME_ELAPSED, gpuQ[qi % 4]);
        game.render((float)(acc / kDt));
        if (stats) {
            glEndQuery(GL_TIME_ELAPSED);
            qi++; qIssued++;
            if (qIssued >= 4) {
                GLuint64 ns = 0; GLint avail = 0;
                glGetQueryObjectiv(gpuQ[qi % 4], GL_QUERY_RESULT_AVAILABLE, &avail);
                if (avail) { glGetQueryObjectui64v(gpuQ[qi % 4], GL_QUERY_RESULT, &ns); gpuAcc += ns / 1e6; gpuWorst = std::max(gpuWorst, ns / 1e6); gpuN++; }
            }
        }
        cpuAcc += (double)(SDL_GetPerformanceCounter() - cpu0) / freq;
        if (video) {
            renderer.readPixels(frameBuf);
            std::fwrite(frameBuf.data(), 1, frameBuf.size(), video);
            audio.renderOffline(audioBuf.data(), 800);   // exactly 1/60 s per frame
            std::fwrite(audioBuf.data(), sizeof(float), audioBuf.size(), audioOut);
        }
        if (autoplay.active) {
            for (auto& s : autoplay.shots)
                if (!s.done && simTime >= s.t) { s.done = true; renderer.saveScreenshot(s.path); logInfo("shot %.2f -> %s", simTime, s.path.c_str()); }
            if (autoplay.quitAt > 0 && simTime >= autoplay.quitAt) running = false;
        }
        SDL_GL_SwapWindow(win);
        statT += frameDt; statFrames++; worstFrame = std::max(worstFrame, frameDt);
        if (stats && statT > 5) {
            logInfo("fps %.1f | worst frame %.1f ms | cpu %.2f ms | gpu %.2f ms avg, %.2f worst | draw calls %d | particles %zu", statFrames / statT, worstFrame * 1000,
                    cpuAcc / statFrames * 1000, gpuN ? gpuAcc / gpuN : 0.0, gpuWorst, renderer.drawCalls(), game.world() ? game.world()->particles.size() : (size_t)0);
            statT = 0; statFrames = 0; worstFrame = 0; cpuAcc = 0; gpuAcc = 0; gpuWorst = 0; gpuN = 0;
        }
    }
    if (game.quitRequested()) logInfo("quit chosen in game");
    if (autoplay.active) logInfo("autoplay ended at t=%.2f (quit cue %.2f, running=%d)", simTime, autoplay.quitAt, (int)running);
    if (video) { pclose(video); std::fclose(audioOut); logInfo("recording finished"); }
    audio.shutdown();
    input.shutdown();
    renderer.shutdown();
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(win);
    platform::shutdown();
    SDL_Quit();
    return 0;
}
