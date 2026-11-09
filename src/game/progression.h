#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>

namespace sv {

enum class ItemSlot { Blade, Garb, Mask, Relic, Gem, Material, Key };
const char* slotName(ItemSlot s);

struct Stats {
    float attack = 0, maxHp = 0, defense = 0, postureDmg = 0, deflectWindow = 0, kiGain = 0;
    float stealth = 0, emberDmg = 0, healAmt = 0, executeHeal = 0, comboEmber = 0, dashCooldown = 0;
    Stats& operator+=(const Stats& o);
};

struct ItemDef {
    std::string id, name, kanji;
    ItemSlot slot;
    std::string desc;
    Stats mods;
    int sockets = 0;
    std::string lore;
};
const std::vector<ItemDef>& itemDefs();
const ItemDef* findItem(const std::string& id);

struct SkillDef {
    std::string id, name, kanji, desc;
    int branch = 0, tier = 0;   // branch: 0 Blade, 1 Veil, 2 Shadow, 3 Wind
};
const std::vector<SkillDef>& skillDefs();
const char* branchName(int b);

struct RecipeDef {
    std::string id, name, desc;
    std::vector<std::pair<std::string, int>> inputs;
    int mon = 0;
    std::string output;         // item id, or "@temper", "@gourd", "@shuriken"
};
const std::vector<RecipeDef>& recipeDefs();

struct AchievementDef { std::string id, name, desc; bool hidden = false; };
const std::vector<AchievementDef>& achievementDefs();

struct LoreDef { std::string id, title, text; int chapter = 1; };
const std::vector<LoreDef>& loreDefs();
const LoreDef* findLore(const std::string& id);

// The six chapters, in order. `level` is the level id stored in saves.
struct ChapterDef {
    int number;                  // 1..6
    std::string level, levelPath, scriptPath;
    std::string title, kanji, numeral;   // "The Drowned Terrace", "水没段", "第二章"
    std::string stance;          // flag the chapter's trial awakens ("" if none)
    std::string bossFlag;        // set when the chapter's champion falls
    std::string prefix;          // spawn/trigger id prefix ("ch2")
};
const std::vector<ChapterDef>& chapterDefs();
const ChapterDef* findChapter(const std::string& level);
const ChapterDef& chapterOf(const std::string& level);   // falls back to Chapter I
int loreCountForChapter(int chapter);

constexpr int kEssencePerPoint = 100;
constexpr int kMaxBladeLevel = 6;

struct SaveData {
    int version = 1;
    std::string level = "ch1_ashen_grove";
    std::string checkpoint;            // shrine spawn id; empty = level start
    float hp = 100, ki = 0;
    int gourds = 2, maxGourds = 2;
    int mon = 0, essence = 0, skillPoints = 0, shuriken = 6;
    int bladeLevel = 0, beads = 0;
    std::map<std::string, int> inventory;
    std::string blade = "kurosaki_wakizashi", garb = "kagenui_garb", mask, relic1, relic2;
    std::map<std::string, std::vector<std::string>> gems;   // blade id -> socketed gems
    std::set<std::string> skills;
    std::set<std::string> flags;        // collected ids, story beats, solved puzzles
    std::map<std::string, int> counters;
    float playTime = 0;
    int ngPlus = 0;
    bool valid = false;

    bool has(const std::string& flag) const { return flags.count(flag) > 0; }
    int count(const std::string& item) const { auto it = inventory.find(item); return it == inventory.end() ? 0 : it->second; }
    void add(const std::string& item, int n = 1) { inventory[item] += n; if (inventory[item] <= 0) inventory.erase(item); }
    int counter(const std::string& k) const { auto it = counters.find(k); return it == counters.end() ? 0 : it->second; }
    void bump(const std::string& k, int n = 1) { counters[k] += n; }
    int maxShuriken() const;
    std::vector<std::string> equippedIds() const;
};

Stats computeStats(const SaveData& s);
float maxHpOf(const SaveData& s);
bool canLearn(const SaveData& s, const SkillDef& d, std::string* why = nullptr);
bool canCraft(const SaveData& s, const RecipeDef& r, std::string* why = nullptr);
void applyCraft(SaveData& s, const RecipeDef& r);
SaveData newGame(int ngPlus = 0, const SaveData* carry = nullptr);

struct Profile {
    std::set<std::string> achievements;
    bool ngPlusUnlocked = false;
    float bestTime = 0;
    int chaptersCleared = 0;
    int bestScore = 0;
    std::string bestGrade;
    int bestTrialMs = 0;
    std::map<std::string, int> chapterBest;          // level id -> best score
    std::map<std::string, std::string> chapterGrade; // level id -> grade of that best
    std::map<std::string, float> chapterTime;        // level id -> fastest clear (s)
    std::set<std::string> endings;                   // "1".."3"
};

// Chapter scoring for the competitive layer: score, letter grade.
struct ChapterResult { int score = 0; std::string grade; };
ChapterResult scoreChapter(const SaveData& s);
// Remember where the counters stood when a chapter began, so each chapter is scored on its own.
void markChapterStart(SaveData& s);
// Point a save at the start of a chapter; clears that chapter's local progress so it can be replayed.
void beginChapter(SaveData& s, const ChapterDef& c, bool replay);

struct Settings {
    float master = 0.8f, music = 0.6f, sfx = 0.9f;
    float shake = 1.0f;
    bool reduceFlashes = false;
    float deflectAssist = 1.0f;    // multiplies the deflect window
    float gameSpeed = 1.0f;
    bool guardToggle = false;
    float subtitleScale = 1.0f;
    bool highContrast = false;
    bool fullscreen = false;
    bool vsync = true;
    bool rumble = true;
    bool hints = true;
    bool inkSplatter = true;       // off = petals instead of ink-blood
};

std::string serialize(const SaveData& s);
bool deserialize(const std::string& text, SaveData& s);
std::string serialize(const Profile& p);
bool deserialize(const std::string& text, Profile& p);
std::string serialize(const Settings& s);
bool deserialize(const std::string& text, Settings& s);

bool saveGame(const SaveData& s);
bool loadGame(SaveData& s);
bool hasSaveGame();
bool saveProfile(const Profile& p);
void loadProfile(Profile& p);
bool saveSettings(const Settings& s);
void loadSettings(Settings& s);

} // namespace sv
