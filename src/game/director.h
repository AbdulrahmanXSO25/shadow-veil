#pragma once
#include "core/input.h"
#include <string>
#include <vector>

namespace sv {

class Game;
struct World;

// Trailer / capture direction layered over autoplay: timed world commands ("do <t> ...") and an
// AI duelist ("bot <t0> <t1>") that reads enemy telegraphs to deflect, step through perilous blows and execute.
class Director {
public:
    bool parseLine(const std::vector<std::string>& tok);   // returns true if the line was a director command
    void tick(Game& game, float t);                        // run due commands
    // Merges bot input into the scripted frame while a bot window is active.
    void drive(Game& game, float t, InputFrame& f);
    bool active() const { return !m_cmds.empty() || !m_bots.empty(); }

private:
    struct Cmd { float t; std::vector<std::string> args; bool done = false; };
    struct Bot { float t0, t1; bool aggressive; };
    std::vector<Cmd> m_cmds;
    std::vector<Bot> m_bots;
    bool m_god = false;
    bool m_prev[(size_t)Act::Count]{};
    float m_lastAttack = -10, m_lastArt = -10, m_lastThreat = -10, m_guardUntil = -10;
    void run(Game& game, const std::vector<std::string>& a);
};

} // namespace sv
