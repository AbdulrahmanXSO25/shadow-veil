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
         "Worn at the Harvest Fair by children playing at being foxes. This one was carved for an adult."},
        {"oni_mask", "Iron Half-Mask", "面", ItemSlot::Mask, "+4 attack, +10% posture damage, -10% defense.", S({{"attack", 4}, {"posture", 0.10f}, {"defense", -0.10f}}), 0,
         "Iron vanguard officers wear these so their men never see them afraid."},
        {"cracked_bell", "Cracked Tsuba", "鍔", ItemSlot::Relic, "Deflect window +25%.", S({{"deflect", 0.25f}}), 0,
         "A sword guard split by a single perfect parry. Its owner kept it to remember what patience is worth."},
        {"ember_talisman", "Ember Emblem", "炎", ItemSlot::Relic, "Unlocks Ember Art. Ember damage +10%.", S({{"ember", 0.10f}}), 0,
         "Fuki's grandmother wove it from a thread of the Veil's edge, where it burns."},
        {"iron_vow", "Iron Vow", "鉄", ItemSlot::Relic, "Executions restore 15 vitality.", S({{"execHeal", 15}}), 0,
         "Tetsu's oath-plate. On the back, scratched by hand: 'Let me meet one worthy blade before the end.'"},
        {"gourd_charm", "Gourd Strap", "瓢", ItemSlot::Relic, "Each gourd heals 15 more.", S({{"heal", 15}}), 0,
         "A tiny lacquered gourd. Camp keepers tie them to the wrists of travellers."},
        {"coal_ruby", "Coal Ruby", "紅", ItemSlot::Gem, "Every third strike of a combo bursts with ember.", S({{"comboEmber", 1}}), 0,
         "A stone that remembers being fire."},
        {"river_jade", "River Jade", "玉", ItemSlot::Gem, "Spark gain +25%.", S({{"ki", 0.25f}}), 0,
         "Smoothed by the river for a thousand years. It is never in a hurry."},
        {"moonstone", "Moonstone", "月", ItemSlot::Gem, "Posture damage +15%.", S({{"posture", 0.15f}}), 0,
         "Pale and heavy. Struck steel rings longer near it."},
        // ---- Chapter II: the drowned terraces
        {"tide_garb", "Tide-Worn Garb", "潮", ItemSlot::Garb, "+15% defense, Spark gain +10%.", S({{"defense", 0.15f}, {"ki", 0.10f}}), 0,
         "A ferryman's coat, waxed against the rain. The hem is heavy with river stones sewn in for ballast."},
        {"rain_pearl", "Rain Pearl", "珠", ItemSlot::Gem, "Deflect window +12%, each gourd heals 10 more.", S({{"deflect", 0.12f}, {"heal", 10}}), 0,
         "Paddy farmers find these in the mud after the longest rains. They are said to keep the eyes calm."},
        {"warden_chain", "Warden's Chain", "鎖", ItemSlot::Relic, "Posture damage +20%.", S({{"posture", 0.20f}}), 0,
         "Funaki's chain, surrendered without a word. Every link is stamped with the name of a village he flooded."},
        // ---- Chapter III: the maple pass
        {"maple_blade", "Crimson Maple", "楓", ItemSlot::Blade, "+10 attack, +5% posture damage. Two gem sockets.", S({{"attack", 10}, {"posture", 0.05f}}), 2,
         "A duelist's blade with a red-lacquered scabbard. The ridge company say it has never been drawn second."},
        {"crow_mask", "Crow Mask", "烏", ItemSlot::Mask, "+3 attack, enemies notice you 25% slower.", S({{"attack", 3}, {"stealth", 0.25f}}), 0,
         "Karasu gave it freely. 'Wear it on the ridge and no crow will raise the alarm. They will think you are family.'"},
        {"thunder_agate", "Thunder Agate", "雷", ItemSlot::Gem, "Spark gain +20%, posture damage +10%.", S({{"ki", 0.20f}, {"posture", 0.10f}}), 0,
         "Split by lightning on the high pass. Hold it to your ear and it crackles faintly."},
        // ---- Chapter IV: Castle Hakuro
        {"wolf_mantle", "White Wolf Mantle", "狼", ItemSlot::Garb, "+20% defense, enemies notice you 20% slower.", S({{"defense", 0.20f}, {"stealth", 0.20f}}), 0,
         "Pale fur over grey silk, warm enough to sleep in the snow. Made for a lord, cut down for a girl."},
        {"frost_sapphire", "Frost Sapphire", "氷", ItemSlot::Gem, "Deflect window +15%, posture damage +10%.", S({{"deflect", 0.15f}, {"posture", 0.10f}}), 0,
         "Mined beneath Castle Hakuro. It never warms, not even in the hand."},
        {"hakuro_fang", "White Fang", "牙", ItemSlot::Relic, "+6 attack, executions restore 10 vitality.", S({{"attack", 6}, {"execHeal", 10}}), 0,
         "The fang of the first wolf Lord Hakuro raised. He gave it to you the way a soldier gives up his sword."},
        // ---- Chapter V: the Mirror Province
        {"mirror_mask", "Mirror Mask", "鏡", ItemSlot::Mask, "Deflect window +15%, enemies notice you 30% slower.", S({{"deflect", 0.15f}, {"stealth", 0.30f}}), 0,
         "A face-plate of polished white lacquer. The Hollow's people wear them against the glare of the Bright."},
        {"hollow_thread", "Hollow Thread", "虚", ItemSlot::Relic, "Veil-Step recovers 0.08 s sooner, Spark gain +15%.", S({{"dash", 0.08f}, {"ki", 0.15f}}), 0,
         "A thread drawn from the inside of the Veil. It is lighter than it should be."},
        // ---- Chapter VI: the heart
        {"weavers_needle", "Weaver's Needle", "針", ItemSlot::Blade, "+16 attack, +20% posture damage. Three gem sockets.", S({{"attack", 16}, {"posture", 0.20f}}), 3,
         "The needle the first Kagenui sewed the pass shut with, worn down to the length of a blade by six hundred years of mending."},
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
        {"blade_tide", "Last Light", "灯", "+10 attack while your vitality is below 40%.", 0, 3},
        {"veil_mantle", "Veil Mantle", "幕", "Veil-Step recovers 0.1 s sooner and its protection lasts a moment longer.", 1, 3},
        {"shadow_breath", "Quiet Breath", "息", "The Shadow Stance cloaks you twice as fast. Stealth kills return 2 shuriken.", 2, 3},
        {"aura_storm", "Gathering Storm", "颯", "At Kage Rank A or higher, every cut after the first looses a wind blade.", 3, 3},
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
        {"bell_puzzle", "Counterweight Catch", "Raise the Weight Gate."},
        {"bridge_puzzle", "Bridge the Gap", "Drop the ravine bridge."},
        {"all_scrolls_c1", "Ink and Memory", "Find every Ink Scroll in the Ashen Grove."},
        {"hollow_moon", "The Hollow Moon", "Find the hidden blade.", true},
        {"fox_mask", "Fox at the Fair", "Find the Fox Mask.", true},
        {"boss_tetsugen", "Iron Falls", "Defeat Tetsu, the Iron Ronin."},
        {"boss_nodamage", "Untouched", "Defeat any champion without taking damage."},
        {"grove_unseen", "Unseen", "Cross the Bamboo Grove without being spotted."},
        {"deflect_50", "Fifty Answers", "Deflect 50 attacks."},
        {"speedrun_c1", "Swift Shadow", "Finish Chapter I in under 15 minutes."},
        {"craft", "Smith's Apprentice", "Craft at a camp forge."},
        {"skills_5", "Way of the Shadow", "Learn five skills."},
        {"ngplus", "Another Road", "Begin New Game+."},
        {"first_death", "Return to Ash", "Die. It happens to everyone.", true},
        {"gale", "Eye of the Wind", "Complete the Wind Trial and awaken the Gale Stance."},
        {"rank_ss", "Kage", "Reach the SS Kage Rank."},
        {"grade_s", "Shadow Master", "Finish any chapter with an S grade or better."},
        {"sluice_puzzle", "Let It Flow", "Open a sluice and drain the drowned path."},
        {"stillwater", "Still Water", "Complete the Rain Trial and awaken the Stillwater Stance."},
        {"boss_warden", "The Tide Turns", "Defeat Funaki, the Tide Warden."},
        {"all_scrolls_c2", "Ink in the Rain", "Find every Ink Scroll on the Drowned Terrace."},
        {"thunder", "Storm Bearer", "Complete the Storm Trial and awaken the Thunder Stance."},
        {"thunder_three", "Three Strikes", "Strike three foes with one thunder-call."},
        {"boss_crowking", "Crown of Feathers", "Defeat Karasu, the Crow King."},
        {"all_scrolls_c3", "Red Leaves", "Find every Ink Scroll on the Crimson Maple Pass."},
        {"douse_beacons", "Lights Out", "Douse six fire-watch beacons with shuriken."},
        {"castle_unseen", "Ghost of the Castle", "Cross the outer bailey of Castle Hakuro without being spotted."},
        {"shadow", "Shadow Kept", "Complete the Night Trial and awaken the Shadow Stance."},
        {"boss_hakuro", "The Wolf Kneels", "Defeat Lord Hakuro, the White Wolf."},
        {"all_scrolls_c4", "Letters in the Snow", "Find every Ink Scroll in Castle Hakuro."},
        {"mirror", "Two Faces", "Complete the Mirror Trial and awaken the Mirror Stance."},
        {"mirror_50", "Between Pages", "Cross between the Bright and the Hollow 50 times."},
        {"boss_kagami", "Reflection", "Defeat Kagami of the Hollow."},
        {"all_scrolls_c5", "The Other Side", "Find every Ink Scroll in the Mirror Province."},
        {"outrun", "Ahead of the Unravelling", "Outrun the Unravelling without being caught."},
        {"boss_odaka", "Master and Student", "Defeat Odaka Sen."},
        {"all_scrolls_c6", "The Last Stitches", "Find every Ink Scroll in the Heart of the Veil."},
        {"all_scrolls", "The Whole Cloth", "Find all 25 Ink Scrolls."},
        {"ending_1", "Kept", "Reweave the Veil as it was."},
        {"ending_2", "Unbound", "Let the Veil fall."},
        {"ending_3", "The Second Way", "Restitch the Veil with a gate in it.", true},
        {"all_chapters", "The End of the Thread", "Finish the story."},
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
         "Twelfth year of my watch: three bales of rice, two lanterns. Thirteenth year: the elders went up the mountain and came back four. "
         "Fourteenth year: the Veil thinned again. Odaka asked me how much it would cost to mend it forever. I did not answer. "
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
        // ---- Chapter II
        {"scroll_sluice", "The Sluice-Keeper's Tally", "Third day of rain. Closed the upper sluice on Lord Hakuro's order. Water to the eaves in Minase. "
         "Fourth day: closed the middle sluice. They are on the roofs now. Fifth day: the captain says a flooded valley cannot hide rebels. "
         "I have kept these gates for thirty years. I have never once closed them on people.", 2},
        {"scroll_ferry", "Ferryman's Song (written down)", "Row low, row slow, the rice is under water; / row low, row slow, mind the farmer's daughter; / "
         "the lord wants the valley and the river wants the sea, / and nobody, nobody, asks the water what it wants to be.", 2},
        {"scroll_minase", "Minase Village Register", "Households: 41. Rice fields: 160 terraces. Sent to Castle Hakuro as labour this year: 22 hands. "
         "Returned: none. Note in the margin, in a different brush: they are building something on the far side of the pass.", 2},
        {"scroll_hana", "Hana's Note", "Rei — if you find this, I'm fine. Grandmother is safe at the camp. Soldiers took everyone who can carry stone "
         "up to the castle. I'm going with them so the little ones aren't alone. Don't do anything brave. (Do something brave.) — H.", 2},
        // ---- Chapter III
        {"scroll_crows", "Articles of the Ridge Company", "One: a crow does not abandon a crow. Two: we take coin, not lives, unless coin is refused. "
         "Three: we fight for Hakuro because Hakuro swore the Veil would open. Four: when it opens, every family goes home first. Then we are paid.", 3},
        {"scroll_exile", "The Exile Roll", "By order of the six houses, the following are sent beyond the Veil for the crime of rebellion: "
         "the Karasu, the Kagami, the river-folk of Shiose… The list runs on for two hundred names. "
         "It is dated six hundred years ago. The Veil was not raised against raiders. It was raised against them.", 3},
        {"scroll_duelist", "A Duelist's Rule", "Never draw first. Never draw second. Draw at the moment your opponent decides to. "
         "If you cannot see that moment, go home and learn to.", 3},
        {"scroll_maple", "Pass Marker", "Carved into a maple at the summit: 'Odaka passed here. He did not stop to rest. He looked back once.' "
         "Below, cut much deeper: 'So did I. — K.'", 3},
        // ---- Chapter IV
        {"scroll_hakuro", "Lord Hakuro's Petition", "To the six houses: for the third year the harvest has failed in my valley while the pass stands closed. "
         "Open the gate. Let my people trade with the far side. You have refused me with silence. I will not ask a fourth time.", 4},
        {"scroll_kennel", "Kennel Master's Book", "Yuki: lame in the left forepaw, keep from the stairs. Kaze: will not stop howling at the north wall. "
         "The lord came down tonight and sat with them an hour without speaking. He does that before he does something terrible.", 4},
        {"scroll_mason", "A Mason's Count", "Stones laid in the north wall: 9,000. Stones taken from the villages: 9,000. "
         "We are building the castle out of our own homes. The girl from Kurosaki counts with us so we don't lose heart.", 4},
        {"scroll_odaka_pact", "The Pact", "Hakuro supplies soldiers and the burning of Kurosaki, so the Veil's keepers are scattered. "
         "Odaka supplies the night the Veil runs thinnest and the way to its heart. What lies beyond belongs to whoever reaches it. "
         "Neither will harm the girl Rei. — sealed by both", 4},
        // ---- Chapter V
        {"scroll_mirror", "On the Two Lands", "The Bright and the Hollow lie over one another like two pages pressed together. "
         "Step hard enough and you pass from one to the other. The stitchers called it a Veil. We call it the ceiling.", 5},
        {"scroll_kagami", "Kagami's Watch-Log", "Six hundred years on this side. We farm, we marry, we bury our dead in good ground. "
         "We wear masks against the light of the other side, which leaks in where the weave is thin. "
         "Lately it leaks everywhere. The weave is failing, and the Weaver is tired.", 5},
        {"scroll_weaver", "The Weaver's Duty", "A Veil this large cannot hold itself. Someone must sit at its heart and keep the tension true. "
         "The clan sends a volunteer every generation. They do not come back. That is the part we do not write down.", 5},
        {"scroll_sayo", "Sayo's Thread", "Brother. Do not come for me. If you pull me out, the weave falls on everyone at once, theirs and ours. "
         "Tell Rei I chose this. Tell her I hear her at the cliff camp sometimes, when the wind is right. — S.", 5},
        // ---- Chapter VI
        {"scroll_loom", "The Loom's Plan", "Drawn by the first stitchers: a single Veil, tensioned from one point, held by one keeper. "
         "In the margin, much later, a sketch of something else: a Veil with a gate in it, tensioned from both sides at once.", 6},
        {"scroll_sentinels", "Of the Thread Sentinels", "When no volunteer could be found, the stitchers wove guardians to keep the heart instead. "
         "They are only cloth and wire. They do not hate you. They will not stop.", 6},
        {"scroll_odaka", "Odaka's Last Entry", "Fifteen years. I raised her daughter. I taught her the blade. "
         "Tonight I pull the thread, and if the whole cloth comes apart with it, then at least it will not be my sister holding it alone.", 6},
        {"scroll_restitch", "The Second Way", "Two keepers, one on each side, and a gate between them that opens from either hand. "
         "No one bound. No one exiled. It needs the whole of the old knowledge to weave. Every scroll, every stitch. "
         "If you are reading this, you are close.", 6},
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
    if (s.skills.count("veil_mantle")) st.dashCooldown += 0.10f;
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
    markChapterStart(s);
    return s;
}

