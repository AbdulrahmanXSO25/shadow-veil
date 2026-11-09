#pragma once
#include <string>
#include <vector>
#include <map>
#include "core/math.h"

namespace sv {

struct World;
class Audio;

struct CutCommand {
    std::string op;
    std::vector<std::string> args;
    int line = 0;
};

struct CutsceneLibrary {
    std::map<std::string, std::vector<CutCommand>> scenes;
    bool load(const std::string& path, std::string& err);
    bool parse(const std::string& text, std::string& err);
};

// What the UI should show while a cutscene runs.
struct CutsceneView {
    std::string speaker, text;      // dialogue line
    std::string narration;          // centred narration
    std::string titleKanji, titleText;
    float titleT = 0, titleDur = 0;
    float letterbox = 0;
    float fade = 0;
    float textReveal = 0;           // characters revealed
    bool waitingForConfirm = false;
    // a choice that changes the story: options shown under the prompt
    std::string choicePrompt;
    std::vector<std::string> choices;
    std::vector<bool> choiceLocked;   // greyed out (a condition is not met)
    int choiceSel = 0;
};

class CutscenePlayer {
public:
    bool active() const { return m_active; }
    void start(const CutsceneLibrary& lib, const std::string& name, World& w);
    // Returns true while running. confirm = player pressed confirm this tick.
    bool update(World& w, float dt, bool confirm, bool skipHeld);
    const CutsceneView& view() const { return m_view; }
    CutsceneView& view() { return m_view; }
    std::string current() const { return m_name; }
    // events the game layer must handle (save, chapter end, ...)
    std::vector<std::string> takeRequests() { auto r = std::move(m_requests); m_requests.clear(); return r; }
    bool autoAdvance = false;       // autoplay/testing
    int autoChoice = 0;             // the option autoplay picks
    // menu navigation while a choice is open (-1 up, +1 down)
    void navChoice(int d);
    bool choosing() const { return !m_view.choices.empty(); }

private:
    bool step(World& w, const CutCommand& c, float dt, bool confirm);
    std::vector<CutCommand> m_cmds;
    std::string m_name;
    size_t m_pc = 0;
    float m_t = 0;
    bool m_active = false, m_started = false;
    CutsceneView m_view;
    float m_letterTarget = 0, m_letterSpeed = 1, m_fadeTarget = 0, m_fadeSpeed = 1;
    std::vector<std::string> m_requests;
    float m_skipT = 0;
};

} // namespace sv
