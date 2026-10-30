// Kage Rank (style meter), wind-beacon trials, and the Gale Stance's wind blades.
#include "game/world.h"
#include "core/util.h"

namespace sv {

// ------------------------------------------------------------------ Kage Rank

static const float kRankThresholds[] = {0, 80, 180, 300, 440, 600};
static const int kRanks = 6;

const char* World::rankLetter(int r) { static const char* l[] = {"D", "C", "B", "A", "S", "SS"}; return l[std::clamp(r, 0, 5)]; }
const char* World::rankName(int r) { static const char* n[] = {"Dull", "Calm", "Bold", "Artful", "Shadow", "Kage"}; return n[std::clamp(r, 0, 5)]; }
float World::rankThreshold(int r) { return r >= kRanks ? 700.f : kRankThresholds[std::clamp(r, 0, 5)]; }

void World::addStyle(StyleAct kind, float amount) {
    // repetition is dull: each recent repeat of the same move halves its worth
    int repeats = 0;
    for (int k : styleRecent) if (k == kind) repeats++;
    amount *= std::pow(0.55f, (float)repeats);
    styleRecent.push_back(kind);
    if (styleRecent.size() > 4) styleRecent.pop_front();
    style = std::min(700.f, style + amount);
    styleIdle = 0;
    int r = 0;
    while (r + 1 < kRanks && style >= kRankThresholds[r + 1]) r++;
    if (r > styleRank) {
        styleRank = r;
        stylePop = 1;
        if (audio) audio->play(Sfx::UiConfirm, 0.35f, 0.7f + 0.12f * r);
        if (save && r > save->counter("maxRank")) save->counters["maxRank"] = r;
        if (r == 5) achieve("rank_ss");
    }
}

void World::loseStyle(float amount) {
    style = std::max(0.f, style - amount);
    int r = 0;
    while (r + 1 < kRanks && style >= kRankThresholds[r + 1]) r++;
    styleRank = r;
    styleRecent.clear();
}

// ------------------------------------------------------------------ trials

void World::startTrial(Mechanism& m) {
    if (trialActive || m.active) return;
    trialActive = true;
    trialT = 0;
    trialLimit = m.w;
    trialMech = m.id;
    trialEnemies.clear();
    int i = 0;
    for (auto& entry : split(m.text, ',')) {
        auto kv = split(entry, ':');
        if (kv.size() != 2) continue;
        EType t = kv[0] == "spearman" ? EType::Spearman : (kv[0] == "archer" ? EType::Archer : EType::Swordsman);
        Enemy e;
        e.id = (int)enemies.size();
        e.type = t; e.def = &enemyDef(t);
        e.pos = e.prevPos = e.home = m.pos + Vec2{std::stof(kv[1]) * TILE, -2};
        e.facing = e.pos.x > player.pos.x ? -1.f : 1.f;
        float ng = (float)save->ngPlus;
        e.hpScale = 1 + 0.5f * ng; e.dmgScale = 1 + 0.3f * ng;
        e.hp = e.def->hp * e.hpScale;
        e.deathblows = 1;
        e.awareness = 1; e.lastKnown = player.pos;
        e.st = EState::Engage;
        e.cooldown = 1.2f + i * 0.4f;
        e.patrolL = e.patrolR = e.pos.x;
        e.sash.init(e.pos + Vec2{0, -70}, 7, 6);
        e.flash = 1;
        enemies.push_back(e);
        trialEnemies.push_back(e.id);
        emit(PType::Thread, e.pos + Vec2{0, -40}, 20, 260, rgb(0x9ff0e0), 2.5f, 0.8f);
        i++;
    }
    sfx(Sfx::Perilous, m.pos, 1.0f, 0.8f);
    shake(0.4f);
    ripple = 1.0f; rippleAt = m.pos + Vec2{0, -60};
    flash = settings && settings->reduceFlashes ? 0.08f : 0.25f; flashColor = rgb(0x9ff0e0);
    event(WorldEvent::Toast, "Wind Trial", strfmt("Defeat every foe within %d seconds.", (int)trialLimit));
}

int World::trialRemaining() const {
    int n = 0;
    for (int id : trialEnemies) if (id < (int)enemies.size() && enemies[id].alive()) n++;
    return n;
}

void World::updateTrial(float dt) {
    if (!trialActive) return;
    trialT += dt;
    if (trialRemaining() == 0) {
        trialActive = false;
        Mechanism* m = mech(trialMech);
        std::string reward = m ? m->item : "gale";
        if (m) m->active = true;
        save->flags.insert(reward);
        int ms = (int)(trialT * 1000);
        if (save->counter("trialBestMs") == 0 || ms < save->counter("trialBestMs")) save->counters["trialBestMs"] = ms;
        achieve(reward);
        event(WorldEvent::Toast, "Trial complete", strfmt("%.1f seconds", trialT));
        event(WorldEvent::Cutscene, reward + "_awaken");
        stats = computeStats(*save);
        return;
    }
    if (trialT > trialLimit || player.state == PState::Dead) {
        trialActive = false;
        for (int id : trialEnemies)
            if (id < (int)enemies.size() && enemies[id].alive()) { enemies[id].st = EState::Dead; enemies[id].dissolve = 0.3f; enemies[id].countedDead = true; }
        if (player.state != PState::Dead) event(WorldEvent::Toast, "The wind turns away", "Touch the stone to try the trial again.");
    }
}

// ------------------------------------------------------------------ Gale Stance

void World::spawnGaleBlade(float dmgScale) {
    Projectile pr;
    pr.type = ProjType::Gale;
    pr.pos = pr.prevPos = player.pos + Vec2{player.facing * 34, -54};
    pr.vel = {player.facing * 1350, 0};
    pr.damage = (8 + stats.attack * 0.45f) * dmgScale;
    pr.posture = 16;
    pr.life = 0.42f;
    projectiles.push_back(pr);
    sfx(Sfx::Swoosh, player.pos, 0.8f, 1.35f);
    emit(PType::Thread, pr.pos, 10, 300, rgb(0x9ff0e0), 2, 0.4f, 1.0f, player.facing > 0 ? 0 : PI);
}

} // namespace sv