const std::vector<ChapterDef>& chapterDefs() {
    static const std::vector<ChapterDef> v = {
        {1, "ch1_ashen_grove", "data/levels/ch1_ashen_grove.lvl", "data/scripts/ch1.cut", "The Ashen Grove", "灰竹林", "第一章", "gale", "boss_tetsugen", "ch1"},
        {2, "ch2_drowned_terrace", "data/levels/ch2_drowned_terrace.lvl", "data/scripts/ch2.cut", "The Drowned Terrace", "水田", "第二章", "stillwater", "boss_warden", "ch2"},
        {3, "ch3_maple_pass", "data/levels/ch3_maple_pass.lvl", "data/scripts/ch3.cut", "Crimson Maple Pass", "紅葉峠", "第三章", "thunder", "boss_crowking", "ch3"},
        {4, "ch4_castle_hakuro", "data/levels/ch4_castle_hakuro.lvl", "data/scripts/ch4.cut", "Castle Hakuro", "白狼城", "第四章", "shadow", "boss_hakuro", "ch4"},
        {5, "ch5_mirror_province", "data/levels/ch5_mirror_province.lvl", "data/scripts/ch5.cut", "The Mirror Province", "鏡国", "第五章", "mirror", "boss_kagami", "ch5"},
        {6, "ch6_heart_of_veil", "data/levels/ch6_heart_of_veil.lvl", "data/scripts/ch6.cut", "The Heart of the Veil", "幕心", "終章", "", "boss_odaka", "ch6"},
    };
    return v;
}

