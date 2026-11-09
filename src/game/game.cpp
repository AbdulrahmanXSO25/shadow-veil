#include "game/game.h"
#include "platform/platform.h"
#include "render/renderer.h"
#include "core/util.h"
#include <SDL3/SDL.h>

namespace sv {


bool Game::init(Renderer* r, Audio* a, Input* in, SDL_Window* win, const GameOptions& opt) {
    m_r = r; m_audio = a; m_in = in; m_win = win; m_opt = opt;
    loadSettings(m_settings);
    loadProfile(m_profile);
    applySettings();
    m_cut.autoAdvance = opt.autoCutscenes;
    if (opt.skipTitle) {
        if (!opt.freshSave && loadGame(m_save)) {}
        else m_save = newGame(0);
        const auto& chs = chapterDefs();
        if (opt.chapter >= 1 && opt.chapter <= (int)chs.size()) beginChapter(m_save, chs[opt.chapter - 1], true);
        if (!loadWorld(false)) return false;
        if (!opt.startAt.empty()) teleport(opt.startAt);
        m_screen = Screen::Playing;
    } else {
        if (!loadWorld(true)) return false;
        m_screen = Screen::Title;
    }
    return true;
}

void Game::debugScreen(const std::string& n) {
    static const std::pair<const char*, Screen> table[] = {
        {"playing", Screen::Playing}, {"pause", Screen::Pause}, {"equipment", Screen::Equipment}, {"skills", Screen::Skills},
        {"collection", Screen::Collection}, {"achievements", Screen::Achievements}, {"settings", Screen::Settings},
        {"howto", Screen::HowTo}, {"camp", Screen::Shrine}, {"forge", Screen::Forge}, {"title", Screen::Title}};
    for (auto& [k, s] : table)
        if (n == k) {
            if (s == Screen::Title) { loadWorld(true); m_titleFade = 0; }
            Screen prev = m_screen == Screen::Title ? Screen::Title : Screen::Playing;
            openScreen(s);
            m_prevScreen = s == Screen::Playing ? Screen::Playing : prev;
            if (m_audio && s != Screen::Playing && s != Screen::Title) m_audio->setPaused(false);   // trailers keep sound
            return;
        }
}

void Game::applySettings() {
    if (m_audio) m_audio->setVolumes(m_settings.master, m_musicOverride >= 0 ? m_musicOverride : m_settings.music, m_settings.sfx);
    if (m_in) { m_in->guardToggle = m_settings.guardToggle; m_in->setRumbleEnabled(m_settings.rumble); }
    if (m_win) {
        SDL_SetWindowFullscreen(m_win, m_settings.fullscreen);
        SDL_GL_SetSwapInterval(m_settings.vsync ? 1 : 0);
    }
}

bool Game::loadWorld(bool titleMode) {
    m_titleMode = titleMode;
    m_world = std::make_unique<World>();
    std::string err;
    SaveData* s = &m_save;
    if (titleMode) { m_save = newGame(0); m_save.flags.insert("t:title"); }
    // an unknown level id (a save from a newer build, or hand-edited) falls back to Chapter I
    if (!findChapter(m_save.level)) { logWarn("unknown level '%s', starting Chapter I", m_save.level.c_str()); m_save.level = chapterDefs()[0].level; m_save.checkpoint.clear(); }
    const ChapterDef& ch = titleMode ? chapterDefs()[0] : chapterOf(m_save.level);
    m_lib = CutsceneLibrary();
    if (!m_lib.load(contentPath(ch.scriptPath), err)) { logError("cutscenes: %s", err.c_str()); return false; }
    m_cut = CutscenePlayer(); m_cut.autoAdvance = m_opt.autoCutscenes;
    if (!m_world->init(contentPath(ch.levelPath), s, &m_settings, m_audio, m_in, err)) { logError("level: %s", err.c_str()); return false; }
    if (titleMode) {
        m_titleSub = "Six chapters  ·  one thread";
        SaveData peek;
        if (hasSaveGame() && loadGame(peek)) { const ChapterDef& c = chapterOf(peek.level); m_titleSub = c.numeral + "  " + c.title; }
        // the title screen is the opening shot: Rei at the cliff camp under the moon
        for (size_t i = 0; i < m_world->triggerFired.size(); i++) m_world->triggerFired[i] = true;
        m_world->controlLocked = true;
        m_world->player.lockedPose = "kneel";
        m_world->player.facing = 1;
        m_world->cam.cinematic = true;
        m_world->cam.cinTo = m_world->player.pos + Vec2{220, -300};
        m_world->cam.cinZoomTo = 0.92f; m_world->cam.cinDur = 0;
        m_world->hasMusicOverride = true; m_world->musicOverride = MusicMode::Title;
    }
    m_lastRoom = -1;
    m_hpGhost = m_world->player.hp;
    return true;
}

void Game::startNewGame(int ngPlus) {
    SaveData carry = m_save;
    bool hadSave = loadGame(carry);
    m_save = ngPlus > 0 && hadSave ? newGame(ngPlus, &carry) : newGame(0);
    if (ngPlus > 0) unlock("ngplus");
    loadWorld(false);
    m_chapterTime = 0;
    persist();
    m_screen = Screen::Playing;
}

void Game::teleport(const std::string& target) {
    World& w = *m_world;
    // debugging/testing jump: drop any running scene and its presentation state
    m_cut = CutscenePlayer(); m_cut.autoAdvance = m_opt.autoCutscenes;
    w.controlLocked = false; w.cam.cinematic = false; w.player.lockedPose.clear();
    w.hasMusicOverride = false;
    m_save.flags.insert("t:" + std::string("intro"));
    for (auto& s : w.level.spawns)
        if (s.id == target || s.kind == target) {
            w.player.pos = w.player.prevPos = s.pos + Vec2{-60, 0}; w.cam.pos = w.cam.prevPos = s.pos;
            for (size_t i = 0; i < w.level.triggers.size(); i++) {
                const Trigger& t = w.level.triggers[i];
                if (t.room < s.room || (t.room == s.room && t.area.x < w.player.pos.x + TILE)) w.triggerFired[i] = true;
            }
            return;
        }
    // "room" or "room:col" (room-local column)
    std::string roomId = target; int col = 3;
    if (auto c = target.find(':'); c != std::string::npos) { roomId = target.substr(0, c); col = std::stoi(target.substr(c + 1)); }
    for (auto& r : w.level.rooms)
        if (r.id == roomId) {
            float x = (r.x0 + col) * TILE;
            // find the first standable spot in the column
            for (int y = 2; y < w.level.height - 1; y++)
                if (!w.level.solidAt(r.x0 + col, y) && !w.level.solidAt(r.x0 + col, y - 1) && w.level.solidAt(r.x0 + col, y + 1)) {
                    w.player.pos = w.player.prevPos = {x + TILE / 2, (y + 1) * TILE - 0.01f};
                    break;
                }
            w.cam.pos = w.cam.prevPos = w.player.pos + Vec2{0, -185};
            // anything before this point (earlier rooms, or behind us in this one) counts as already seen
            int ri = (int)(&r - &w.level.rooms[0]);
            for (size_t i = 0; i < w.level.triggers.size(); i++) {
                const Trigger& t = w.level.triggers[i];
                if (t.room < ri || (t.room == ri && t.area.x < w.player.pos.x + TILE)) w.triggerFired[i] = true;
            }
            return;
        }
    logWarn("teleport: unknown target %s", target.c_str());
}

void Game::persist() {
    if (m_titleMode) return;
    m_save.valid = true;
    if (!saveGame(m_save)) logWarn("could not write save");
    saveProfile(m_profile);
}

void Game::unlock(const std::string& id) {
    if (m_profile.achievements.count(id)) return;
    m_profile.achievements.insert(id);
    for (auto& a : achievementDefs())
        if (a.id == id) { Toast t{a.name, a.desc, 0, true}; m_toasts.push_back(t); }
    if (m_audio) m_audio->play(Sfx::UiConfirm, 0.6f, 0.75f);
    saveProfile(m_profile);
    platform::unlockAchievement(id);
}

void Game::toast(const std::string& title, const std::string& sub) {
    m_toasts.push_back({title, sub, 0, false});
    if (m_toasts.size() > 5) m_toasts.pop_front();
}

void Game::openScreen(Screen s) {
    m_prevScreen = m_screen;
    m_screen = s;
    m_screenT = 0; m_sel = 0; m_sel2 = 0; m_col = 0; m_listMode = false;
    if (s == Screen::Collection) m_col = chapterOf(m_save.level).number - 1;   // open on the current chapter
    // a direction still held from the previous screen must be released before it navigates here
    if (m_in) {
        m_navDir[0] = m_in->held(Act::Up) ? -1 : (m_in->held(Act::Down) ? 1 : 0);
        m_navDir[1] = m_in->held(Act::Left) ? -1 : (m_in->held(Act::Right) ? 1 : 0);
        m_navRepeat[0] = m_navRepeat[1] = 0.45f;
    }
    if (m_audio) m_audio->setPaused(s != Screen::Playing && s != Screen::Title && s != Screen::Death && s != Screen::ChapterEnd);
}

void Game::startCutscene(const std::string& name) { m_cut.start(m_lib, name, *m_world); }

void Game::ask(const std::string& q, std::function<void()> yes, Screen back) {
    m_confirmText = q; m_confirmYes = std::move(yes); m_confirmBack = back;
    Screen prev = m_screen;
    openScreen(Screen::Confirm);
    m_prevScreen = prev;
    m_sel = 1;
}

int Game::menuNav(bool vertical) {
    // One step per press; holding repeats after a pause, at a readable pace.
    int axis = vertical ? 0 : 1;
    Act neg = vertical ? Act::Up : Act::Left, pos = vertical ? Act::Down : Act::Right;
    int dir = m_in->held(neg) ? -1 : (m_in->held(pos) ? 1 : 0);
    if (dir == 0) { m_navDir[axis] = 0; return 0; }
    if (dir != m_navDir[axis]) { m_navDir[axis] = dir; m_navRepeat[axis] = 0.45f; if (m_audio) m_audio->play(Sfx::UiMove, 0.5f); return dir; }
    m_navRepeat[axis] -= kDt;
    if (m_navRepeat[axis] <= 0) { m_navRepeat[axis] = 0.14f; if (m_audio) m_audio->play(Sfx::UiMove, 0.4f); return dir; }
    return 0;
}

// =================================================================== tick

void Game::tick(float dt) {
    m_time += dt;
    m_screenT += dt;
    for (auto& t : m_toasts) t.t += dt;
    while (!m_toasts.empty() && m_toasts.front().t > (m_toasts.front().achievement ? 5.5f : 3.8f)) m_toasts.pop_front();
    m_hintT = std::max(0.f, m_hintT - dt);
    m_roomCardT += dt;
    m_messageT = std::max(0.f, m_messageT - dt);

    // fade transitions (rest, scene changes)
    if (m_transitionOut) {
        m_transition = std::min(1.f, m_transition + dt * 2.2f);
        if (m_transition >= 1) { m_transitionOut = false; if (m_afterFade) { auto f = m_afterFade; m_afterFade = nullptr; f(); } }
    } else m_transition = std::max(0.f, m_transition - dt * 1.5f);

    switch (m_screen) {
    case Screen::Title: tickTitle(dt); break;
    case Screen::Playing: tickPlaying(dt); break;
    case Screen::Pause: tickPause(dt); break;
    case Screen::Equipment: tickEquipment(dt); break;
    case Screen::Skills: tickSkills(dt); break;
    case Screen::Collection: tickCollection(dt); break;
    case Screen::Achievements: tickAchievements(dt); break;
    case Screen::Settings: tickSettings(dt); break;
    case Screen::Shrine: tickShrine(dt); break;
    case Screen::Forge: tickForge(dt); break;
    case Screen::Lore: tickLore(dt); break;
    case Screen::Death: tickDeath(dt); break;
    case Screen::ChapterEnd: tickChapterEnd(dt); break;
    case Screen::Credits: tickCredits(dt); break;
    case Screen::Confirm: tickConfirm(dt); break;
    case Screen::HowTo: tickHowTo(dt); break;
    case Screen::Chapters: tickChapters(dt); break;
    }
}

void Game::tickTitle(float dt) {
    World& w = *m_world;
    m_in->setLightbar(0.15f, 0.2f, 0.45f);
    w.update(dt);
    m_titleFade = std::min(1.f, m_titleFade + dt * 0.5f);
    if (m_transitionOut) return;
    bool hasSave = hasSaveGame();
    std::vector<std::string> items;
    std::vector<std::function<void()>> acts;
    if (hasSave) { items.push_back("Continue"); acts.push_back([&] { m_transitionOut = true; m_afterFade = [this] { loadGame(m_save); loadWorld(false); m_screen = Screen::Playing; m_chapterTime = m_save.playTime; }; }); }
    items.push_back("New Game");
    acts.push_back([&, hasSave] {
        auto go = [this] { m_transitionOut = true; m_afterFade = [this] { startNewGame(0); }; };
        if (hasSave) ask("Begin anew? Your current journey will be lost.", go, Screen::Title); else go();
    });
    if (m_profile.ngPlusUnlocked) {
        items.push_back("New Game+");
        acts.push_back([&] { ask("Begin New Game+? You keep your gear and skills; foes grow stronger.", [this] { m_transitionOut = true; m_afterFade = [this] { int lvl = std::max(1, m_save.ngPlus + 1); SaveData s; if (loadGame(s)) lvl = s.ngPlus + 1; startNewGame(lvl); }; }, Screen::Title); });
    }
    if (m_profile.chaptersCleared >= 1) { items.push_back("Chapters"); acts.push_back([&] { openScreen(Screen::Chapters); }); }
    items.push_back("How to Play"); acts.push_back([&] { openScreen(Screen::HowTo); });
    items.push_back("Settings"); acts.push_back([&] { openScreen(Screen::Settings); });
    items.push_back("Achievements"); acts.push_back([&] { openScreen(Screen::Achievements); });
    items.push_back("Credits"); acts.push_back([&] { openScreen(Screen::Credits); });
    items.push_back("Quit"); acts.push_back([&] { m_quit = true; });
    m_sel = std::clamp(m_sel, 0, (int)items.size() - 1);
    int d = menuNav();
    if (d) m_sel = (m_sel + d + (int)items.size()) % (int)items.size();
    if (m_in->pressed(Act::Confirm) && m_titleFade > 0.4f) {
        if (m_audio) m_audio->play(Sfx::UiConfirm, 0.7f);
        int s = m_sel;
        acts[s]();
        if (m_screen != Screen::Title) m_prevScreen = Screen::Title;
    }
}

void Game::tickPlaying(float dt) {
    World& w = *m_world;
    bool cutscene = m_cut.active();
    if (!cutscene && !m_transitionOut) {
        if (m_in->pressed(Act::Pause)) { openScreen(Screen::Pause); if (m_audio) m_audio->play(Sfx::UiConfirm, 0.5f, 0.8f); return; }
        if (m_in->pressed(Act::Menu)) { m_atShrine = false; openScreen(Screen::Equipment); m_prevScreen = Screen::Playing; return; }
    }
    if (cutscene) {
        bool confirm = m_in->pressed(Act::Confirm) || m_in->pressed(Act::Attack) || m_in->pressed(Act::Jump);
        if (m_cut.choosing()) {
            int d = menuNav(true);
            if (d) m_cut.navChoice(d);
            confirm = m_in->pressed(Act::Confirm) || m_in->pressed(Act::Attack);
        }
        m_cut.update(w, dt, confirm, m_in->held(Act::Pause));
        for (auto& req : m_cut.takeRequests()) {
            if (req == "save") persist();
            else if (req == "chapter_end") finishChapter();
        }
    } else m_cut.update(w, dt, false, false);
    w.update(dt);
    if (!m_titleMode) { m_save.playTime += dt; m_chapterTime += dt; }
    handleWorldEvents();
    // room title cards on first entry
    int room = w.level.roomAt(w.player.pos.x);
    if (room != m_lastRoom) {
        if (m_lastRoom >= 0 || true) {
            const Room& R = w.level.rooms[room];
            std::string key = "visited:" + R.id;
            if (!m_save.has(key) && !R.name.empty()) {
                m_save.flags.insert(key);
                m_roomCard = R.name; m_roomCardT = 0;
            }
        }
        m_lastRoom = room;
    }
    updateController(dt);
    if (m_save.mon != m_lastMon) { m_monShowT = 3; m_lastMon = m_save.mon; }
    m_monShowT = std::max(0.f, m_monShowT - dt);
    if (m_save.skills.size() >= 5) unlock("skills_5");
}

// The pad speaks too: the light bar mirrors Rei's state and haptics carry the tension.
void Game::updateController(float dt) {
    World& w = *m_world;
    float hpF = w.player.hp / std::max(1.f, maxHpOf(m_save));
    float pulse = 0.5f + 0.5f * std::sin(m_time * 5);
    Color c = rgb(0x5a0810);                                   // Kagenui crimson, banked low
    if (w.hasGale()) c = rgb(0x1a8a80);
    if (m_save.has("ember") && w.player.ki >= (m_save.skills.count("veil_well") ? 25.f : 35.f)) c = lerp(c, rgb(0xff6a10), 0.5f + 0.3f * pulse);
    if (w.bossActive) c = lerp(c, rgb(0x900010), 0.6f);
    if (w.trialActive) c = rgb(0x30d0c0) * (0.6f + 0.4f * pulse);
    if (hpF < 0.3f && w.player.state != PState::Dead) c = rgb(0xff1010) * (0.3f + 0.7f * pulse);
    m_in->setLightbar(c.r, c.g, c.b);
    // heartbeat when near death
    m_heartT -= dt;
    if (hpF < 0.3f && w.player.state != PState::Dead && m_heartT <= 0) {
        m_in->rumble(0.35f, 0.0f, 70);
        m_heartT = 1.1f; m_heartSecond = 0.2f;
    }
    if (m_heartSecond > 0) { m_heartSecond -= dt; if (m_heartSecond <= 0) m_in->rumble(0.22f, 0.0f, 60); }
    // trial: a tick each second in the final ten
    if (w.trialActive) {
        int left = (int)(w.trialLimit - w.trialT);
        if (left < 10 && left != m_trialTick) { m_trialTick = left; m_in->rumble(0.0f, 0.25f, 40); if (m_audio) m_audio->play(Sfx::UiMove, 0.5f, 1.4f); }
    }
}

void Game::handleWorldEvents() {
    World& w = *m_world;
    auto events = std::move(w.events);
    w.events.clear();
    for (auto& e : events) {
        switch (e.type) {
        case WorldEvent::Toast: toast(e.a, e.b); break;
        case WorldEvent::Achievement: unlock(e.a); break;
        case WorldEvent::Hint:
            if (m_settings.hints || e.a == "bell") { m_hint = e.a; m_hintT = 7; }
            break;
        case WorldEvent::Cutscene: startCutscene(e.a); break;
        case WorldEvent::Dialogue: {
            CutsceneLibrary tmp;
            CutCommand lock{"lock", {}}, say{"say", {e.a, e.b}}, un{"unlock", {}};
            tmp.scenes["__examine"] = {lock, say, un};
            m_cut.start(tmp, "__examine", w);
            break;
        }
        case WorldEvent::ShrineMenu:
            m_atShrine = true;
            w.player.hp = maxHpOf(m_save);
            m_save.gourds = m_save.maxGourds;
            persist();
            openScreen(Screen::Shrine);
            break;
        case WorldEvent::Lore: m_loreId = e.a; openScreen(Screen::Lore); persist(); break;
        case WorldEvent::Death:
            m_save.bump("deaths");
            unlock("first_death");
            openScreen(Screen::Death);
            m_deathT = 0;
            break;
        case WorldEvent::BossStart: m_bossName = e.a; m_bossKanji = e.b; m_bossBarT = 0; break;
        case WorldEvent::BossDefeated: persist(); startCutscene("boss_outro"); break;
        case WorldEvent::ChapterEnd: openScreen(Screen::ChapterEnd); break;
        case WorldEvent::Checkpoint: persist(); break;
        }
    }
}

void Game::tickPause(float dt) {
    static const char* items[] = {"Resume", "Equipment", "Way of the Shadow", "Collection", "Achievements", "How to Play", "Settings", "Quit to Title"};
    const int n = 8;
    int d = menuNav();
    if (d) m_sel = (m_sel + d + n) % n;
    if (m_in->pressed(Act::Cancel) || m_in->pressed(Act::Pause)) { if (m_screenT > 0.1f) { openScreen(Screen::Playing); return; } }
    if (m_in->pressed(Act::Confirm)) {
        if (m_audio) m_audio->play(Sfx::UiConfirm, 0.6f);
        m_atShrine = false;
        switch (m_sel) {
        case 0: openScreen(Screen::Playing); break;
        case 1: openScreen(Screen::Equipment); m_prevScreen = Screen::Pause; break;
        case 2: openScreen(Screen::Skills); m_prevScreen = Screen::Pause; break;
        case 3: openScreen(Screen::Collection); m_prevScreen = Screen::Pause; break;
        case 4: openScreen(Screen::Achievements); m_prevScreen = Screen::Pause; break;
        case 5: openScreen(Screen::HowTo); m_prevScreen = Screen::Pause; break;
        case 6: openScreen(Screen::Settings); m_prevScreen = Screen::Pause; break;
        case 7: ask("Return to the title screen? Progress since your last camp is kept.", [this] {
                    persist();
                    m_transitionOut = true;
                    m_afterFade = [this] { loadWorld(true); openScreen(Screen::Title); m_titleFade = 0; };
                }, Screen::Pause); break;
        }
    }
    (void)items;
}

void Game::tickConfirm(float dt) {
    int dh = menuNav(false), dv = menuNav(true);
    if (dh || dv) m_sel = 1 - m_sel;
    if (m_in->pressed(Act::Cancel)) { Screen b = m_confirmBack; openScreen(b); return; }
    if (m_in->pressed(Act::Confirm) && m_screenT > 0.15f) {
        Screen b = m_confirmBack;
        if (m_sel == 0) {
            auto f = m_confirmYes;
            openScreen(b);
            if (f) f();
        } else openScreen(b);
    }
}

const char* const kSlotRows[7] = {"Blade", "Gem I", "Gem II", "Garb", "Mask", "Token I", "Token II"};

std::vector<std::string> candidates(const SaveData& s, int row) {
    std::vector<std::string> out;
    ItemSlot want = row == 0 ? ItemSlot::Blade : (row <= 2 ? ItemSlot::Gem : (row == 3 ? ItemSlot::Garb : (row == 4 ? ItemSlot::Mask : ItemSlot::Relic)));
    if (row != 0 && row != 3) out.push_back("");
    for (auto& [id, n] : s.inventory) {
        const ItemDef* d = findItem(id);
        if (d && d->slot == want && n > 0) out.push_back(id);
    }
    return out;
}

void Game::tickEquipment(float dt) {
    const int rows = 7;
    if (!m_listMode) {
        int d = menuNav();
        if (d) m_sel = (m_sel + d + rows) % rows;
        if (m_in->pressed(Act::Cancel) || m_in->pressed(Act::Menu)) { openScreen(m_prevScreen == Screen::Equipment ? Screen::Playing : m_prevScreen); return; }
        if (m_in->pressed(Act::Confirm)) {
            const ItemDef* blade = findItem(m_save.blade);
            int sockets = blade ? blade->sockets : 0;
            if ((m_sel == 1 && sockets < 1) || (m_sel == 2 && sockets < 2)) { m_message = "This blade has no socket there."; m_messageT = 2; return; }
            m_listMode = true; m_sel2 = 0;
            if (m_audio) m_audio->play(Sfx::UiConfirm, 0.4f, 1.3f);
        }
        return;
    }
    auto list = candidates(m_save, m_sel);
    if (list.empty()) { m_listMode = false; return; }
    int d = menuNav();
    if (d) m_sel2 = (m_sel2 + d + (int)list.size()) % (int)list.size();
    if (m_in->pressed(Act::Cancel)) { m_listMode = false; return; }
    if (m_in->pressed(Act::Confirm)) {
        const std::string& id = list[m_sel2];
        auto& gems = m_save.gems[m_save.blade];
        switch (m_sel) {
        case 0: m_save.blade = id; break;
        case 1: case 2: {
            size_t slot = m_sel - 1;
            // a gem can only sit in one socket
            int owned = m_save.count(id), used = 0;
            for (auto& [b, g] : m_save.gems) for (size_t i = 0; i < g.size(); i++) if (g[i] == id && !(b == m_save.blade && i == slot)) used++;
            if (!id.empty() && used >= owned) { m_message = "That gem is already socketed."; m_messageT = 2; return; }
            if (gems.size() <= slot) gems.resize(slot + 1);
            gems[slot] = id;
            break;
        }
        case 3: m_save.garb = id; break;
        case 4: m_save.mask = id; break;
        case 5: if (!id.empty() && id == m_save.relic2) m_save.relic2.clear(); m_save.relic1 = id; break;
        case 6: if (!id.empty() && id == m_save.relic1) m_save.relic1.clear(); m_save.relic2 = id; break;
        }
        m_world->stats = computeStats(m_save);
        m_world->player.hp = std::min(m_world->player.hp, maxHpOf(m_save));
        if (m_audio) m_audio->play(Sfx::UiConfirm, 0.6f);
        m_listMode = false;
    }
}

void Game::tickSkills(float dt) {
    auto& defs = skillDefs();
    int dv = menuNav(true), dh = menuNav(false);
    if (dv) m_sel = std::clamp(m_sel + dv, 0, 3);
    if (dh) m_col = (m_col + dh + 4) % 4;
    if (m_in->pressed(Act::Cancel)) { openScreen(m_prevScreen); return; }
    if (m_in->pressed(Act::Confirm)) {
        const SkillDef* sd = nullptr;
        for (auto& d : defs) if (d.branch == m_col && d.tier == m_sel) sd = &d;
        if (!sd) return;
        std::string why;
        if (!m_atShrine) { m_message = "Rest at a camp to learn new arts."; m_messageT = 2.5f; return; }
        if (!canLearn(m_save, *sd, &why)) { m_message = why; m_messageT = 2.5f; if (m_audio) m_audio->play(Sfx::UiBack, 0.6f); return; }
        m_save.skillPoints--;
        m_save.skills.insert(sd->id);
        m_world->stats = computeStats(m_save);
        if (m_audio) m_audio->play(Sfx::CampRest, 0.5f, 1.5f);
        m_message = sd->name + " learned."; m_messageT = 2.5f;
        if (m_save.skills.size() >= 5) unlock("skills_5");
        persist();
    }
}

void Game::tickCollection(float dt) {
    (void)dt;
    int chs = (int)chapterDefs().size();
    int tab = menuNav(false);
    if (m_in->pressed(Act::TabPrev)) tab = -1;
    if (m_in->pressed(Act::TabNext)) tab = 1;
    if (tab) { m_col = (m_col + tab + chs) % chs; m_sel = 0; }
    int n = std::max(1, loreCountForChapter(m_col + 1));
    int d = menuNav();
    if (d) m_sel = (m_sel + d + n) % n;
    if (m_in->pressed(Act::Cancel)) openScreen(m_prevScreen);
}

void Game::tickAchievements(float dt) {
    int n = (int)achievementDefs().size();
    int d = menuNav();
    if (d) m_sel = (m_sel + d + n) % n;
    if (m_in->pressed(Act::Cancel)) openScreen(m_prevScreen);
}

void Game::tickSettings(float dt) {
    const int n = 15;
    int d = menuNav();
    if (d) m_sel = (m_sel + d + n) % n;
    int h = menuNav(false);
    bool toggle = m_in->pressed(Act::Confirm) || h != 0;
    Settings& s = m_settings;
    auto adj = [&](float& v, float step, float lo, float hi) { if (h) v = std::clamp(std::round((v + h * step) / step) * step, lo, hi); };
    switch (m_sel) {
    case 0: adj(s.master, 0.1f, 0, 1); break;
    case 1: adj(s.music, 0.1f, 0, 1); break;
    case 2: adj(s.sfx, 0.1f, 0, 1); break;
    case 3: adj(s.shake, 0.25f, 0, 1); break;
    case 4: if (toggle) s.reduceFlashes = !s.reduceFlashes; break;
    case 5: adj(s.deflectAssist, 0.25f, 1, 2); break;
    case 6: adj(s.gameSpeed, 0.1f, 0.6f, 1); break;
    case 7: if (toggle) s.guardToggle = !s.guardToggle; break;
    case 8: adj(s.subtitleScale, 0.1f, 0.8f, 1.6f); break;
    case 9: if (toggle) s.highContrast = !s.highContrast; break;
    case 10: if (toggle) s.inkSplatter = !s.inkSplatter; break;
    case 11: if (toggle) s.hints = !s.hints; break;
    case 12: if (toggle) s.rumble = !s.rumble; break;
    case 13: if (toggle) s.fullscreen = !s.fullscreen; break;
    case 14: if (toggle) s.vsync = !s.vsync; break;
    }
    if (h || toggle) applySettings();
    if (m_in->pressed(Act::Cancel)) { saveSettings(m_settings); openScreen(m_prevScreen); }
}

void Game::tickShrine(float dt) {
    static const int n = 5;   // Rest, Meditate, Forge, Equipment, Leave
    if (m_restT >= 0) {
        m_restT += dt;
        if (m_restT > 1.1f) m_restT = -1;
        return;
    }
    int d = menuNav();
    if (d) m_sel = (m_sel + d + n) % n;
    if (m_in->pressed(Act::Cancel)) { openScreen(Screen::Playing); return; }
    if (m_in->pressed(Act::Confirm)) {
        if (m_audio) m_audio->play(Sfx::UiConfirm, 0.6f);
        switch (m_sel) {
        case 0:
            m_transitionOut = true;
            m_restT = 0;
            m_afterFade = [this] {
                m_world->respawnAtCheckpoint();
                m_world->player.facing = 1;
                if (m_audio) m_audio->play(Sfx::CampRest, 0.6f);
                persist();
                toast("Rested", "Vitality and gourds restored. Enemies have returned.");
            };
            break;
        case 1: { Screen s = Screen::Shrine; openScreen(Screen::Skills); m_prevScreen = s; m_atShrine = true; break; }
        case 2: { openScreen(Screen::Forge); m_prevScreen = Screen::Shrine; break; }
        case 3: { openScreen(Screen::Equipment); m_prevScreen = Screen::Shrine; break; }
        case 4: openScreen(Screen::Playing); break;
        }
    }
}

void Game::tickForge(float dt) {
    auto& rec = recipeDefs();
    int n = (int)rec.size();
    int d = menuNav();
    if (d) m_sel = (m_sel + d + n) % n;
    if (m_in->pressed(Act::Cancel)) { openScreen(Screen::Shrine); return; }
    if (m_in->pressed(Act::Confirm)) {
        std::string why;
        if (!canCraft(m_save, rec[m_sel], &why)) { m_message = why; m_messageT = 2.5f; if (m_audio) m_audio->play(Sfx::UiBack, 0.6f); return; }
        applyCraft(m_save, rec[m_sel]);
        m_world->stats = computeStats(m_save);
        if (m_audio) { m_audio->play(Sfx::Clang, 0.8f, 0.9f); m_audio->play(Sfx::Pickup, 0.5f); }
        m_message = rec[m_sel].name + " — done."; m_messageT = 2.5f;
        unlock("craft");
        persist();
    }
}

void Game::tickLore(float dt) {
    m_world->update(0);   // keep the camera stable behind the page
    if (m_screenT > 0.4f && (m_in->pressed(Act::Confirm) || m_in->pressed(Act::Cancel))) openScreen(Screen::Playing);
}

void Game::tickDeath(float dt) {
    m_deathT += dt;
    if (m_deathT > 1.6f && (m_in->pressed(Act::Confirm) || m_in->pressed(Act::Attack))) {
        m_transitionOut = true;
        m_afterFade = [this] {
            m_world->respawnAtCheckpoint();
            m_cut = CutscenePlayer(); m_cut.autoAdvance = m_opt.autoCutscenes;
            m_bossName.clear();
            persist();
            openScreen(Screen::Playing);
        };
    }
}

void Game::finishChapter() {
    const ChapterDef& ch = chapterOf(m_save.level);
    float played = m_save.playTime - (float)m_save.counter("@time");
    auto delta = [&](const char* k) { return m_save.counter(k) - m_save.counter(std::string("@") + k); };
    m_end = EndCard();
    m_end.number = ch.number; m_end.title = ch.title; m_end.numeral = ch.numeral;
    m_end.final = ch.number == (int)chapterDefs().size();
    int found = 0, total = 0;
    for (auto& l : loreDefs()) if (l.chapter == ch.number) { total++; if (m_save.has("lore:" + l.id)) found++; }
    m_end.rows = {
        {"Time", strfmt("%d:%02d", (int)(played / 60), (int)played % 60)}, {"Foes felled", strfmt("%d", delta("kills"))},
        {"Deflections", strfmt("%d", delta("deflects"))}, {"Executions", strfmt("%d", delta("executions"))},
        {"Stealth kills", strfmt("%d", delta("stealthKills"))}, {"Deaths", strfmt("%d", delta("deaths"))},
        {"Ink Scrolls", strfmt("%d / %d", found, total)},
    };
    m_end.res = scoreChapter(m_save);
    m_end.prevBest = m_profile.chapterBest.count(ch.level) ? m_profile.chapterBest[ch.level] : 0;
    m_end.prevGrade = m_profile.chapterGrade.count(ch.level) ? m_profile.chapterGrade[ch.level] : "";
    m_profile.chaptersCleared = std::max(m_profile.chaptersCleared, ch.number);
    if (m_profile.chapterTime[ch.level] <= 0 || played < m_profile.chapterTime[ch.level]) m_profile.chapterTime[ch.level] = played;
    if (ch.number == 1) {
        m_profile.ngPlusUnlocked = true;
        if (m_profile.bestTime <= 0 || played < m_profile.bestTime) m_profile.bestTime = played;
        if (played < 15 * 60) unlock("speedrun_c1");
        m_save.flags.insert("chapter1_complete");
    }
    if (m_end.res.grade[0] == 'S') unlock("grade_s");
    m_pendingRecord = m_end.res.score;
    m_pendingGrade = m_end.res.grade;
    m_save.flags.insert(ch.prefix + "_complete");
    if (m_end.final) {
        m_profile.ngPlusUnlocked = true;
        unlock("all_chapters");
        for (int k = 1; k <= 3; k++)
            if (m_save.has("ending_" + std::to_string(k))) { m_end.ending = k; unlock("ending_" + std::to_string(k)); m_profile.endings.insert(std::to_string(k)); }
    } else {
        // the save now points at the next chapter, so Continue resumes there even if the game is closed here
        const ChapterDef& next = chapterDefs()[ch.number];
        m_end.nextTitle = next.numeral + std::string("  ") + next.title;
        beginChapter(m_save, next, false);
    }
    persist();
    openScreen(Screen::ChapterEnd);
}

void Game::startChapter(const ChapterDef& c, bool replay) {
    SaveData s;
    if (!loadGame(s)) s = newGame(0);
    m_save = s;
    beginChapter(m_save, c, replay);
    loadWorld(false);
    m_chapterTime = 0;
    persist();
    openScreen(Screen::Playing);
}

void Game::tickChapterEnd(float dt) {
    (void)dt;
    if (m_pendingRecord >= 0 && m_screenT > 3.0f) {
        // the record is compared on the chapter screen before being stored
        const ChapterDef& ch = chapterDefs()[m_end.number - 1];
        if (m_pendingRecord > m_end.prevBest) {
            m_profile.chapterBest[ch.level] = m_pendingRecord; m_profile.chapterGrade[ch.level] = m_pendingGrade;
            if (ch.number == 1) { m_profile.bestScore = m_pendingRecord; m_profile.bestGrade = m_pendingGrade; }
            saveProfile(m_profile);
        }
        m_pendingRecord = -1;
    }
    int d = menuNav();
    if (d) m_sel = (m_sel + d + 2) % 2;
    if (m_screenT > 2.5f && m_in->pressed(Act::Confirm)) {
        if (m_sel == 0 && m_end.final) { openScreen(Screen::Credits); m_prevScreen = Screen::ChapterEnd; }
        else if (m_sel == 0) { m_transitionOut = true; m_afterFade = [this] { loadWorld(false); m_chapterTime = 0; openScreen(Screen::Playing); }; }
        else { m_transitionOut = true; m_afterFade = [this] { loadWorld(true); openScreen(Screen::Title); m_titleFade = 0; }; }
    }
}

void Game::tickChapters(float dt) {
    (void)dt;
    int n = (int)chapterDefs().size();
    int d = menuNav();
    if (d) m_sel = (m_sel + d + n) % n;
    if (m_in->pressed(Act::Cancel)) { openScreen(Screen::Title); return; }
    if (m_in->pressed(Act::Confirm)) {
        const ChapterDef& c = chapterDefs()[m_sel];
        if (c.number > m_profile.chaptersCleared + 1) { if (m_audio) m_audio->play(Sfx::UiBack, 0.6f); return; }
        int idx = m_sel;
        ask(strfmt("Play %s? Gear and skills carry over; the chapter's items and scenes start fresh.", c.title.c_str()),
            [this, idx] { m_transitionOut = true; m_afterFade = [this, idx] { startChapter(chapterDefs()[idx], true); }; }, Screen::Chapters);
    }
}

void Game::tickHowTo(float dt) {
    int d = menuNav(false);
    if (m_in->pressed(Act::TabPrev)) d = -1;
    if (m_in->pressed(Act::TabNext)) d = 1;
    if (d) m_col = (m_col + d + 2) % 2;   // Controls / Techniques pages
    if (m_in->pressed(Act::Cancel) || (m_screenT > 0.3f && m_in->pressed(Act::Confirm))) openScreen(m_prevScreen == Screen::HowTo ? Screen::Title : m_prevScreen);
}

void Game::tickCredits(float dt) {
    (void)dt;
    if (m_in->pressed(Act::Cancel) || (m_screenT > 1 && m_in->pressed(Act::Confirm))) {
        if (m_prevScreen == Screen::ChapterEnd) { m_transitionOut = true; m_afterFade = [this] { loadWorld(true); openScreen(Screen::Title); m_titleFade = 0; }; }
        else openScreen(Screen::Title);
    }
}

} // namespace sv
