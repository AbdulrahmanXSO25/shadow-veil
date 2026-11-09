#include "game/director.h"
#include "game/game.h"
#include "core/util.h"

namespace sv {

bool Director::parseLine(const std::vector<std::string>& t) {
    if (t.empty()) return false;
    if (t[0] == "do" && t.size() >= 3) {
        Cmd c; c.t = std::stof(t[1]);
        c.args.assign(t.begin() + 2, t.end());
        m_cmds.push_back(c);
        return true;
    }
    if (t[0] == "bot" && t.size() >= 3) {
        m_bots.push_back({std::stof(t[1]), std::stof(t[2]), t.size() > 3 && t[3] == "aggressive"});
        return true;
    }
    return false;
}

static EType typeByName(const std::string& n) {
    if (n == "boss") return EType::Tetsugen;
    return etypeByName(n, nullptr);
}

void Director::run(Game& game, const std::vector<std::string>& a) {
    World& w = *game.world();
    SaveData& s = game.saveMut();
    const std::string& op = a[0];
    auto f = [&](size_t i, float d) { return i < a.size() ? std::stof(a[i]) : d; };
    if (op == "invuln") m_god = true;
    else if (op == "mortal") m_god = false;
    else if (op == "gale") { s.flags.insert("gale"); w.stats = computeStats(s); }
    else if (op == "ember") { s.flags.insert("ember"); if (!s.count("ember_talisman")) s.add("ember_talisman"); s.relic1 = "ember_talisman"; w.stats = computeStats(s); w.player.ki = 100; }
    else if (op == "spark") w.player.ki = f(1, 100);
    else if (op == "hp") w.player.hp = maxHpOf(s) * f(1, 1);
    else if (op == "skill" && a.size() > 1) { s.skills.insert(a[1]); w.stats = computeStats(s); }
    else if (op == "flag" && a.size() > 1) s.flags.insert(a[1]);
    else if (op == "equip" && a.size() > 2) { s.add(a[2]); if (a[1] == "blade") s.blade = a[2]; else if (a[1] == "mask") s.mask = a[2]; else if (a[1] == "garb") s.garb = a[2]; w.stats = computeStats(s); }
    else if (op == "style") w.addStyle(World::SExecute, f(1, 100));
    else if (op == "pose") w.player.lockedPose = a.size() > 1 && a[1] != "none" ? a[1] : "";
    else if (op == "face") w.player.facing = a.size() > 1 && a[1] == "left" ? -1.f : 1.f;
    else if (op == "spawn" || op == "spawnidle") {
        // spawn <type> <dxTiles> [left|right]
        Enemy e;
        e.id = (int)w.enemies.size();
        e.type = typeByName(a.size() > 1 ? a[1] : "swordsman"); e.def = &enemyDef(e.type);
        e.pos = e.prevPos = e.home = w.player.pos + Vec2{f(2, 4) * TILE, -2};
        e.facing = a.size() > 3 ? (a[3] == "right" ? 1.f : -1.f) : (e.pos.x > w.player.pos.x ? -1.f : 1.f);
        bool tough = a.size() > 4 && a[4] == "tough";
        e.hpScale = tough ? 3.f : 1.f; e.dmgScale = 1;
        e.hp = e.def->hp * e.hpScale; e.deathblows = e.def->deathblows;
        e.patrolL = e.patrolR = e.pos.x;
        e.sash.init(e.pos + Vec2{0, -70}, 7, 6);
        if (op == "spawn") { e.awareness = 1; e.st = EState::Engage; e.cooldown = 0.8f; e.lastKnown = w.player.pos; }
        else { e.awareness = 0; e.st = EState::Patrol; }
        w.enemies.push_back(e);
    }
    else if (op == "clearfoes") {
        for (auto& e : w.enemies) if (e.alive() && std::abs(e.pos.x - w.player.pos.x) < f(1, 2400)) { e.st = EState::Dead; e.dissolve = 1.5f; if (e.hasToken) { e.hasToken = false; } }
    }
    else if (op == "break") {
        int best = -1; float bd = 1e9f;
        for (size_t i = 0; i < w.enemies.size(); i++)
            if (w.enemies[i].alive()) { float d = std::abs(w.enemies[i].pos.x - w.player.pos.x); if (d < bd) { bd = d; best = (int)i; } }
        if (best >= 0) { Enemy& e = w.enemies[best]; e.posture = e.def->posture; e.st = EState::Broken; e.stT = 0; e.atk = -1; }
    }
    else if (op == "cutscene" && a.size() > 1) w.event(WorldEvent::Cutscene, a[1]);
    else if (op == "boss") w.startBoss();
    else if (op == "trial") { for (auto& m : w.mechs) if (m.kind == MechKind::Trial) w.startTrial(m); }
    else if (op == "clean") game.setClean(true);
    else if (op == "hud") game.setClean(false);
    else if (op == "quiet") game.setQuiet(true);
    else if (op == "screen" && a.size() > 1) game.debugScreen(a[1]);
    else if (op == "lock") w.controlLocked = true;
    else if (op == "unlock") w.controlLocked = false;
    else if (op == "music") game.setMusicVolume(f(1, 0.6f));
    else if (op == "cam") {
        // cam <dxTiles> <dyTiles> <zoom> <secs>: a directed camera move relative to the player
        w.cam.cinematic = true;
        w.cam.cinFrom = w.cam.pos; w.cam.cinTo = w.player.pos + Vec2{f(1, 0) * TILE, -60 + f(2, 0) * TILE};
        w.cam.cinZoomFrom = w.cam.zoom; w.cam.cinZoomTo = f(3, 1); w.cam.cinT = 0; w.cam.cinDur = f(4, 2);
    }
    else if (op == "camfree") w.cam.cinematic = false;
    else if (op == "zoom") w.cam.zoomBias = f(1, 1);
    else logWarn("director: unknown command '%s'", op.c_str());
}

void Director::tick(Game& game, float t) {
    if (!game.world()) return;
    for (auto& c : m_cmds) if (!c.done && t >= c.t) { c.done = true; run(game, c.args); }
    if (m_god) {
        World& w = *game.world();
        w.player.hp = std::max(w.player.hp, maxHpOf(game.save()) * 0.6f);
        game.saveMut().shuriken = std::max(game.saveMut().shuriken, 6);
        game.saveMut().gourds = game.saveMut().maxGourds;
    }
}

void Director::drive(Game& game, float t, InputFrame& f) {
    World* wp = game.world();
    if (!wp) return;
    const Bot* bot = nullptr;
    for (auto& b : m_bots) if (t >= b.t0 && t < b.t1) bot = &b;
    if (!bot) { for (auto& p : m_prev) p = false; return; }
    World& w = *wp;
    Player& p = w.player;
    bool want[(size_t)Act::Count]{};
    float move = 0;

    // 1) read threats: blows about to land and arrows in flight
    const Enemy* threat = nullptr; bool perilous = false;
    for (auto& e : w.enemies) {
        if (!e.alive() || e.st != EState::Windup || e.atk < 0) continue;
        const EAttack& a = e.def->attacks[e.atk];
        if (a.projectile && e.type != EType::Tetsugen) continue;
        float remaining = a.windup - e.stT;
        float dist = std::abs(e.pos.x - p.pos.x);
        float reach = a.projectile ? 700 : a.range + a.lunge * 0.12f + 60;
        if (remaining > 0 && remaining < (a.perilous ? 0.14f : 0.07f) && dist < reach) { threat = &e; perilous = a.perilous; }
    }
    for (auto& pr : w.projectiles)
        if (!pr.fromPlayer && pr.type == ProjType::Arrow && std::abs(pr.pos.x - p.pos.x) < 120 && sign(pr.vel.x) == sign(p.pos.x - pr.pos.x)) { want[(size_t)Act::Guard] = true; }

    // 2) choose a target
    const Enemy* target = nullptr; float bd = 1e9f;
    for (auto& e : w.enemies) {
        if (!e.alive()) continue;
        float d = std::abs(e.pos.x - p.pos.x);
        if (d < 1400 && d < bd) { bd = d; target = &e; }
    }

    if (threat && t - m_lastThreat > 0.25f) {
        m_lastThreat = t;
        float toward = sign(threat->pos.x - p.pos.x);
        if (perilous) { want[(size_t)Act::Dash] = true; move = toward; }     // Veil-Step through it
        else want[(size_t)Act::Guard] = true;                                // deflect on the beat
    } else if (target) {
        float dx = target->pos.x - p.pos.x, d = std::abs(dx);
        float toward = sign(dx);
        if (target->st == EState::Broken) {
            if (d > 90) move = toward;
            else if (t - m_lastAttack > 0.1f) { want[(size_t)Act::Attack] = true; m_lastAttack = t; }
        } else if (target->type == EType::Archer && d > 200) {
            move = toward;
            if (d < 520 && t - m_lastArt > 1.2f && game.save().shuriken > 0) { want[(size_t)Act::Shuriken] = true; m_lastArt = t; }
        } else if (d > 95) {
            move = toward;
            if (bot->aggressive && d > 220 && d < 560 && p.ki >= 35 && game.save().has("ember") && t - m_lastArt > 2.0f) {
                want[(size_t)Act::Art] = true; m_lastArt = t; move = 0; p.facing = toward;
            }
        } else {
            if (p.facing != toward) move = toward * 0.3f;
            bool enemyBusy = target->st == EState::Recover || target->st == EState::Stunned || target->st == EState::Engage || target->st == EState::Guarding;
            if (enemyBusy && t - m_lastAttack > 0.14f) { want[(size_t)Act::Attack] = true; m_lastAttack = t; }
        }
    }
    // presses are edges: release for a tick after each press so buffered inputs register
    for (size_t i = 0; i < (size_t)Act::Count; i++) {
        bool press = want[i] && !m_prev[i];
        if (press) f.down[i] = true;
        m_prev[i] = press;
    }
    // a deflect is a tap that must still be held as the blade arrives
    if (want[(size_t)Act::Guard] && f.down[(size_t)Act::Guard]) m_guardUntil = t + 0.28f;
    if (t < m_guardUntil && !m_prev[(size_t)Act::Guard]) f.down[(size_t)Act::Guard] = true;
    if (move != 0) {
        f.moveX = clampf(move, -1, 1);
        f.down[(size_t)(move > 0 ? Act::Right : Act::Left)] = true;
    }
}

} // namespace sv
