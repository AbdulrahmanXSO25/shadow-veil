#pragma once
#include "game/world.h"
#include "game/cutscene.h"
#include "game/progression.h"
#include <memory>
#include <deque>
#include <functional>

struct SDL_Window;

namespace sv {

class Renderer;

extern const char* const kSlotRows[7];
std::vector<std::string> candidates(const SaveData& s, int row);

enum class Screen { HowTo, Title, Playing, Pause, Equipment, Skills, Collection, Achievements, Settings, Shrine, Forge, Lore, Death, ChapterEnd, Credits, Confirm };

struct GameOptions {
    bool skipTitle = false;         // jump straight into play (testing)
    std::string startAt;            // spawn id or room id to start at
    bool autoCutscenes = false;     // auto-advance dialogue
    bool freshSave = false;         // ignore any save on disk
};

class Game {
public:
    bool init(Renderer* r, Audio* a, Input* in, SDL_Window* win, const GameOptions& opt);
    void tick(float dt);
    void render(float alpha);
    bool quitRequested() const { return m_quit; }
    World* world() { return m_world.get(); }
    Screen screen() const { return m_screen; }
    const SaveData& save() const { return m_save; }
    SaveData& saveMut() { return m_save; }
    const Profile& profile() const { return m_profile; }
    Settings& settings() { return m_settings; }
    void applySettings();
    void startNewGame(int ngPlus);
    void teleport(const std::string& target);
    CutscenePlayer& cutscenes() { return m_cut; }
    float gameSpeed() const { return m_settings.gameSpeed; }
    // capture/direction hooks
    void setClean(bool clean) { m_clean = clean; }
    void setQuiet(bool quiet) { m_quiet = quiet; }
    void debugScreen(const std::string& name);
    void setMusicVolume(float v) { m_musicOverride = v; if (m_audio) m_audio->setVolumes(m_settings.master, v, m_settings.sfx); }

private:
    bool loadWorld(bool titleMode);
    void handleWorldEvents();
    void unlock(const std::string& achievement);
    void toast(const std::string& title, const std::string& sub = "");
    void persist();
    void updateController(float dt);
    int menuNav(bool vertical = true);
    void openScreen(Screen s);
    void startCutscene(const std::string& name);
    void ask(const std::string& question, std::function<void()> yes, Screen back);

    // per-screen update
    void tickTitle(float dt);
    void tickPlaying(float dt);
    void tickPause(float dt);
    void tickEquipment(float dt);
    void tickSkills(float dt);
    void tickCollection(float dt);
    void tickAchievements(float dt);
    void tickSettings(float dt);
    void tickShrine(float dt);
    void tickForge(float dt);
    void tickLore(float dt);
    void tickDeath(float dt);
    void tickChapterEnd(float dt);
    void tickCredits(float dt);
    void tickHowTo(float dt);
    void tickConfirm(float dt);

    // drawing (game_ui.cpp)
    void drawHUD();
    void drawDialogue();
    void drawTitle();
    void drawMenuList(const std::vector<std::string>& items, int sel, Vec2 at, float size, const std::vector<bool>* enabled = nullptr);
    void drawPause();
    void drawEquipment();
    void drawSkills();
    void drawCollection();
    void drawAchievements();
    void drawSettings();
    void drawShrine();
    void drawForge();
    void drawLore();
    void drawDeath();
    void drawChapterEnd();
    void drawCredits();
    void drawHowTo();
    void drawConfirm();
    void drawToasts();
    void drawHint();
    void drawRoomCard();
    void drawPanel(const Rect& r, float alpha = 0.82f);
    void drawSeal(Vec2 c, float size, const std::string& kanji, Color bg);
    void drawHeader(const std::string& kanji, const std::string& title, const std::string& sub);
    void drawStatsBlock(Vec2 at);
    std::string hintText(const std::string& id) const;

    Renderer* m_r = nullptr;
    Audio* m_audio = nullptr;
    Input* m_in = nullptr;
    SDL_Window* m_win = nullptr;
    GameOptions m_opt;
    std::unique_ptr<World> m_world;
    CutsceneLibrary m_lib;
    CutscenePlayer m_cut;
    SaveData m_save;
    Profile m_profile;
    Settings m_settings;
    Screen m_screen = Screen::Title;
    Screen m_prevScreen = Screen::Playing;
    bool m_quit = false;
    bool m_titleMode = true;
    bool m_atShrine = false;
    float m_screenT = 0, m_time = 0;
    int m_sel = 0, m_sel2 = 0, m_col = 0;
    bool m_listMode = false;
    // independent auto-repeat state per axis (0 vertical, 1 horizontal)
    float m_navRepeat[2] = {0, 0}; int m_navDir[2] = {0, 0};
    std::string m_loreId, m_message;
    float m_messageT = 0;
    std::string m_confirmText; std::function<void()> m_confirmYes; Screen m_confirmBack = Screen::Title;
    struct Toast { std::string title, sub; float t = 0; bool achievement = false; };
    std::deque<Toast> m_toasts;
    std::string m_hint; float m_hintT = 0;
    std::string m_roomCard, m_roomKanji; float m_roomCardT = 0; int m_lastRoom = -1;
    float m_hpGhost = 100;
    float m_bossBarT = 0;
    std::string m_bossName, m_bossKanji;
    float m_titleFade = 0;
    int m_pendingNewGame = -1;
    float m_transition = 0; bool m_transitionOut = false;
    std::function<void()> m_afterFade;
    float m_monShowT = 0; int m_lastMon = 0;
    float m_deathT = 0;
    float m_restT = -1;
    float m_chapterTime = 0;
    float m_heartT = 0, m_heartSecond = 0;
    int m_trialTick = -1;
    int m_pendingRecord = -1;
    bool m_clean = false, m_quiet = false;
    float m_musicOverride = -1;
    std::string m_pendingGrade;
};

} // namespace sv
