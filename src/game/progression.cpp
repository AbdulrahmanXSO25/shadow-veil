#include "game/progression.h"
#include "core/util.h"
#include <sstream>
#include <algorithm>

namespace sv {

const char* slotName(ItemSlot s) {
    switch (s) {
    case ItemSlot::Blade: return "Blade";
    case ItemSlot::Garb: return "Garb";
    case ItemSlot::Mask: return "Mask";
    case ItemSlot::Relic: return "Token";
    case ItemSlot::Gem: return "Gem";
    case ItemSlot::Material: return "Material";
    case ItemSlot::Key: return "Key";
    }
    return "?";
}

Stats& Stats::operator+=(const Stats& o) {
    attack += o.attack; maxHp += o.maxHp; defense += o.defense; postureDmg += o.postureDmg;
    deflectWindow += o.deflectWindow; kiGain += o.kiGain; stealth += o.stealth; emberDmg += o.emberDmg;
    healAmt += o.healAmt; executeHeal += o.executeHeal; comboEmber += o.comboEmber; dashCooldown += o.dashCooldown;
    return *this;
}

static Stats S(std::initializer_list<std::pair<const char*, float>> kv) {
    Stats s;
    for (auto& [k, v] : kv) {
        std::string key = k;
        if (key == "attack") s.attack = v;
        else if (key == "maxHp") s.maxHp = v;
        else if (key == "defense") s.defense = v;
        else if (key == "posture") s.postureDmg = v;
        else if (key == "deflect") s.deflectWindow = v;
        else if (key == "ki") s.kiGain = v;
        else if (key == "stealth") s.stealth = v;
        else if (key == "ember") s.emberDmg = v;
        else if (key == "heal") s.healAmt = v;
        else if (key == "execHeal") s.executeHeal = v;
        else if (key == "comboEmber") s.comboEmber = v;
        else if (key == "dash") s.dashCooldown = v;
    }
    return s;
}

const std::vector<ItemDef>& itemDefs() {
    static const std::vector<ItemDef> defs = {
        {"kurosaki_wakizashi", "Kurosaki Wakizashi", "刃", ItemSlot::Blade, "A short Kagenui blade. Quick and honest.", S({}), 1,
         "Every child of the clan is given one at twelve. Rei's has never left her side."},
        {"hollow_moon", "Hollow Moon", "朧月", ItemSlot::Blade, "+6 attack, +10% posture damage. Two gem sockets.", S({{"attack", 6}, {"posture", 0.10f}}), 2,
         "Forged from star-iron that fell through a tear in the Veil. The steel is cold even in fire."},
        {"kagenui_garb", "Kagenui Garb", "忍", ItemSlot::Garb, "The indigo of the shadow-stitchers.", S({}), 0,
         "Dyed nine times in the river below Kurosaki."},
        {"ash_garb", "Ash-Dyed Garb", "灰", ItemSlot::Garb, "+10% defense. Enemies notice you 15% slower.", S({{"defense", 0.10f}, {"stealth", 0.15f}}), 0,
         "Cloth soaked in the ash of the burning village. It smells of home."},
        {"ronin_lamellar", "Ronin's Lamellar", "鎧", ItemSlot::Garb, "+25% defense, but enemies notice you 15% faster.", S({{"defense", 0.25f}, {"stealth", -0.15f}}), 0,
         "Iron scales laced with faded crimson cord. Someone mended it with great care."},
        {"fox_mask", "Fox Mask", "狐", ItemSlot::Mask, "Enemies notice you 35% slower.", S({{"stealth", 0.35f}}), 0,
         "Worn at the Lantern Fair by children playing at being foxes. This one was carved for an adult."},
        {"oni_mask", "Horned Half-Mask", "面", ItemSlot::Mask, "+4 attack, +10% posture damage, -10% defense.", S({{"attack", 4}, {"posture", 0.10f}, {"defense", -0.10f}}), 0,
         "Iron vanguard officers wear these so their men never see them afraid."},
        {"cracked_bell", "Cracked Bell", "鈴", ItemSlot::Relic, "Deflect window +25%.", S({{"deflect", 0.25f}}), 0,
         "It no longer rings true. Those who listen closely still hear the warning in it."},
        {"ember_talisman", "Ember Emblem", "炎", ItemSlot::Relic, "Unlocks Ember Art. Ember damage +10%.", S({{"ember", 0.10f}}), 0,
         "Fuki's grandmother wove it from a thread of the Veil's edge, where it burns."},
        {"iron_vow", "Iron Vow", "鉄", ItemSlot::Relic, "Executions restore 15 vitality.", S({{"execHeal", 15}}), 0,
         "Tetsu's oath-plate. On the back, scratched by hand: 'Let me die facing someone worthy.'"},
        {"gourd_charm", "Gourd Strap", "瓢", ItemSlot::Relic, "Each gourd heals 15 more.", S({{"heal", 15}}), 0,
         "A tiny lacquered gourd. Camp keepers tie them to the wrists of travellers."},
        {"coal_ruby", "Coal Ruby", "紅", ItemSlot::Gem, "Every third strike of a combo bursts with ember.", S({{"comboEmber", 1}}), 0,
         "A stone that remembers being fire."},
        {"river_jade", "River Jade", "玉", ItemSlot::Gem, "Spark gain +25%.", S({{"ki", 0.25f}}), 0,
         "Smoothed by the river for a thousand years. It is never in a hurry."},
        {"moonstone", "Moonstone", "月", ItemSlot::Gem, "Posture damage +15%.", S({{"posture", 0.15f}}), 0,
         "Pale and heavy. Struck steel rings longer near it."},
        {"iron_sand", "Iron Sand", "砂", ItemSlot::Material, "Smithing material. Tempers blades and forges shuriken.", S({}), 0, ""},
        {"spirit_silk", "Moon Silk", "絹", ItemSlot::Material, "Thread spun near the Veil. Used for straps and gourds.", S({}), 0, ""},
        {"ember_stone", "Ember Stone", "石", ItemSlot::Material, "Warm to the touch. Used to forge ember gems.", S({}), 0, ""},
    };
    return defs;
}

const ItemDef* findItem(const std::string& id) {
    for (auto& d : itemDefs()) if (d.id == id) return &d;
    return nullptr;
}

const std::vector<SkillDef>& skillDefs() {
    static const std::vector<SkillDef> defs = {
        {"blade_fourth", "Fourth Stroke", "四", "Adds a rising fourth cut to the ground combo.", 0, 0},
        {"blade_iron", "Iron Wrist", "鉄", "+20% posture damage with every strike.", 0, 1},
        {"blade_blossom", "Death Blossom", "花", "Executions restore 20 vitality.", 0, 2},
        {"veil_second", "Second Step", "弐", "Veil-Step twice before landing.", 1, 0},
        {"veil_thread", "Long Thread", "糸", "Veil-Thread reaches 30% farther.", 1, 1},
        {"veil_well", "Ember Wellspring", "炎", "Ember Art costs 10 less Spark and burns 20% hotter.", 1, 2},
        {"shadow_soft", "Soft Step", "忍", "Move faster while crouched. Enemies notice you 25% slower.", 2, 0},
        {"shadow_pouch", "Deep Pouch", "参", "Carry 6 more shuriken.", 2, 1},
        {"shadow_drop", "Shadow Harvest", "影", "Stealth kills restore 30 Sparks and a little vitality.", 2, 2},
        {"aura_rising", "Rising Gale", "風", "Air strikes loose wind blades too.", 3, 0},
        {"aura_eye", "Eye of the Storm", "嵐", "A perfect deflect bursts outward, battering the posture of every foe nearby.", 3, 1},
        {"aura_tail", "Tailwind", "追", "Veil-Step through an attack: time slows around you and you gain 15 Sparks.", 3, 2},
    };
    return defs;
}
const char* branchName(int b) { static const char* n[] = {"Blade", "Veil", "Shadow", "Wind"}; return n[std::clamp(b, 0, 3)]; }

const std::vector<RecipeDef>& recipeDefs() {
    static const std::vector<RecipeDef> defs = {
        {"temper", "Temper Blade", "+4 attack to every blade. Up to three times.", {{"iron_sand", 2}}, 120, "@temper"},
        {"coal_ruby", "Forge Coal Ruby", "Ember bursts on every third strike.", {{"ember_stone", 2}, {"iron_sand", 1}}, 60, "coal_ruby"},
        {"moonstone", "Polish Moonstone", "+15% posture damage gem.", {{"spirit_silk", 1}, {"iron_sand", 1}}, 150, "moonstone"},
        {"gourd", "Glaze a Gourd", "+1 healing gourd (max 5).", {{"spirit_silk", 1}, {"ember_stone", 1}}, 200, "@gourd"},
        {"gourd_charm", "Tie Gourd Strap", "Token: each gourd heals 15 more.", {{"spirit_silk", 2}}, 100, "gourd_charm"},
        {"shuriken", "Forge Shuriken", "+5 shuriken.", {{"iron_sand", 1}}, 20, "@shuriken"},
    };
    return defs;
}

const std::vector<AchievementDef>& achievementDefs() {
    static const std::vector<AchievementDef> defs = {
        {"stealth_kill", "Silent Petal", "Perform a stealth kill."},
        {"first_deflect", "Steel Answers Steel", "Perfectly deflect an attack."},
        {"first_execution", "Broken Will", "Execute an enemy whose posture you broke."},
        {"first_grapple", "Thread of the Veil", "Swing on the Veil-Thread."},
        {"bell_puzzle", "The Bell Remembers", "Raise the Bell Gate."},
        {"bridge_puzzle", "Counterweight", "Drop the ravine bridge."},
        {"all_scrolls_c1", "Ink and Memory", "Find every Ink Scroll in the Ashen Grove."},
        {"hollow_moon", "The Hollow Moon", "Find the hidden blade.", true},
        {"fox_mask", "Fox at the Fair", "Find the Fox Mask.", true},
        {"boss_tetsugen", "Iron Falls", "Defeat Tetsu, the Iron Ronin."},
        {"boss_nodamage", "Untouched", "Defeat Tetsu without taking damage."},
        {"grove_unseen", "Unseen", "Cross the Bamboo Grove without being spotted."},
        {"deflect_50", "Fifty Answers", "Deflect 50 attacks."},
        {"speedrun_c1", "Swift Shadow", "Finish Chapter I in under 15 minutes."},
        {"craft", "Smith's Apprentice", "Craft at a camp forge."},
        {"skills_5", "Way of the Shadow", "Learn five skills."},
        {"ngplus", "Another Road", "Begin New Game+."},
        {"first_death", "Return to Ash", "Die. It happens to everyone.", true},
        {"gale", "Eye of the Wind", "Complete the Wind Trial and awaken the Gale Stance."},
        {"rank_ss", "Kage", "Reach the SS Kage Rank."},
        {"grade_s", "Shadow Master", "Finish Chapter I with an S grade or better."},
    };
    return defs;
}

const std::vector<LoreDef>& loreDefs() {
    static const std::vector<LoreDef> defs = {
        {"scroll_founding", "The First Stitch",
         "When the raiders outnumbered the clan, the first Kagenui climbed to the Old Gate with a needle of bone "
         "and a spool of thread no one would name. They sewed for forty nights. On the forty-first, the pass held. "
         "We do not speak of what the thread was spun from. We only keep it mended."},
        {"scroll_fuki", "The Camp Keeper's Ledger",
         "Year of the Ox: three bales of rice, two lanterns. Year of the Tiger: the elders went up the mountain and came back four. "
         "Year of the Hare: the Veil thinned again. Odaka asked me how much it would cost to mend it forever. I did not answer. "
         "He already knew."},
        {"scroll_orders", "Orders of the Iron Vanguard",
         "Burn the village to the river. Take the Anchor from the Old Gate. Do NOT harm the old man in the straw cloak. "
         "He is expected, and he will open the way. Any soldier who disobeys will be sent beyond the Veil. — Lord Hakuro's hand"},
        {"scroll_letter", "A Letter, Unsent",
         "Sayo. The girl has your stillness now. She sits at the cliff camp for hours and the moon seems to lean closer to listen. "
         "She asks about you and I tell her the truth, mostly. Soon I will tell her the rest. Soon I will bring you home. — O."},
        {"scroll_ronin", "A Ronin's Debt",
         "I served a lord who fled. I served a lord who lied. Now I serve a lord who wants to break the Veil. "
         "I do not care for his war. I care only that somewhere on this burning mountain there is a blade worth losing to."},
    };
    return defs;
}
const LoreDef* findLore(const std::string& id) {
    for (auto& d : loreDefs()) if (d.id == id) return &d;
    return nullptr;
}

int SaveData::maxShuriken() const { return 12 + (skills.count("shadow_pouch") ? 6 : 0); }

std::vector<std::string> SaveData::equippedIds() const {
    std::vector<std::string> v;
    for (const auto* id : {&blade, &garb, &mask, &relic1, &relic2}) if (!id->empty()) v.push_back(*id);
    auto it = gems.find(blade);
    if (it != gems.end()) for (auto& g : it->second) v.push_back(g);
    return v;
}

Stats computeStats(const SaveData& s) {
    Stats st;
    st.attack = 12 + s.bladeLevel * 4;
    for (auto& id : s.equippedIds())
        if (auto* d = findItem(id)) st += d->mods;
    if (s.skills.count("blade_iron")) st.postureDmg += 0.20f;
    if (s.skills.count("blade_blossom")) st.executeHeal += 20;
    if (s.skills.count("shadow_soft")) st.stealth += 0.25f;
    if (s.skills.count("veil_well")) st.emberDmg += 0.20f;
    st.maxHp += (s.beads / 4) * 15.f;
    st.defense = std::clamp(st.defense, -0.5f, 0.6f);
    st.stealth = std::clamp(st.stealth, -0.5f, 0.85f);
    return st;
}

float maxHpOf(const SaveData& s) { return 100.f + computeStats(s).maxHp; }

bool canLearn(const SaveData& s, const SkillDef& d, std::string* why) {
    if (s.skills.count(d.id)) { if (why) *why = "Already learned."; return false; }
    if (d.branch == 3 && !s.has("gale")) { if (why) *why = "Awaken the Gale Stance at a wind beacon first."; return false; }
    if (d.tier > 0) {
        for (auto& o : skillDefs())
            if (o.branch == d.branch && o.tier == d.tier - 1 && !s.skills.count(o.id)) {
                if (why) *why = "Learn " + o.name + " first.";
                return false;
            }
    }
    if (s.skillPoints < 1) { if (why) *why = "Requires a skill point."; return false; }
    return true;
}

bool canCraft(const SaveData& s, const RecipeDef& r, std::string* why) {
    if (r.output == "@temper" && s.bladeLevel >= kMaxBladeLevel) { if (why) *why = "Your blades are fully tempered."; return false; }
    if (r.output == "@gourd" && s.maxGourds >= 5) { if (why) *why = "You cannot carry more gourds."; return false; }
    if (r.output == "@shuriken" && s.shuriken >= s.maxShuriken()) { if (why) *why = "Your pouch is full."; return false; }
    if (r.output[0] != '@' && s.count(r.output) > 0 && findItem(r.output) && findItem(r.output)->slot != ItemSlot::Gem) {
        if (why) *why = "Already owned.";
        return false;
    }
    int mon = r.output == "@temper" ? r.mon * (s.bladeLevel + 1) : r.mon;
    if (s.mon < mon) { if (why) *why = strfmt("Requires %d mon.", mon); return false; }
    for (auto& [id, n] : r.inputs)
        if (s.count(id) < n) { if (why) *why = "Missing " + (findItem(id) ? findItem(id)->name : id) + "."; return false; }
    return true;
}

void applyCraft(SaveData& s, const RecipeDef& r) {
    int mon = r.output == "@temper" ? r.mon * (s.bladeLevel + 1) : r.mon;
    s.mon -= mon;
    for (auto& [id, n] : r.inputs) s.add(id, -n);
    if (r.output == "@temper") s.bladeLevel++;
    else if (r.output == "@gourd") { s.maxGourds++; s.gourds = s.maxGourds; }
    else if (r.output == "@shuriken") s.shuriken = std::min(s.maxShuriken(), s.shuriken + 5);
    else s.add(r.output, 1);
    s.bump("crafted");
}

SaveData newGame(int ngPlus, const SaveData* carry) {
    SaveData s;
    s.valid = true;
    s.ngPlus = ngPlus;
    s.inventory["kurosaki_wakizashi"] = 1;
    s.inventory["kagenui_garb"] = 1;
    if (carry) {
        // New Game+: keep gear, skills, crafting progress; story and collectibles reset.
        s.inventory = carry->inventory;
        s.blade = carry->blade; s.garb = carry->garb; s.mask = carry->mask; s.relic1 = carry->relic1; s.relic2 = carry->relic2;
        s.gems = carry->gems; s.skills = carry->skills; s.skillPoints = carry->skillPoints;
        s.bladeLevel = carry->bladeLevel; s.maxGourds = carry->maxGourds; s.beads = carry->beads;
        s.counters = carry->counters;
        s.mon = carry->mon;
        // The emblem is a story item; it is re-earned from Fuki each run.
        s.inventory.erase("ember_talisman");
        if (s.relic1 == "ember_talisman") s.relic1.clear();
        if (s.relic2 == "ember_talisman") s.relic2.clear();
    }
    s.gourds = s.maxGourds;
    s.hp = maxHpOf(s);
    s.shuriken = 6;
    return s;
}

ChapterResult scoreChapter(const SaveData& s) {
    ChapterResult r;
    float timeBonus = std::max(0.f, 1500.f - s.playTime) * 6.f;   // under 25 minutes earns time points
    r.score = s.counter("kills") * 100 + s.counter("executions") * 250 + s.counter("stealthKills") * 220 + s.counter("deflects") * 60 +
              s.counter("maxRank") * 500 + (int)timeBonus - s.counter("deaths") * 400 + (s.has("gale") ? 1500 : 0) + s.ngPlus * 2000;
    r.score = std::max(0, r.score);
    r.grade = r.score >= 16000 ? "S+" : r.score >= 12000 ? "S" : r.score >= 8500 ? "A" : r.score >= 5000 ? "B" : "C";
    return r;
}

// ------------------------------------------------------------------ serialization

static std::string joinSet(const std::set<std::string>& s) {
    std::string out;
    for (auto& v : s) { if (!out.empty()) out += ','; out += v; }
    return out;
}
static std::set<std::string> splitSet(const std::string& v) {
    std::set<std::string> s;
    if (v.empty()) return s;
    for (auto& x : split(v, ',')) if (!x.empty()) s.insert(x);
    return s;
}

static std::map<std::string, std::string> parseKV(const std::string& text) {
    std::map<std::string, std::string> kv;
    for (auto& line : split(text, '\n')) {
        auto l = trim(line);
        if (l.empty() || l[0] == '#') continue;
        auto eq = l.find('=');
        if (eq == std::string::npos) continue;
        kv[trim(l.substr(0, eq))] = trim(l.substr(eq + 1));
    }
    return kv;
}

std::string serialize(const SaveData& s) {
    std::ostringstream o;
    o << "# Shadow Veil save\nversion=" << s.version << "\nlevel=" << s.level << "\ncheckpoint=" << s.checkpoint
      << "\nhp=" << s.hp << "\nki=" << s.ki << "\ngourds=" << s.gourds << "\nmaxGourds=" << s.maxGourds
      << "\nmon=" << s.mon << "\nessence=" << s.essence << "\nskillPoints=" << s.skillPoints << "\nshuriken=" << s.shuriken
      << "\nbladeLevel=" << s.bladeLevel << "\nbeads=" << s.beads << "\nblade=" << s.blade << "\ngarb=" << s.garb
      << "\nmask=" << s.mask << "\nrelic1=" << s.relic1 << "\nrelic2=" << s.relic2
      << "\nskills=" << joinSet(s.skills) << "\nflags=" << joinSet(s.flags) << "\nplayTime=" << s.playTime
      << "\nngPlus=" << s.ngPlus << "\n";
    for (auto& [k, v] : s.inventory) o << "inv." << k << "=" << v << "\n";
    for (auto& [k, v] : s.counters) o << "ctr." << k << "=" << v << "\n";
    for (auto& [k, v] : s.gems) {
        std::string g;
        for (auto& x : v) { if (!g.empty()) g += ','; g += x; }
        o << "gems." << k << "=" << g << "\n";
    }
    return o.str();
}

bool deserialize(const std::string& text, SaveData& s) {
    auto kv = parseKV(text);
    if (!kv.count("version")) return false;
    s = SaveData();
    auto gi = [&](const char* k, int& v) { if (kv.count(k)) v = std::atoi(kv[k].c_str()); };
    auto gf = [&](const char* k, float& v) { if (kv.count(k)) v = (float)std::atof(kv[k].c_str()); };
    auto gs = [&](const char* k, std::string& v) { if (kv.count(k)) v = kv[k]; };
    gi("version", s.version); gs("level", s.level); gs("checkpoint", s.checkpoint);
    gf("hp", s.hp); gf("ki", s.ki); gi("gourds", s.gourds); gi("maxGourds", s.maxGourds);
    gi("mon", s.mon); gi("essence", s.essence); gi("skillPoints", s.skillPoints); gi("shuriken", s.shuriken);
    gi("bladeLevel", s.bladeLevel); gi("beads", s.beads);
    gs("blade", s.blade); gs("garb", s.garb); gs("mask", s.mask); gs("relic1", s.relic1); gs("relic2", s.relic2);
    s.skills = splitSet(kv["skills"]); s.flags = splitSet(kv["flags"]);
    gf("playTime", s.playTime); gi("ngPlus", s.ngPlus);
    for (auto& [k, v] : kv) {
        if (startsWith(k, "inv.")) s.inventory[k.substr(4)] = std::atoi(v.c_str());
        else if (startsWith(k, "ctr.")) s.counters[k.substr(4)] = std::atoi(v.c_str());
        else if (startsWith(k, "gems.")) { auto set = split(v, ','); std::vector<std::string> g; for (auto& x : set) if (!x.empty()) g.push_back(x); s.gems[k.substr(5)] = g; }
    }
    s.valid = true;
    return true;
}

std::string serialize(const Profile& p) {
    std::ostringstream o;
    o << "achievements=" << joinSet(p.achievements) << "\nngPlusUnlocked=" << (p.ngPlusUnlocked ? 1 : 0)
      << "\nbestTime=" << p.bestTime << "\nchaptersCleared=" << p.chaptersCleared
      << "\nbestScore=" << p.bestScore << "\nbestGrade=" << p.bestGrade << "\nbestTrialMs=" << p.bestTrialMs << "\n";
    return o.str();
}
bool deserialize(const std::string& text, Profile& p) {
    auto kv = parseKV(text);
    p = Profile();
    p.achievements = splitSet(kv["achievements"]);
    p.ngPlusUnlocked = kv["ngPlusUnlocked"] == "1";
    p.bestTime = (float)std::atof(kv["bestTime"].c_str());
    p.chaptersCleared = std::atoi(kv["chaptersCleared"].c_str());
    p.bestScore = std::atoi(kv["bestScore"].c_str());
    p.bestGrade = kv["bestGrade"];
    p.bestTrialMs = std::atoi(kv["bestTrialMs"].c_str());
    return true;
}

std::string serialize(const Settings& s) {
    std::ostringstream o;
    o << "master=" << s.master << "\nmusic=" << s.music << "\nsfx=" << s.sfx << "\nshake=" << s.shake
      << "\nreduceFlashes=" << s.reduceFlashes << "\ndeflectAssist=" << s.deflectAssist << "\ngameSpeed=" << s.gameSpeed
      << "\nguardToggle=" << s.guardToggle << "\nsubtitleScale=" << s.subtitleScale << "\nhighContrast=" << s.highContrast
      << "\nfullscreen=" << s.fullscreen << "\nvsync=" << s.vsync << "\nrumble=" << s.rumble << "\nhints=" << s.hints
      << "\ninkSplatter=" << s.inkSplatter << "\n";
    return o.str();
}
bool deserialize(const std::string& text, Settings& s) {
    auto kv = parseKV(text);
    auto gf = [&](const char* k, float& v, float lo, float hi) { if (kv.count(k)) v = std::clamp((float)std::atof(kv[k].c_str()), lo, hi); };
    auto gb = [&](const char* k, bool& v) { if (kv.count(k)) v = kv[k] == "1"; };
    gf("master", s.master, 0, 1); gf("music", s.music, 0, 1); gf("sfx", s.sfx, 0, 1); gf("shake", s.shake, 0, 1);
    gb("reduceFlashes", s.reduceFlashes); gf("deflectAssist", s.deflectAssist, 1, 2); gf("gameSpeed", s.gameSpeed, 0.6f, 1);
    gb("guardToggle", s.guardToggle); gf("subtitleScale", s.subtitleScale, 0.8f, 1.6f); gb("highContrast", s.highContrast);
    gb("fullscreen", s.fullscreen); gb("vsync", s.vsync); gb("rumble", s.rumble); gb("hints", s.hints); gb("inkSplatter", s.inkSplatter);
    return true;
}

static std::string savePath() { return userDir() + "/save1.dat"; }
static std::string profilePath() { return userDir() + "/profile.dat"; }
static std::string settingsPath() { return userDir() + "/settings.cfg"; }

bool saveGame(const SaveData& s) { return writeFileAtomic(savePath(), serialize(s)); }
bool loadGame(SaveData& s) { auto t = readFile(savePath()); return t && deserialize(*t, s); }
bool hasSaveGame() { SaveData s; return loadGame(s); }
bool saveProfile(const Profile& p) { return writeFileAtomic(profilePath(), serialize(p)); }
void loadProfile(Profile& p) { if (auto t = readFile(profilePath())) deserialize(*t, p); }
bool saveSettings(const Settings& s) { return writeFileAtomic(settingsPath(), serialize(s)); }
void loadSettings(Settings& s) { if (auto t = readFile(settingsPath())) deserialize(*t, s); }

} // namespace sv
