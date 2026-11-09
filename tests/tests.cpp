// Headless test suite: content validation, persistence, progression rules, and real simulation
// of movement, combat, stealth and puzzles using the same World code the game runs.
#include "game/world.h"
#include "game/cutscene.h"
#include "audio/audio.h"
#include "core/util.h"
#include <cstdio>
#include <functional>
#include <set>
#include <filesystem>

using namespace sv;

static int g_fail = 0, g_pass = 0;
#define CHECK(cond, ...) do { if (cond) g_pass++; else { g_fail++; std::printf("  FAIL %s:%d: %s — ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Sim {
    Level lvl;
    SaveData save = newGame(0);
    Settings settings;
    Input input;
    World w;
    bool ok = false;
    Sim() {
        std::string err;
        ok = w.init(contentPath("data/levels/ch1_ashen_grove.lvl"), &save, &settings, nullptr, &input, err);
        if (!ok) std::printf("  world init: %s\n", err.c_str());
        // mark every scripted trigger as seen so tests aren't interrupted by cutscenes
        for (size_t i = 0; i < w.triggerFired.size(); i++) w.triggerFired[i] = true;
    }
    void step(const std::set<Act>& held = {}, int ticks = 1) {
        for (int i = 0; i < ticks; i++) {
            InputFrame f;
            for (Act a : held) f.down[(size_t)a] = true;
            f.moveX = (held.count(Act::Right) ? 1.f : 0.f) - (held.count(Act::Left) ? 1.f : 0.f);
            input.injectFrame(f);
            input.tick(kDt);
            w.update(kDt);
        }
    }
    void settle() { step({}, 60); }
    // Put the player on the ground at a room-local tile column.
    void place(const std::string& room, int col) {
        for (auto& r : w.level.rooms)
            if (r.id == room) {
                int gx = r.x0 + col;
                for (int y = 1; y < w.level.height; y++)
                    if (!w.level.solidAt(gx, y) && w.level.solidAt(gx, y + 1)) {
                        w.player.pos = w.player.prevPos = {(gx + 0.5f) * TILE, (y + 1) * TILE - 0.01f};
                        w.player.vel = {0, 0};
                        w.player.state = PState::Ground;
                        return;
                    }
            }
    }
    Enemy& addEnemy(EType t, Vec2 pos, float facing) {
        Enemy e;
        e.id = (int)w.enemies.size();
        e.type = t; e.def = &enemyDef(t);
        e.pos = e.prevPos = e.home = pos;
        e.facing = facing;
        e.hp = e.def->hp; e.deathblows = e.def->deathblows;
        e.patrolL = e.patrolR = pos.x;
        e.sash.init(pos, 4, 6);
        w.enemies.push_back(e);
        return w.enemies.back();
    }
};

static void test(const char* name, const std::function<void()>& fn) {
    int before = g_fail;
    fn();
    std::printf("%s %s\n", g_fail == before ? "[ ok ]" : "[FAIL]", name);
}

