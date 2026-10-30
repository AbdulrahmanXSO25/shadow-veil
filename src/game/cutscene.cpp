#include "game/cutscene.h"
#include "game/world.h"
#include "core/util.h"
#include "render/text.h"

namespace sv {

bool CutsceneLibrary::load(const std::string& path, std::string& err) {
    auto t = readFile(path);
    if (!t) { err = "cannot read " + path; return false; }
    return parse(*t, err);
}

bool CutsceneLibrary::parse(const std::string& text, std::string& err) {
    std::string cur;
    auto lines = split(text, '\n');
    for (size_t i = 0; i < lines.size(); i++) {
        std::string l = trim(lines[i]);
        if (l.empty() || l[0] == '#') continue;
        auto tok = tokenize(l);
        if (tok[0] == "scene") {
            if (tok.size() < 2) { err = strfmt("line %zu: scene needs a name", i + 1); return false; }
            cur = tok[1];
            scenes[cur].clear();
            continue;
        }
        if (tok[0] == "end") { cur.clear(); continue; }
        if (cur.empty()) { err = strfmt("line %zu: command outside scene", i + 1); return false; }
        CutCommand c; c.op = tok[0]; c.args.assign(tok.begin() + 1, tok.end()); c.line = (int)i + 1;
        scenes[cur].push_back(c);
    }
    return true;
}

void CutscenePlayer::start(const CutsceneLibrary& lib, const std::string& name, World& w) {
    auto it = lib.scenes.find(name);
    if (it == lib.scenes.end()) { logWarn("missing cutscene '%s'", name.c_str()); return; }
    m_cmds = it->second; m_name = name; m_pc = 0; m_t = 0; m_active = true; m_started = false;
    m_view.speaker.clear(); m_view.text.clear(); m_view.narration.clear(); m_view.titleDur = 0;
    m_skipT = 0;
    (void)w;
}

static float argf(const CutCommand& c, size_t i, float def) { return i < c.args.size() ? std::stof(c.args[i]) : def; }
static std::string args(const CutCommand& c, size_t i, const std::string& def = "") { return i < c.args.size() ? c.args[i] : def; }
static bool hasFlag(const CutCommand& c, const char* f) { for (auto& a : c.args) if (a == f) return true; return false; }

static Vec2 focusPoint(World& w, const std::string& who) {
    if (who == "player") return w.player.pos + Vec2{0, -60};
    if (who == "boss" && w.bossIdx >= 0) return w.enemies[w.bossIdx].pos + Vec2{0, -80};
    if (startsWith(who, "npc:")) if (Npc* n = w.npc(who.substr(4))) return n->pos + Vec2{0, -60};
    if (startsWith(who, "mech:")) if (Mechanism* m = w.mech(who.substr(5))) return m->pos + Vec2{0, -60};
    return w.cam.pos;
}

static Sfx sfxByName(const std::string& n) {
    static const std::map<std::string, Sfx> m = {
        {"bell", Sfx::Bell}, {"campbell", Sfx::ShrineBell}, {"gate", Sfx::Gate}, {"ignite", Sfx::Ignite}, {"perilous", Sfx::Perilous},
        {"clang", Sfx::Clang}, {"deflect", Sfx::Deflect}, {"execute", Sfx::Execute}, {"swoosh", Sfx::Swoosh}, {"thud", Sfx::Thud},
        {"pickup", Sfx::Pickup}, {"alert", Sfx::Alert}, {"dash", Sfx::Dash}, {"death", Sfx::Death}, {"break", Sfx::Break}, {"slash", Sfx::Slash}};
    auto it = m.find(n);
    return it == m.end() ? Sfx::UiConfirm : it->second;
}

static MusicMode musicByName(const std::string& n) {
    if (n == "title") return MusicMode::Title;
    if (n == "explore") return MusicMode::Explore;
    if (n == "tension") return MusicMode::Tension;
    if (n == "combat") return MusicMode::Combat;
    if (n == "boss") return MusicMode::Boss;
    if (n == "camp") return MusicMode::Shrine;
    if (n == "elegy") return MusicMode::Requiem;
    return MusicMode::Silent;
}

bool CutscenePlayer::step(World& w, const CutCommand& c, float dt, bool confirm) {
    const std::string& op = c.op;
    bool first = !m_started;
    m_started = true;
    m_t += dt;
    if (op == "lock") { w.controlLocked = true; w.player.vel.x = 0; return true; }
    if (op == "unlock") { w.controlLocked = false; w.player.lockedPose.clear(); return true; }
    if (op == "letterbox") { m_letterTarget = argf(c, 0, 1); float t = argf(c, 1, 0.8f); m_letterSpeed = t > 0 ? 1 / t : 1000; return true; }
    if (op == "fade") {
        m_fadeTarget = argf(c, 0, 0); float t = argf(c, 1, 1); m_fadeSpeed = t > 0 ? 1 / t : 1000;
        if (hasFlag(c, "nowait")) return true;
        return std::abs(m_view.fade - m_fadeTarget) < 0.01f;
    }
    if (op == "wait") return m_t >= argf(c, 0, 1);
    if (op == "say" || op == "narrate") {
        if (first) {
            if (op == "say") { m_view.speaker = args(c, 0); m_view.text = args(c, 1); m_view.narration.clear(); }
            else { m_view.narration = args(c, 0); m_view.speaker.clear(); m_view.text.clear(); }
            m_view.textReveal = 0;
        }
        const std::string& txt = op == "say" ? m_view.text : m_view.narration;
        size_t n = decodeUtf8(txt).size();
        m_view.textReveal = std::min((float)n, m_view.textReveal + dt * 55);
        m_view.waitingForConfirm = m_view.textReveal >= n;
        if (confirm) {
            if (m_view.textReveal < n) m_view.textReveal = (float)n;
            else { m_view.speaker.clear(); m_view.text.clear(); m_view.narration.clear(); m_view.waitingForConfirm = false; return true; }
        }
        if (autoAdvance && m_view.textReveal >= n && m_t > 0.8f + n * 0.02f) { m_view.speaker.clear(); m_view.text.clear(); m_view.narration.clear(); return true; }
        return false;
    }
    if (op == "title") {
        if (first) { m_view.titleKanji = args(c, 0); m_view.titleText = args(c, 1); m_view.titleDur = argf(c, 2, 4); m_view.titleT = 0; return hasFlag(c, "nowait"); }
        return m_view.titleDur <= 0 || hasFlag(c, "nowait");
    }
    if (op == "camera") {
        // camera <who> <dxTiles> <dyTiles> <zoom> <secs> [nowait]
        if (first) {
            Vec2 f = focusPoint(w, args(c, 0, "player")) + Vec2{argf(c, 1, 0) * TILE, argf(c, 2, 0) * TILE};
            w.cam.cinematic = true;
            w.cam.cinFrom = w.cam.pos; w.cam.cinTo = f;
            w.cam.cinZoomFrom = w.cam.zoom; w.cam.cinZoomTo = argf(c, 3, w.cam.zoom);
            w.cam.cinT = 0; w.cam.cinDur = argf(c, 4, 1);
        }
        if (hasFlag(c, "nowait")) return true;
        return w.cam.cinT >= w.cam.cinDur;
    }
    if (op == "camera_release") { w.cam.cinematic = false; return true; }
    if (op == "npc") {
        // npc <id> show|hide|walk <dx> <speed>|face <dir>|pose <name>|place <who> <dx>|fade <alpha> <secs>
        Npc* n = w.npc(args(c, 0));
        if (!n) { logWarn("cutscene %s: unknown npc %s", m_name.c_str(), args(c, 0).c_str()); return true; }
        std::string sub = args(c, 1);
        if (sub == "show") { n->visible = true; return true; }
        if (sub == "hide") { n->visible = false; return true; }
        if (sub == "face") { n->facing = args(c, 2) == "right" ? 1.f : -1.f; return true; }
        if (sub == "pose") { n->pose = args(c, 2); return true; }
        if (sub == "place") { Vec2 f = focusPoint(w, args(c, 2, "player")); n->pos = n->prevPos = Vec2{f.x + argf(c, 3, 0) * TILE, w.player.pos.y}; return true; }
        if (sub == "walk") {
            if (first) { n->walkTo = n->pos.x + argf(c, 2, 1) * TILE; n->walkSpeed = argf(c, 3, 120); n->walking = true; }
            if (hasFlag(c, "nowait")) return true;
            return !n->walking;
        }
        if (sub == "fade") {
            float target = argf(c, 2, 0), dur = argf(c, 3, 1);
            n->visible = true;
            n->alpha = approach(n->alpha, target, dt / std::max(0.01f, dur));
            if (target <= 0 && n->alpha <= 0) n->visible = false;
            return std::abs(n->alpha - target) < 0.001f || hasFlag(c, "nowait");
        }
        return true;
    }
    if (op == "player") {
        std::string sub = args(c, 0);
        if (sub == "pose") { w.player.lockedPose = args(c, 1) == "none" ? "" : args(c, 1); return true; }
        if (sub == "face") { w.player.facing = args(c, 1) == "right" ? 1.f : -1.f; return true; }
        if (sub == "walk") {
            float target = w.player.pos.x;
            if (first) { w.player.anchor.x = w.player.pos.x + argf(c, 1, 1) * TILE; }
            target = w.player.anchor.x;
            float d = target - w.player.pos.x;
            if (std::abs(d) < 4) { w.player.vel.x = 0; return true; }
            w.player.facing = sign(d);
            w.player.vel.x = sign(d) * argf(c, 2, 200);
            return false;
        }
        return true;
    }
    if (op == "sfx") { if (w.audio) w.audio->play(sfxByName(args(c, 0)), argf(c, 1, 1)); return true; }
    if (op == "music") {
        std::string m = args(c, 0);
        if (m == "none") w.hasMusicOverride = false;
        else { w.hasMusicOverride = true; w.musicOverride = musicByName(m); }
        return true;
    }
    if (op == "shake") { w.shake(argf(c, 0, 0.5f)); return true; }
    if (op == "flash") {
        w.flash = argf(c, 0, 0.5f) * (w.settings && w.settings->reduceFlashes ? 0.3f : 1.f);
        w.flashColor = c.args.size() > 1 ? rgb((uint32_t)std::stoul(c.args[1], nullptr, 16)) : Color{1, 1, 1};
        return true;
    }
    if (op == "slowmo") { w.slowmoScale = argf(c, 0, 0.3f); w.slowmo = argf(c, 1, 1); return true; }
    if (op == "give") {
        w.save->add(args(c, 0), (int)argf(c, 1, 1));
        const ItemDef* d = findItem(args(c, 0));
        w.event(WorldEvent::Toast, d ? d->name : args(c, 0), d ? std::string(slotName(d->slot)) + " acquired" : "");
        return true;
    }
    if (op == "flag") { w.save->flags.insert(args(c, 0)); return true; }
    if (op == "ember") {
        w.save->flags.insert("ember");
        if (!w.save->count("ember_talisman")) w.save->add("ember_talisman");
        if (w.save->relic1.empty()) w.save->relic1 = "ember_talisman";
        else if (w.save->relic2.empty() && w.save->relic1 != "ember_talisman") w.save->relic2 = "ember_talisman";
        w.stats = computeStats(*w.save);
        w.player.ki = std::max(w.player.ki, 40.f);
        w.event(WorldEvent::Toast, "Ember Art awakened", "Ember Emblem equipped");
        return true;
    }
    if (op == "ripple") {
        w.ripple = argf(c, 1, 1); w.rippleAt = focusPoint(w, args(c, 0, "player"));
        return true;
    }
    if (op == "open") { w.activateLink(args(c, 0)); return true; }
    if (op == "toast") { w.event(WorldEvent::Toast, args(c, 0), args(c, 1)); return true; }
    if (op == "hint") { w.event(WorldEvent::Hint, args(c, 0)); return true; }
    if (op == "boss_start") { w.startBoss(); return true; }
    if (op == "request") { m_requests.push_back(args(c, 0)); return true; }
    if (op == "if_flag") {
        // skip the next N commands unless the flag is set
        if (!w.save->has(args(c, 0))) m_pc += (size_t)argf(c, 1, 1);
        return true;
    }
    if (op == "if_not_flag") {
        if (w.save->has(args(c, 0))) m_pc += (size_t)argf(c, 1, 1);
        return true;
    }
    logWarn("cutscene %s line %d: unknown op '%s'", m_name.c_str(), c.line, op.c_str());
    return true;
}

bool CutscenePlayer::update(World& w, float dt, bool confirm, bool skipHeld) {
    // animate letterbox & fade continuously
    m_view.letterbox = approach(m_view.letterbox, m_letterTarget, dt * m_letterSpeed);
    m_view.fade = approach(m_view.fade, m_fadeTarget, dt * m_fadeSpeed);
    if (m_view.titleDur > 0) { m_view.titleT += dt; if (m_view.titleT >= m_view.titleDur) m_view.titleDur = 0; }
    if (!m_active) return false;
    m_skipT = skipHeld ? m_skipT + dt : 0;
    bool skipping = m_skipT > 1.0f;
    int guard = 0;
    while (m_pc < m_cmds.size() && guard++ < 512) {
        const CutCommand& c = m_cmds[m_pc];
        bool done;
        if (skipping) {
            // fast-forward: resolve state-changing commands, drop presentation
            static const char* visualOnly[] = {"wait", "say", "narrate", "title", "camera", "shake", "flash", "sfx", "slowmo", "fade", "letterbox"};
            bool vis = false;
            for (auto* v : visualOnly) if (c.op == v) vis = true;
            if (c.op == "npc" && (args(c, 1) == "walk")) { if (Npc* n = w.npc(args(c, 0))) { n->pos.x += argf(c, 2, 0) * TILE; n->prevPos = n->pos; n->walking = false; } vis = true; }
            if (c.op == "npc" && args(c, 1) == "fade") { if (Npc* n = w.npc(args(c, 0))) { n->alpha = argf(c, 2, 0); n->visible = n->alpha > 0; } vis = true; }
            if (c.op == "player" && args(c, 0) == "walk") { w.player.pos.x += argf(c, 1, 0) * TILE; w.player.prevPos = w.player.pos; vis = true; }
            done = vis ? true : step(w, c, 0, false);
            if (c.op == "fade") { m_fadeTarget = argf(c, 0, 0); }
            if (c.op == "letterbox") { m_letterTarget = argf(c, 0, 0); }
            m_view.speaker.clear(); m_view.text.clear(); m_view.narration.clear(); m_view.titleDur = 0;
        } else done = step(w, c, dt, confirm);
        if (!done) break;
        confirm = false;
        m_pc++; m_t = 0; m_started = false;
        dt = 0;
    }
    if (m_pc >= m_cmds.size()) {
        m_active = false;
        w.cam.cinematic = false;
        if (skipping) { m_fadeTarget = 0; m_letterTarget = 0; w.controlLocked = false; w.player.lockedPose.clear(); }
    }
    return m_active;
}

} // namespace sv