const ChapterDef* findChapter(const std::string& level) {
    for (auto& c : chapterDefs()) if (c.level == level) return &c;
    return nullptr;
}

const ChapterDef& chapterOf(const std::string& level) {
    const ChapterDef* c = findChapter(level);
    return c ? *c : chapterDefs()[0];
}

int loreCountForChapter(int chapter) {
    int n = 0;
    for (auto& l : loreDefs()) if (l.chapter == chapter) n++;
    return n;
}

static const char* kScored[] = {"kills", "executions", "stealthKills", "deflects", "deaths"};

void markChapterStart(SaveData& s) {
    for (const char* k : kScored) s.counters[std::string("@") + k] = s.counter(k);
    s.counters["@time"] = (int)s.playTime;
    s.counters["maxRank"] = 0;
}

void beginChapter(SaveData& s, const ChapterDef& c, bool replay) {
    s.level = c.level;
    s.checkpoint.clear();
    s.hp = maxHpOf(s);
    s.gourds = s.maxGourds;
    if (replay) {
        // forget this chapter's pickups, scenes, camps and champion so it plays fresh
        std::string pre = c.prefix + ".";
        for (auto it = s.flags.begin(); it != s.flags.end();) {
            const std::string& f = *it;
            bool local = f.rfind(pre, 0) == 0 || f.rfind("t:" + pre, 0) == 0 || f.rfind("shrine:" + pre, 0) == 0 || f.rfind("open:" + pre, 0) == 0 ||
                         f == c.bossFlag || f == c.prefix + "_complete" || f.rfind("visited:", 0) == 0;
            it = local ? s.flags.erase(it) : std::next(it);
        }
    }
    markChapterStart(s);
}