int main() {
    setUserDirOverride((std::filesystem::temp_directory_path() / "shadow-veil-tests").string());

    test("level parses and is well-formed", [] {
        Level L; std::string err;
        bool ok = loadLevel(contentPath("data/levels/ch1_ashen_grove.lvl"), L, err);
        CHECK(ok, "%s", err.c_str());
        CHECK(L.rooms.size() == 10, "rooms=%zu", L.rooms.size());
        CHECK(L.findSpawn("player") != nullptr, "no player");
        int shrines = 0, bosses = 0;
        for (auto& s : L.spawns) { if (s.kind == "shrine") shrines++; if (s.kind == "boss") bosses++; }
        CHECK(shrines == 4, "shrines=%d", shrines);
        int trials = 0; for (auto& sp : L.spawns) if (sp.kind == "trial") trials++;
        CHECK(trials == 1, "trials=%d", trials);
        CHECK(bosses == 1, "bosses=%d", bosses);
        std::set<std::string> ids;
        for (auto& s : L.spawns) { CHECK(!ids.count(s.id), "duplicate id %s", s.id.c_str()); ids.insert(s.id); }
        for (auto& s : L.spawns) {
            if (s.kind == "chest") CHECK(findItem(s.get("item")) != nullptr, "chest item %s", s.get("item").c_str());
            if (s.kind == "material") CHECK(findItem(s.get("item", "iron_sand")) != nullptr, "material %s", s.get("item").c_str());
            if (s.kind == "scroll") CHECK(findLore(s.get("lore")) != nullptr, "lore %s", s.get("lore").c_str());
        }
    });

    test("level format errors are reported", [] {
        Level L; std::string err;
        CHECK(!parseLevel("level x\nheight 3\nroom a \"A\"\nmap\n...\nP..\nendmap\nendroom\n", L, err), "short map accepted");
        CHECK(err.find("rows") != std::string::npos, "%s", err.c_str());
        CHECK(!parseLevel("level x\nheight 2\nroom a \"A\"\nmap\n...\n###\nendmap\nendroom\n", L, err), "missing player accepted");
        CHECK(parseLevel("level x\nheight 2\nroom a \"A\"\nmap\n.P.\n###\nendmap\nendroom\n", L, err), "%s", err.c_str());
    });

    test("every cutscene referenced by the level exists and uses known commands", [] {
        CutsceneLibrary lib; std::string err;
        CHECK(lib.load(contentPath("data/scripts/ch1.cut"), err), "%s", err.c_str());
        Level L; loadLevel(contentPath("data/levels/ch1_ashen_grove.lvl"), L, err);
        for (auto& t : L.triggers)
            if (startsWith(t.action, "cutscene ")) CHECK(lib.scenes.count(t.action.substr(9)), "missing scene %s", t.action.c_str());
        for (auto& s : L.spawns) if (s.kind == "npc" && !s.get("talk").empty()) CHECK(lib.scenes.count(s.get("talk")), "missing talk %s", s.get("talk").c_str());
        for (const char* req : {"boss_phase2", "boss_outro", "bell_rung", "gale_awaken"}) CHECK(lib.scenes.count(req), "engine scene %s", req);
        std::set<std::string> known = {"lock", "unlock", "letterbox", "fade", "wait", "say", "narrate", "title", "camera", "camera_release", "npc",
                                       "player", "sfx", "music", "shake", "flash", "slowmo", "give", "flag", "ember", "hint", "boss_start", "request",
                                       "if_flag", "if_not_flag", "ripple", "open", "toast"};
        for (auto& [name, cmds] : lib.scenes)
            for (auto& c : cmds) CHECK(known.count(c.op), "%s:%d unknown op %s", name.c_str(), c.line, c.op.c_str());
    });

    test("every chapter's level and script load, link up and use known commands", [] {
        std::set<std::string> known = {"lock", "unlock", "letterbox", "fade", "wait", "say", "narrate", "title", "camera", "camera_release", "npc",
                                       "player", "sfx", "music", "shake", "flash", "slowmo", "give", "flag", "ember", "hint", "boss_start", "request",
                                       "if_flag", "if_not_flag", "ripple", "open", "toast", "flood", "chase", "chase_stop", "layer", "spawn",
                                       "boss_leave", "choice", "goto", "goto_if", "label", "walk"};
        int totalLore = 0;
        for (auto& ch : chapterDefs()) {
            std::string err;
            Level L;
            bool ok = loadLevel(contentPath(ch.levelPath), L, err);
            CHECK(ok, "chapter %d level: %s", ch.number, err.c_str());
            if (!ok) continue;
            CutsceneLibrary lib;
            CHECK(lib.load(contentPath(ch.scriptPath), err), "chapter %d script: %s", ch.number, err.c_str());
            for (auto& t : L.triggers)
                if (startsWith(t.action, "cutscene ")) CHECK(lib.scenes.count(t.action.substr(9)), "ch%d missing scene %s", ch.number, t.action.c_str());
            int bosses = 0, camps = 0, lore = 0;
            int deathblows = 1;
            std::set<std::string> ids;
            for (auto& s : L.spawns) {
                if (s.kind != "drain") { CHECK(!ids.count(s.id), "ch%d duplicate id %s", ch.number, s.id.c_str()); ids.insert(s.id); }
                // anything that persists an "open:" flag must carry the chapter prefix, so replays can reset it
                bool persists = (s.kind == "gate" && s.id != "boss_door") || s.kind == "drain" || s.kind == "sluice";
                if (persists && ch.number > 1) {
                    CHECK(startsWith(s.id, ch.prefix + "."), "ch%d id %s lacks prefix", ch.number, s.id.c_str());
                    if (!s.get("link").empty()) CHECK(startsWith(s.get("link"), ch.prefix + "."), "ch%d link %s lacks prefix", ch.number, s.get("link").c_str());
                }
                if (s.kind == "npc" && !s.get("talk").empty()) CHECK(lib.scenes.count(s.get("talk")), "ch%d missing talk %s", ch.number, s.get("talk").c_str());
                if (s.kind == "chest") CHECK(findItem(s.get("item")) != nullptr, "ch%d chest item %s", ch.number, s.get("item").c_str());
                if (s.kind == "material") CHECK(findItem(s.get("item", "iron_sand")) != nullptr, "ch%d material %s", ch.number, s.get("item").c_str());
                if (s.kind == "scroll") {
                    const LoreDef* ld = findLore(s.get("lore"));
                    CHECK(ld != nullptr, "ch%d lore %s", ch.number, s.get("lore").c_str());
                    if (ld) CHECK(ld->chapter == ch.number, "ch%d scroll %s belongs to chapter %d", ch.number, ld->id.c_str(), ld->chapter);
                    lore++;
                }
                if (s.kind == "shrine") camps++;
                if (s.kind == "trial") CHECK(s.get("reward") == ch.stance, "ch%d trial reward %s", ch.number, s.get("reward").c_str());
                if (s.kind == "boss") {
                    bosses++;
                    if (ch.number > 1) {
                        bool okT = false; EType t = etypeByName(s.get("who"), &okT);
                        CHECK(okT && enemyDef(t).boss, "ch%d boss who=%s", ch.number, s.get("who").c_str());
                        if (okT) { deathblows = enemyDef(t).deathblows; CHECK(enemyDef(t).flag == ch.bossFlag, "ch%d boss flag %s", ch.number, enemyDef(t).flag.c_str()); }
                    }
                }
            }
            CHECK(bosses == 1, "ch%d bosses=%d", ch.number, bosses);
            CHECK(camps >= 2, "ch%d camps=%d", ch.number, camps);
            CHECK(lore == loreCountForChapter(ch.number), "ch%d scrolls placed %d of %d", ch.number, lore, loreCountForChapter(ch.number));
            totalLore += lore;
            std::vector<std::string> req = {"intro", "boss_intro", "boss_outro"};
            for (int p = 2; p <= deathblows; p++) req.push_back("boss_phase" + std::to_string(p));
            if (!ch.stance.empty()) req.push_back(ch.stance + "_awaken");
            for (auto& r : req) CHECK(lib.scenes.count(r), "ch%d engine scene %s", ch.number, r.c_str());
            bool endsChapter = false;
            for (auto& c : lib.scenes["boss_outro"]) if (c.op == "request" && !c.args.empty() && c.args[0] == "chapter_end") endsChapter = true;
            CHECK(endsChapter, "ch%d boss_outro does not end the chapter", ch.number);
            for (auto& [name, cmds] : lib.scenes) {
                std::set<std::string> labels;
                for (auto& c : cmds) { CHECK(known.count(c.op), "ch%d %s:%d unknown op %s", ch.number, name.c_str(), c.line, c.op.c_str()); if (c.op == "label" && !c.args.empty()) labels.insert(c.args[0]); }
                for (auto& c : cmds) {
                    if (c.op == "goto" && !c.args.empty()) CHECK(labels.count(c.args[0]), "ch%d %s goto %s has no label", ch.number, name.c_str(), c.args[0].c_str());
                    if (c.op == "goto_if" && c.args.size() > 1) CHECK(labels.count(c.args[1]), "ch%d %s goto_if %s has no label", ch.number, name.c_str(), c.args[1].c_str());
                    if (c.op == "npc" && !c.args.empty()) {
                        bool found = false;
                        for (auto& s : L.spawns) if (s.kind == "npc" && s.get("id", s.id) == c.args[0]) found = true;
                        CHECK(found, "ch%d %s: npc %s not in level", ch.number, name.c_str(), c.args[0].c_str());
                    }
                }
            }
            // the world must come up in every chapter
            SaveData save = newGame(0); Settings settings; Input input; World w;
            CHECK(w.init(contentPath(ch.levelPath), &save, &settings, nullptr, &input, err), "ch%d world init: %s", ch.number, err.c_str());
        }
        CHECK(totalLore == (int)loreDefs().size(), "scrolls placed %d of %zu", totalLore, loreDefs().size());
    });

    test("save data round-trips", [] {
        SaveData s = newGame(0);
        s.mon = 321; s.hp = 77.5f; s.skills = {"blade_fourth", "veil_second"}; s.flags = {"ember", "open:bellgate"};
        s.inventory["coal_ruby"] = 2; s.gems["kurosaki_wakizashi"] = {"coal_ruby"}; s.bump("kills", 9); s.ngPlus = 2;
        SaveData t;
        CHECK(deserialize(serialize(s), t), "deserialize failed");
        CHECK(t.mon == 321 && std::abs(t.hp - 77.5f) < 0.01f, "mon=%d hp=%f", t.mon, t.hp);
        CHECK(t.skills == s.skills && t.flags == s.flags, "sets differ");
        CHECK(t.count("coal_ruby") == 2 && t.gems["kurosaki_wakizashi"].size() == 1, "inventory/gems");
        CHECK(t.counter("kills") == 9 && t.ngPlus == 2, "counters");
        Settings st; st.gameSpeed = 0.7f; st.highContrast = true;
        Settings st2; deserialize(serialize(st), st2);
        CHECK(std::abs(st2.gameSpeed - 0.7f) < 0.01f && st2.highContrast, "settings");
        CHECK(saveGame(s), "write save");
        SaveData u; CHECK(loadGame(u) && u.mon == 321, "load save");
    });

    test("progression: skills, crafting, equipment stats", [] {
        SaveData s = newGame(0);
        const SkillDef* tier2 = nullptr; const SkillDef* tier1 = nullptr;
        for (auto& d : skillDefs()) { if (d.id == "blade_iron") tier2 = &d; if (d.id == "blade_fourth") tier1 = &d; }
        s.skillPoints = 1;
        CHECK(!canLearn(s, *tier2), "tier 2 without tier 1");
        CHECK(canLearn(s, *tier1), "tier 1 learnable");
        float base = computeStats(s).attack;
        s.inventory["hollow_moon"] = 1; s.blade = "hollow_moon";
        CHECK(computeStats(s).attack == base + 6, "hollow moon attack");
        s.mon = 500; s.inventory["iron_sand"] = 4;
        const RecipeDef* temper = nullptr; for (auto& r : recipeDefs()) if (r.id == "temper") temper = &r;
        CHECK(canCraft(s, *temper), "temper craftable");
        applyCraft(s, *temper);
        CHECK(s.bladeLevel == 1 && s.count("iron_sand") == 2 && s.mon == 380, "temper applied lvl=%d sand=%d mon=%d", s.bladeLevel, s.count("iron_sand"), s.mon);
        CHECK(computeStats(s).attack == base + 10, "tempered attack");
        SaveData ng = newGame(1, &s);
        CHECK(ng.ngPlus == 1 && ng.blade == "hollow_moon" && ng.bladeLevel == 1 && !ng.has("ember"), "new game+ carry");
    });

    test("player lands on the ground and runs", [] {
        Sim s; if (!s.ok) return;
        s.place("street", 10);
        s.settle();
        CHECK(s.w.player.onGround, "not grounded y=%f", s.w.player.pos.y);
        float x0 = s.w.player.pos.x;
        s.step({Act::Right}, 120);
        float moved = s.w.player.pos.x - x0;
        CHECK(moved > 380 && moved < 460, "ran %f px in 1s", moved);
    });

    test("jump height and coyote/variable jump", [] {
        Sim s; if (!s.ok) return;
        s.place("street", 5);
        s.settle();
        float y0 = s.w.player.pos.y, minY = y0;
        s.step({Act::Jump}, 1);
        for (int i = 0; i < 80; i++) { s.step({Act::Jump}); minY = std::min(minY, s.w.player.pos.y); }
        float full = y0 - minY;
        CHECK(full > 160 && full < 230, "full jump %f", full);
        s.settle();
        y0 = s.w.player.pos.y; minY = y0;
        s.step({Act::Jump}, 3);
        for (int i = 0; i < 80; i++) { s.step(); minY = std::min(minY, s.w.player.pos.y); }
        CHECK(y0 - minY < full * 0.6f, "short hop %f vs %f", y0 - minY, full);
    });

    test("wall-jump shaft in the cliff room is climbable", [] {
        Sim s; if (!s.ok) return;
        s.place("cliff", 44);
        s.settle();
        float startY = s.w.player.pos.y;
        // a player's rhythm: jump into the shaft, then kick off whichever wall we're sliding on
        Act dir = Act::Right;
        s.step({dir, Act::Jump}, 1);
        int sinceKick = 99;
        for (int t = 0; t < 600; t++) {
            sinceKick++;
            if (s.w.player.state == PState::WallSlide && sinceKick > 8) {
                Act toward = dir;
                dir = s.w.player.wallDir > 0 ? Act::Left : Act::Right;
                s.step({toward}, 1);              // release jump for a frame, still pressing into the wall
                s.step({dir, Act::Jump}, 1);
                sinceKick = 0;
            } else s.step(sinceKick < 2 ? std::set<Act>{} : std::set<Act>{dir, Act::Jump});
            if (getenv("SV_TRACE") && t % 6 == 0) std::printf("    t=%d x=%.0f y=%.0f st=%d wall=%d vy=%.0f\n", t, s.w.player.pos.x, s.w.player.pos.y, (int)s.w.player.state, s.w.player.wallDir, s.w.player.vel.y);
            if (s.w.player.pos.y < startY - 430 && s.w.player.onGround) break;
        }
        float peak = startY;
        for (int t = 0; t < 60; t++) { s.step({Act::Right}); peak = std::min(peak, s.w.player.pos.y); }
        float wallRight = 0;
        for (auto& r : s.w.level.rooms) if (r.id == "cliff") wallRight = (r.x0 + 48) * TILE;
        CHECK(s.w.player.pos.x > wallRight, "did not cross the wall: x=%f", s.w.player.pos.x);
    });

    test("combo attacks damage and kill an enemy", [] {
        Sim s; if (!s.ok) return;
        s.place("street", 10);
        s.settle();
        Vec2 p = s.w.player.pos;
        s.addEnemy(EType::Swordsman, p + Vec2{70, 0}, -1);
        int idx = (int)s.w.enemies.size() - 1;
        s.w.enemies[idx].awareness = 1; s.w.enemies[idx].st = EState::Stunned;
        s.save.counters.erase("kills");
        for (int i = 0; i < 40 && s.w.enemies[idx].alive(); i++) { s.step({Act::Attack}, 2); s.step({}, 12); }
        CHECK(!s.w.enemies[idx].alive(), "enemy survived hp=%f", s.w.enemies[idx].hp);
        CHECK(s.save.counter("kills") == 1, "kills=%d", s.save.counter("kills"));
    });

    test("deflecting an attack builds posture and counts", [] {
        Sim s; if (!s.ok) return;
        s.place("street", 10);
        s.settle();
        Vec2 p = s.w.player.pos;
        s.addEnemy(EType::Swordsman, p + Vec2{90, 0}, -1);
        int idx = (int)s.w.enemies.size() - 1;
        Enemy& e = s.w.enemies[idx];
        e.awareness = 1; e.atk = 0; e.st = EState::Windup; e.stT = 0;
        const EAttack& a = e.def->attacks[0];
        // press guard just before the blade arrives
        int pressAt = (int)((a.windup - 0.05f) / kDt);
        s.step({}, pressAt);
        s.step({Act::Guard}, 30);
        CHECK(s.save.counter("deflects") >= 1, "no deflect recorded");
        CHECK(s.w.enemies[idx].posture > 20, "posture %f", s.w.enemies[idx].posture);
        CHECK(s.w.player.hp == maxHpOf(s.save), "took damage %f", s.w.player.hp);
    });

    test("posture break enables an execution", [] {
        Sim s; if (!s.ok) return;
        s.place("street", 10);
        s.settle();
        s.addEnemy(EType::Swordsman, s.w.player.pos + Vec2{70, 0}, -1);
        int idx = (int)s.w.enemies.size() - 1;
        s.w.enemies[idx].awareness = 1;
        s.w.enemies[idx].st = EState::Broken; s.w.enemies[idx].stT = 0;
        s.step({Act::Attack}, 2);
        s.step({}, 140);
        CHECK(!s.w.enemies[idx].alive(), "not executed st=%d", (int)s.w.enemies[idx].st);
        CHECK(s.save.counter("executions") == 1, "executions=%d", s.save.counter("executions"));
    });

    test("stealth kill from behind an unaware guard", [] {
        Sim s; if (!s.ok) return;
        s.place("edge", 10);
        s.settle();
        s.w.enemies.clear();
        s.addEnemy(EType::Swordsman, s.w.player.pos + Vec2{60, 0}, 1);  // facing away
        s.step({}, 2);
        CHECK(s.w.findStealthTarget() == 0, "no stealth target");
        s.step({Act::Attack}, 2);
        s.step({}, 120);
        CHECK(!s.w.enemies[0].alive(), "guard alive");
        CHECK(s.save.counter("stealthKills") == 1, "stealthKills=%d", s.save.counter("stealthKills"));
    });

    test("crouching in tall grass hides from a watching guard", [] {
        Sim s; if (!s.ok) return;
        s.place("edge", 9);
        s.settle();
        s.w.enemies.clear();
        s.addEnemy(EType::Swordsman, s.w.player.pos + Vec2{300, 0}, -1);   // looking right at us
        s.step({Act::Down}, 240);
        CHECK(s.w.player.hidden, "not hidden");
        CHECK(s.w.enemies[0].awareness < 0.3f, "awareness %f", s.w.enemies[0].awareness);
        s.step({}, 240);
        CHECK(s.w.enemies[0].awareness >= 1, "standing in the open went unseen (%f)", s.w.enemies[0].awareness);
    });

    test("bell gate opens to a thrown shuriken, not a sword", [] {
        Sim s; if (!s.ok) return;
        Mechanism* gate = s.w.mech("bellgate");
        CHECK(gate && !gate->active, "gate missing or open");
        s.place("bellgate", 8);   // standing on the high ledge
        // find the ledge: walk until on the plank row
        for (auto& r : s.w.level.rooms) if (r.id == "bellgate") { s.w.player.pos = s.w.player.prevPos = {(r.x0 + 8.5f) * TILE, 15 * TILE - 0.01f}; }
        s.settle();
        s.w.player.facing = 1;
        s.save.shuriken = 5;
        s.step({Act::Shuriken}, 2);
        s.step({}, 200);
        CHECK(gate->active, "gate did not open");
        CHECK(s.save.has("open:bellgate"), "flag not saved");
    });

    test("cutting the ravine rope drops the bridge and it holds the player", [] {
        Sim s; if (!s.ok) return;
        Mechanism* bridge = s.w.mech("bridge1");
        CHECK(bridge && !bridge->active, "bridge missing");
        s.place("ravine", 15);
        s.settle();
        s.w.player.facing = 1;
        s.step({Act::Attack}, 2);
        s.step({}, 360);
        CHECK(bridge->active && bridge->t >= 1, "bridge t=%f", bridge->t);
        float y0 = s.w.player.pos.y;
        s.step({Act::Right}, 150);
        CHECK(std::abs(s.w.player.pos.y - y0) < 4, "fell: dy=%f", s.w.player.pos.y - y0);
    });

    test("audio synthesis is finite", [] {
        for (int i = 0; i < (int)Sfx::Count; i++) {
            auto b = synthesizeSfx((Sfx)i, 1);
            bool finite = !b.empty();
            float peak = 0;
            for (float v : b) { if (!std::isfinite(v)) finite = false; peak = std::max(peak, std::abs(v)); }
            CHECK(finite && peak > 0.01f && peak <= 1.01f, "sfx %d peak %f", i, peak);
        }
        Audio a;
        std::vector<float> buf(48000 * 2);
        for (MusicMode m : {MusicMode::Title, MusicMode::Combat, MusicMode::Boss, MusicMode::Shrine}) {
            a.setMusic(m); a.setIntensity(1);
            a.play(Sfx::Deflect); a.play(Sfx::Execute);
            AmbienceParams amb; amb.fire = 1; amb.crickets = 1; a.setAmbience(amb);
            a.renderOffline(buf.data(), 48000);
            bool finite = true; float peak = 0;
            for (float v : buf) { if (!std::isfinite(v)) finite = false; peak = std::max(peak, std::abs(v)); }
            CHECK(finite && peak > 0.001f && peak <= 1.0f, "mode %d peak %f", (int)m, peak);
        }
    });

    test("boss fight: two deathblows, phase two ignites", [] {
        Sim s; if (!s.ok) return;
        CHECK(s.w.bossIdx >= 0, "no boss");
        Enemy& b = s.w.enemies[s.w.bossIdx];
        s.w.player.pos = s.w.player.prevPos = b.pos + Vec2{-90, 0};
        s.w.startBoss();
        CHECK(s.w.bossActive, "boss not active");
        b.st = EState::Broken; b.stT = 0; b.facing = -1;
        s.step({Act::Attack}, 2); s.step({}, 140);
        CHECK(b.deathblows == 1 && b.phase2 && b.alive(), "after first deathblow: pips=%d phase2=%d", b.deathblows, b.phase2);
        s.step({}, 500);   // kneel, rise burning
        b.st = EState::Broken; b.stT = 0;
        s.w.player.pos = s.w.player.prevPos = b.pos + Vec2{-90, 0};
        s.step({Act::Attack}, 2); s.step({}, 140);
        CHECK(!b.alive(), "boss survived");
        CHECK(s.save.has("boss_tetsugen") && s.save.count("iron_vow") == 1, "boss rewards");
    });

    test("wind-beacon trial awakens the Gale Stance", [] {
        Sim s; if (!s.ok) return;
        Mechanism* stone = nullptr;
        for (auto& m : s.w.mechs) if (m.kind == MechKind::Trial) stone = &m;
        CHECK(stone && !stone->active, "no trial stone");
        if (!stone) return;
        s.w.player.pos = s.w.player.prevPos = stone->pos + Vec2{-30, 0};
        s.settle();
        s.step({Act::Interact}, 2);
        CHECK(s.w.trialActive && s.w.trialRemaining() == 5, "trial not started (%d foes)", s.w.trialRemaining());
        for (int id : s.w.trialEnemies) s.w.killEnemy(id, false, false);
        s.step({}, 10);
        CHECK(!s.w.trialActive && s.save.has("gale") && stone->active, "trial not won");
        CHECK(s.save.counter("trialBestMs") > 0, "trial time not recorded");
        // gale: faster run, wind blade on the third cut
        s.place("street", 10); s.settle();
        float x0 = s.w.player.pos.x; s.step({Act::Right}, 120);
        CHECK(s.w.player.pos.x - x0 > 460, "gale speed %f", s.w.player.pos.x - x0);
        s.settle();
        size_t before = s.w.projectiles.size();
        for (int i = 0; i < 3; i++) { s.step({Act::Attack}, 2); s.step({}, 18); }
        bool blade = false;
        for (auto& pr : s.w.projectiles) if (pr.type == ProjType::Gale) blade = true;
        CHECK(blade || s.w.projectiles.size() > before, "no wind blade");
    });

    test("trial fails when time runs out, and can be retried", [] {
        Sim s; if (!s.ok) return;
        Mechanism* stone = nullptr;
        for (auto& m : s.w.mechs) if (m.kind == MechKind::Trial) stone = &m;
        if (!stone) return;
        s.w.player.pos = s.w.player.prevPos = stone->pos + Vec2{-30, 0};
        s.settle();
        s.w.startTrial(*stone);
        s.w.trialT = stone->w + 1;
        s.step({}, 2);
        CHECK(!s.w.trialActive && !s.save.has("gale") && s.w.trialRemaining() == 0, "failure not handled");
        s.w.startTrial(*stone);
        CHECK(s.w.trialActive, "retry blocked");
    });

    test("stance skills are sealed until the Gale awakens", [] {
        SaveData s = newGame(0); s.skillPoints = 3;
        const SkillDef* rising = nullptr;
        for (auto& d : skillDefs()) if (d.id == "aura_rising") rising = &d;
        CHECK(rising && !canLearn(s, *rising), "stance learnable without gale");
        s.flags.insert("gale");
        CHECK(canLearn(s, *rising), "stance not learnable with gale");
    });

    test("Kage Rank rewards variety and drops when hit", [] {
        Sim s; if (!s.ok) return;
        for (int i = 0; i < 6; i++) s.w.addStyle(World::SHit, 14);
        float mashing = s.w.style;
        s.w.loseStyle(1000);
        s.w.addStyle(World::SHit, 14); s.w.addStyle(World::SDeflect, 45); s.w.addStyle(World::SDodge, 40); s.w.addStyle(World::SExecute, 90);
        CHECK(s.w.style > mashing * 2, "variety %f vs mashing %f", s.w.style, mashing);
        CHECK(s.w.styleRank >= 2, "rank %d", s.w.styleRank);
        AttackInfo a; a.damage = 10; a.origin = s.w.player.pos + Vec2{50, 0};
        s.w.hitPlayer(a);
        CHECK(s.w.styleRank <= 1, "rank kept after hit: %d", s.w.styleRank);
        ChapterResult r = scoreChapter(s.save);
        CHECK(!r.grade.empty(), "no grade");
    });

    test("shuriken follow the right-stick aim", [] {
        Sim s; if (!s.ok) return;
        s.place("street", 10); s.settle();
        s.save.shuriken = 3;
        InputFrame f; f.down[(size_t)Act::Shuriken] = true; f.aimX = -0.7f; f.aimY = -0.7f;
        s.input.injectFrame(f); s.input.tick(kDt); s.w.update(kDt);
        bool ok = false;
        for (auto& pr : s.w.projectiles) if (pr.type == ProjType::Shuriken && pr.vel.x < -500 && pr.vel.y < -500) ok = true;
        CHECK(ok, "shuriken not thrown up-left");
    });

    std::printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