ChapterResult scoreChapter(const SaveData& s) {
    ChapterResult r;
    auto delta = [&](const char* k) { return s.counter(k) - s.counter(std::string("@") + k); };
    float played = s.playTime - (float)s.counter("@time");
    const ChapterDef& c = chapterOf(s.level);
    float par = c.number == 1 ? 1500.f : 1800.f;   // under par earns time points
    float timeBonus = std::max(0.f, par - played) * 6.f;
    r.score = delta("kills") * 100 + delta("executions") * 250 + delta("stealthKills") * 220 + delta("deflects") * 60 +
              s.counter("maxRank") * 500 + (int)timeBonus - delta("deaths") * 400 + (!c.stance.empty() && s.has(c.stance) ? 1500 : 0) +
              s.ngPlus * 2000;
    r.score = std::max(0, r.score);
    // later chapters are longer and denser: the bar rises with them
    float k = 1.f + (c.number - 1) * 0.15f;
    r.grade = r.score >= 16000 * k ? "S+" : r.score >= 12000 * k ? "S" : r.score >= 8500 * k ? "A" : r.score >= 5000 * k ? "B" : "C";
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
    for (auto& [k, v] : p.chapterBest) o << "best." << k << "=" << v << "\n";
    for (auto& [k, v] : p.chapterGrade) o << "grade." << k << "=" << v << "\n";
    for (auto& [k, v] : p.chapterTime) o << "time." << k << "=" << v << "\n";
    o << "endings=" << joinSet(p.endings) << "\n";
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
    for (auto& [k, v] : kv) {
        if (k.rfind("best.", 0) == 0) p.chapterBest[k.substr(5)] = std::atoi(v.c_str());
        else if (k.rfind("grade.", 0) == 0) p.chapterGrade[k.substr(6)] = v;
        else if (k.rfind("time.", 0) == 0) p.chapterTime[k.substr(5)] = (float)std::atof(v.c_str());
    }
    p.endings = splitSet(kv["endings"]);
    // v0.1 profiles kept a single record: it belongs to Chapter I
    if (p.bestScore > 0 && !p.chapterBest.count("ch1_ashen_grove")) { p.chapterBest["ch1_ashen_grove"] = p.bestScore; p.chapterGrade["ch1_ashen_grove"] = p.bestGrade; }
    if (p.bestTime > 0 && !p.chapterTime.count("ch1_ashen_grove")) p.chapterTime["ch1_ashen_grove"] = p.bestTime;
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
