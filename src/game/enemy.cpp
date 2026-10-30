#include "game/world.h"
#include "core/util.h"

namespace sv {

static Rng g_aiRng(9001);

// name, windup, active, recover, dmg, posture, perilous, hit{x,y,w,h}, lunge, range, weight, chain, projectile, phase2, grab, poseSet
static EAttack A(const char* n, float wu, float ac, float rc, float dmg, float pos, bool peril, Rect hit, float lunge, float range,
                 float weight, int chain = -1, int poseSet = 0, bool proj = false, bool p2 = false, bool grab = false) {
    EAttack a;
    a.name = n; a.windup = wu; a.active = ac; a.recover = rc; a.damage = dmg; a.posture = pos; a.perilous = peril;
    a.hit = hit; a.lunge = lunge; a.range = range; a.weight = weight; a.chain = chain; a.poseSet = poseSet;
    a.projectile = proj; a.phase2 = p2; a.grab = grab;
    return a;
}

const EDef& enemyDef(EType t) {
    static const EDef sword = {EType::Swordsman, "Vanguard Swordsman", "足軽", 60, 75, 110, 270, 560, 1.0f, 0.30f, 1, 25, 12, {
        A("slash", 0.45f, 0.10f, 0.45f, 15, 18, false, {8, -82, 94, 66}, 220, 125, 1.0f, -1, 0),
        A("double1", 0.38f, 0.10f, 0.12f, 13, 14, false, {8, -82, 94, 66}, 160, 125, 0.8f, 2, 0),
        A("double2", 0.22f, 0.10f, 0.50f, 14, 16, false, {8, -112, 92, 92}, 200, 140, 0.0f, -1, 1),
        A("thrust", 0.70f, 0.16f, 0.70f, 26, 30, true, {0, -68, 124, 30}, 760, 270, 0.45f, -1, 2),
    }};
    static const EDef spear = {EType::Spearman, "Vanguard Spearman", "槍兵", 70, 85, 100, 240, 580, 1.05f, 0.25f, 1, 30, 14, {
        A("poke", 0.50f, 0.12f, 0.45f, 16, 16, false, {10, -72, 168, 28}, 120, 175, 1.0f, -1, 2),
        A("poke2a", 0.40f, 0.10f, 0.14f, 14, 14, false, {10, -72, 168, 28}, 90, 175, 0.7f, 2, 2),
        A("poke2b", 0.22f, 0.10f, 0.50f, 15, 16, false, {10, -90, 168, 28}, 120, 185, 0.0f, -1, 2),
        A("sweep", 0.75f, 0.18f, 0.70f, 24, 28, true, {-30, -36, 214, 36}, 80, 200, 0.5f, -1, 3),
    }};
    static const EDef archer = {EType::Archer, "Vanguard Archer", "弓兵", 40, 50, 100, 230, 760, 0.95f, 0.0f, 1, 20, 10, {
        A("shoot", 0.85f, 0.05f, 0.55f, 12, 10, false, {0, 0, 0, 0}, 0, 900, 1.0f, -1, 4, true),
        A("kick", 0.30f, 0.10f, 0.40f, 8, 25, false, {6, -62, 62, 42}, 150, 85, 0.0f, -1, 5),
    }};
    static const EDef boss = {EType::Tetsugen, "Tetsu, the Iron Ronin", "鉄", 220, 150, 150, 310, 1400, 1.38f, 0.35f, 2, 300, 300, {
        A("cleave1", 0.55f, 0.12f, 0.16f, 20, 20, false, {10, -122, 156, 104}, 260, 175, 1.0f, 1, 0),
        A("cleave2", 0.30f, 0.12f, 0.16f, 20, 20, false, {10, -156, 146, 134}, 220, 175, 0.0f, 2, 1),
        A("cleave3", 0.40f, 0.14f, 0.62f, 24, 28, false, {10, -122, 166, 112}, 300, 175, 0.0f, -1, 0),
        A("overhead", 0.95f, 0.14f, 0.75f, 34, 40, false, {20, -186, 144, 186}, 200, 175, 0.8f, -1, 1),
        A("charge", 0.85f, 0.30f, 0.80f, 30, 30, true, {0, -94, 156, 44}, 1150, 540, 0.7f, -1, 2),
        A("sweep", 0.75f, 0.18f, 0.70f, 26, 26, true, {-60, -46, 290, 46}, 120, 230, 0.6f, -1, 3),
        A("flamewave", 0.70f, 0.10f, 0.70f, 20, 10, true, {0, 0, 0, 0}, 0, 900, 0.8f, -1, 6, true, true),
        A("grab", 0.60f, 0.20f, 0.90f, 36, 0, true, {0, -124, 116, 112}, 640, 280, 0.6f, -1, 2, false, true, true),
        A("fcleave1", 0.45f, 0.12f, 0.10f, 22, 20, false, {10, -122, 166, 112}, 260, 180, 1.0f, 9, 0, false, true),
        A("fcleave2", 0.25f, 0.12f, 0.10f, 22, 20, false, {10, -156, 156, 134}, 220, 180, 0.0f, 10, 1, false, true),
        A("fcleave3", 0.30f, 0.14f, 0.60f, 26, 26, false, {10, -122, 176, 112}, 320, 180, 0.0f, -1, 0, false, true),
    }};
    switch (t) {
    case EType::Spearman: return spear;
    case EType::Archer: return archer;
    case EType::Tetsugen: return boss;
    default: return sword;
    }
}

namespace eposes {
const Pose ready = P(0, 6, 10, -2, 40, 60, 20, 60, 18, -16, -18, -10, 10);
const Pose spearReady = P(0, 6, 8, -2, 70, 10, 50, 40, 18, -16, -18, -10, 0);
const Pose bowReady = P(0, 3, 4, -2, 60, 10, 30, 30, 12, -8, -14, -6, 0);
const Pose broken = P(-6, 22, 44, 20, 10, 10, 0, 10, 60, -110, -10, -80, -40);
const Pose stunned = P(-8, 6, -20, 14, -20, 40, -50, 30, 16, -10, -26, -14, 50);
const Pose guardP = P(-2, 9, 6, 4, 62, 82, 30, 70, 26, -22, -26, -10, 18);
const Pose deadP = P(-12, 36, -94, -10, 80, 10, 120, 10, 92, -4, 96, -10, 30);
const Pose kneelP = P(-4, 26, 30, 20, 40, 30, 10, 30, 70, -140, 0, -60, -60);
const Pose sweepW = P(-6, 18, -10, 0, -60, 40, -30, 30, 50, -80, -30, -40, 70);
const Pose sweepS = P(10, 26, 40, 0, 76, 0, -30, 30, 72, -104, -26, -60, 12);
const Pose bowDraw = P(-2, 5, -2, -4, 90, 0, 82, -150, 22, -12, -22, -8, 0);
const Pose bowLoose = P(-4, 5, -6, -4, 90, 0, 40, -60, 22, -12, -22, -8, 0);
const Pose kickW = P(-4, 6, -10, 0, 30, 60, -30, 40, 50, -100, -10, -10, 10);
const Pose kickS = P(-8, 4, -22, 0, 30, 60, -30, 40, 92, -4, -16, -10, 10);
const Pose waveW = P(-4, 2, -16, -12, 176, 26, 170, 30, 26, -16, -30, -10, 50);
const Pose waveS = P(12, 26, 50, 10, 60, 6, 50, 10, 64, -76, -44, -4, 10);
}

static Pose windupPose(int set) {
    switch (set) {
    case 0: return P(-2, 8, -6, -2, -50, 60, -20, 30, 30, -24, -30, -10, 62);
    case 1: return P(-4, 4, -12, -10, 172, 22, 160, 30, 26, -16, -30, -10, 44);
    case 2: return P(-6, 12, 6, 0, -40, 100, 40, 60, 34, -30, -34, -8, 0);
    case 3: return eposes::sweepW;
    case 4: return eposes::bowDraw;
    case 5: return eposes::kickW;
    default: return eposes::waveW;
    }
}
static Pose strikePose(int set) {
    switch (set) {
    case 0: return P(8, 11, 26, -8, 88, 8, -40, 30, 46, -46, -36, -6, 8);
    case 1: return P(10, 20, 38, 6, 70, 8, 60, 10, 58, -62, -40, -4, 18);
    case 2: return P(14, 16, 38, 0, 92, 0, 70, 20, 60, -60, -46, -4, -2);
    case 3: return eposes::sweepS;
    case 4: return eposes::bowLoose;
    case 5: return eposes::kickS;
    default: return eposes::waveS;
    }
}

static void setE(Enemy& e, EState s) {
    e.st = s; e.stT = 0;
    e.fromPose = e.pose; e.blend = 0;
}

void World::updateEnemies(float dt) {
    tokensInUse = 0;
    for (auto& e : enemies) if (e.hasToken && e.alive()) tokensInUse++;
    for (size_t i = 0; i < enemies.size(); i++) updateEnemy(enemies[i], (int)i, dt);
}

static bool chooseAttack(Enemy& e, float dist) {
    float total = 0;
    for (auto& a : e.def->attacks) if (a.weight > 0 && (!a.phase2 || e.phase2) && a.range >= dist - 20) total += a.weight;
    if (total <= 0) return false;
    float r = g_aiRng.uniform() * total;
    for (size_t i = 0; i < e.def->attacks.size(); i++) {
        auto& a = e.def->attacks[i];
        if (!(a.weight > 0 && (!a.phase2 || e.phase2) && a.range >= dist - 20)) continue;
        r -= a.weight;
        if (r <= 0) { e.atk = (int)i; return true; }
    }
    return false;
}

void World::updateEnemy(Enemy& e, int idx, float dt) {
    e.prevPos = e.pos;
    e.stT += dt;
    e.flash = std::max(0.f, e.flash - dt * 5);
    e.alertIcon = std::max(0.f, e.alertIcon - dt * 0.8f);
    e.glint = std::max(0.f, e.glint - dt * 4);
    e.cooldown -= dt; e.thinkT -= dt;
    e.postureDelay -= dt;
    Player& p = player;
    const EDef& D = *e.def;

    if (e.st == EState::Dead) {
        // the iron ronin lingers for his last words
        if (!e.boss || save->has("boss_outro_done")) e.dissolve += dt * 0.6f;
        if (e.dissolve < 1 && g_aiRng.chance(0.5f)) emit(PType::Ink, e.pos + Vec2{g_aiRng.range(-20, 20), -g_aiRng.range(0, 60)}, 1, 40, rgb(0x0a0204, 0.6f), 3, 0.8f, 1, -PI / 2);
        e.vel.y = std::min(1300.f, e.vel.y + 2900 * dt);
        e.vel.x = damp(e.vel.x, 0.f, 6, dt);
        moveBody(e.pos, e.vel, e.w(), 40, dt);
        enemyAnim(e, dt);
        return;
    }
    if (e.st != EState::Broken && e.postureDelay <= 0 && e.posture > 0) {
        float hpFrac = e.hp / (D.hp * e.hpScale);
        e.posture = std::max(0.f, e.posture - (e.boss ? 9.f : 16.f) * (0.4f + 0.6f * hpFrac) * dt);
    }
    if (e.burn > 0) {
        e.burn -= dt;
        e.hp -= 5 * dt;
        if (g_aiRng.chance(0.4f)) emit(PType::Ember, e.pos + Vec2{g_aiRng.range(-15, 15), -g_aiRng.range(10, e.h())}, 1, 90, rgb(0xff8a3a), 4, 0.6f, 1, -PI / 2);
        if (e.hp <= 0 && e.alive()) { if (e.boss && e.deathblows > 1) e.hp = 1; else { killEnemy(idx, false, false); return; } }
    }

    float dx = p.pos.x - e.pos.x, dy = p.pos.y - e.pos.y;
    float dist = std::abs(dx);
    Vec2 eye = e.pos + Vec2{0, -e.h() * 0.85f};
    bool playerAlive = p.state != PState::Dead;

    // ------------------------------------------------ perception
    auto perceive = [&]() {
        if (!playerAlive || controlLocked) { e.awareness = std::max(0.f, e.awareness - 0.2f * dt); return; }
        bool inCone = dx * e.facing > -20 && dist < D.sight && std::abs(dy) < 170;
        bool see = inCone && lineOfSight(eye, p.pos + Vec2{0, -50});
        float rate = 0;
        if (see) {
            rate = (1.4f - dist / D.sight) * 1.6f;
            if (p.hidden) rate = 0;
            else {
                if (p.crouch) rate *= 0.5f;
                if (p.inGrass) rate *= 0.4f;
            }
            rate *= (1 - stats.stealth);
            if (dist < 120 && !p.hidden) rate = std::max(rate, 2.5f);
        }
        if (p.noise > 0 && dist < 210 && std::abs(dy) < 100) rate += 0.9f * (1 - stats.stealth);
        if (e.sleeping) rate *= 0.3f;
        if (rate > 0) { e.awareness += rate * dt; e.lastKnown = p.pos; }
        else if (e.awareness < 1) e.awareness = std::max(0.f, e.awareness - 0.12f * dt);
        if (e.awareness >= 1) {
            e.awareness = 1;
            e.sleeping = false;
            setE(e, EState::Engage);
            e.alertIcon = 1;
            e.lostT = 0;
            e.cooldown = 0.5f + g_aiRng.uniform() * 0.5f;
            sfx(Sfx::Alert, e.pos, 0.8f);
            if (level.rooms[level.roomAt(e.pos.x)].stealthZone) groveSpotted = true;
            for (auto& o : enemies)
                if (&o != &e && o.alive() && o.awareness < 1 && std::abs(o.pos.x - e.pos.x) < 520 && std::abs(o.pos.y - e.pos.y) < 200) {
                    o.awareness = std::max(o.awareness, 0.97f); o.lastKnown = p.pos;
                }
        } else if (e.awareness > 0.35f && (e.st == EState::Patrol || e.st == EState::Idle)) {
            e.sleeping = false;
            setE(e, EState::Suspicious);
        }
    };

    auto groundAhead = [&](float dir) {
        float x = e.pos.x + dir * (e.w() / 2 + 10);
        int tx = (int)std::floor(x / TILE), ty = (int)std::floor((e.pos.y + 4) / TILE);
        Tile t = level.at(tx, ty);
        bool ground = level.solidAt(tx, ty) || t == Tile::OneWay;
        bool wall = level.solidAt(tx, (int)std::floor((e.pos.y - 20) / TILE));
        Tile below = level.at(tx, ty - 1);
        return ground && !wall && below != Tile::Spikes;
    };
    auto walkToward = [&](float targetX, float speed) {
        float d = targetX - e.pos.x;
        if (std::abs(d) < 6) { e.vel.x = approach(e.vel.x, 0, 2000 * dt); return true; }
        float dir = sign(d);
        e.facing = dir;
        if (!groundAhead(dir)) { e.vel.x = approach(e.vel.x, 0, 3000 * dt); return true; }
        e.vel.x = approach(e.vel.x, dir * speed, 1600 * dt);
        return false;
    };
    auto releaseToken = [&]() { if (e.hasToken && !e.boss) { e.hasToken = false; tokensInUse--; } };

    switch (e.st) {
    case EState::Idle:
        e.vel.x = approach(e.vel.x, 0, 2000 * dt);
        if (!e.boss) perceive();
        else if (playerAlive && dist < 900) e.facing = sign(dx) == 0 ? e.facing : sign(dx);
        break;
    case EState::Patrol: {
        if (e.sleeping) { e.vel.x = 0; perceive(); break; }
        if (e.patrolR - e.patrolL < 8) { e.vel.x = approach(e.vel.x, 0, 2000 * dt); perceive(); break; }
        if (e.patrolWait > 0) {
            e.patrolWait -= dt;
            e.vel.x = approach(e.vel.x, 0, 2000 * dt);
            if (e.patrolWait <= 0) e.facing = -e.facing;
        } else {
            float target = e.facing > 0 ? e.patrolR : e.patrolL;
            if (walkToward(target, D.walk)) e.patrolWait = 1.6f;
        }
        perceive();
        break;
    }
    case EState::Suspicious: {
        e.vel.x = approach(e.vel.x, 0, 2000 * dt);
        float d = e.lastKnown.x - e.pos.x;
        if (std::abs(d) > 10) e.facing = sign(d);
        if (e.stT > 0.9f) walkToward(e.lastKnown.x, D.walk * 0.6f);
        perceive();
        if (e.st == EState::Suspicious && e.awareness < 0.12f) { setE(e, EState::Patrol); e.patrolWait = 0.8f; }
        break;
    }
    case EState::Search: {
        bool arrived = walkToward(e.lastKnown.x, D.walk);
        if (arrived && e.stT > 1.0f) {
            if ((int)(e.stT * 0.8f) % 2 == 0) e.facing = -e.facing;
            if (e.stT > 4.5f) { e.awareness = 0.3f; setE(e, EState::Patrol); }
        }
        perceive();
        break;
    }
    case EState::Engage: {
        if (!playerAlive) { releaseToken(); if (e.stT > 2) { e.awareness = 0; setE(e, e.boss ? EState::Idle : EState::Patrol); } e.vel.x = approach(e.vel.x, 0, 2000 * dt); break; }
        if (controlLocked) { e.vel.x = approach(e.vel.x, 0, 2000 * dt); break; }
        bool los = lineOfSight(eye, p.pos + Vec2{0, -50});
        if (los) { e.lostT = 0; e.lastKnown = p.pos; } else e.lostT += dt;
        if (!e.boss && e.lostT > 5 && dist > 600) { releaseToken(); e.awareness = 0.9f; setE(e, EState::Search); break; }
        if (e.thinkT <= 0 && dist > 20) { float f = sign(dx); if (f != 0 && f != e.facing) { e.facing = f; e.thinkT = e.boss ? 0.15f : 0.28f; } }

        if (e.type == EType::Archer) {
            if (dist < 85 && e.cooldown <= 0) { e.atk = 1; setE(e, EState::Windup); break; }
            if (dist < 280) {
                // retreat while keeping eyes on the target
                float away = -sign(dx);
                if (groundAhead(away)) e.vel.x = approach(e.vel.x, away * D.run * 0.8f, 1600 * dt);
                else e.vel.x = approach(e.vel.x, 0, 2000 * dt);
                e.facing = sign(dx);
            } else e.vel.x = approach(e.vel.x, 0, 2000 * dt);
            if (e.cooldown <= 0 && los && dist < 900 && dist > 120) { e.atk = 0; setE(e, EState::Windup); sfx(Sfx::BowDraw, e.pos, 0.7f); }
            break;
        }
        if (!e.hasToken && (tokensInUse < 2 || e.boss) && e.cooldown <= 0) { e.hasToken = true; tokensInUse++; }
        if (e.hasToken) {
            if (e.cooldown <= 0 && std::abs(dy) < 120 && chooseAttack(e, dist)) { setE(e, EState::Windup); break; }
            float want = e.boss ? 150.f : 105.f;
            if (dist > want) walkToward(p.pos.x, D.run);
            else e.vel.x = approach(e.vel.x, 0, 2000 * dt);
        } else {
            // circle at a respectful distance, waiting for an opening
            float want = 250;
            if (e.thinkT <= 0 && g_aiRng.chance(0.02f)) e.strafeDir = -e.strafeDir;
            float d = dist - want;
            float vx = std::abs(d) > 40 ? sign(dx) * sign(d) * D.walk : e.strafeDir * D.walk * 0.4f;
            if (!groundAhead(sign(vx))) vx = 0;
            e.vel.x = approach(e.vel.x, vx, 1200 * dt);
            e.facing = sign(dx) != 0 ? sign(dx) : e.facing;
        }
        break;
    }
    case EState::Windup: {
        const EAttack& a = D.attacks[e.atk];
        e.vel.x = approach(e.vel.x, 0, 2400 * dt);
        if (e.stT < a.windup * 0.55f && playerAlive && dist > 20) e.facing = sign(dx) != 0 ? sign(dx) : e.facing;
        if (e.stT - dt <= 0 && a.perilous) {
            e.perilousIcon = 1; sfx(Sfx::Perilous, e.pos, 0.9f);
            if (dist < 900 && input) input->rumble(0.1f, 0.55f, 80);   // a haptic warning you can feel
        }
        if (!a.perilous && e.stT >= a.windup * 0.65f && e.stT - dt < a.windup * 0.65f && !a.projectile) { e.glint = 1; sfx(Sfx::UiConfirm, e.pos, 0.25f, 1.8f); }
        if (e.stT >= a.windup) {
            setE(e, EState::Attack);
            e.hitDone = false;
            e.blend = 1;
            e.vel.x = e.facing * a.lunge;
            e.trail.clear();
            if (a.projectile) {
                Projectile pr; pr.fromPlayer = false;
                if (e.type == EType::Archer) {
                    pr.type = ProjType::Arrow;
                    pr.pos = pr.prevPos = e.pos + Vec2{e.facing * 24, -64};
                    Vec2 tgt = p.pos + Vec2{0, -50};
                    Vec2 d = tgt - pr.pos;
                    float spd = 1100, tt = d.len() / spd;
                    pr.vel = d.norm() * spd; pr.vel.y -= 0.5f * 300 * tt;
                    pr.damage = a.damage * e.dmgScale; pr.posture = a.posture; pr.life = 2.5f;
                    sfx(Sfx::Arrow, e.pos, 0.8f);
                } else {
                    pr.type = ProjType::FlameWave;
                    pr.pos = pr.prevPos = Vec2{e.pos.x + e.facing * 80, e.pos.y};
                    pr.vel = {e.facing * 640, 0}; pr.damage = a.damage * e.dmgScale; pr.life = 2.4f;
                    sfx(Sfx::Ember, e.pos, 1.0f, 0.8f);
                    shake(0.35f);
                }
                projectiles.push_back(pr);
            } else sfx(e.boss ? Sfx::SlashHeavy : Sfx::Slash, e.pos, 0.85f, e.boss ? 0.8f : 0.95f);
        }
        break;
    }
    case EState::Attack: {
        const EAttack& a = D.attacks[e.atk];
        e.vel.x = approach(e.vel.x, 0, (a.lunge > 600 ? 1800.f : 2600.f) * dt);
        if (!groundAhead(e.facing)) e.vel.x = 0;
        if (!e.hitDone && !a.projectile && a.hit.w > 0) {
            Rect hb{e.facing > 0 ? e.pos.x + a.hit.x : e.pos.x - a.hit.x - a.hit.w, e.pos.y + a.hit.y, a.hit.w, a.hit.h};
            if (hb.overlaps(playerBox())) {
                e.hitDone = true;
                AttackInfo info; info.damage = a.damage * e.dmgScale; info.posture = a.posture; info.perilous = a.perilous;
                info.origin = e.pos; info.sourceEnemy = idx; info.grab = a.grab; info.knock = e.boss ? 420.f : 300.f;
                HitResult r = hitPlayer(info);
                if (e.st != EState::Attack) break;   // deflect may have staggered us
                if (r == HitResult::Deflected && !e.boss) { setE(e, EState::Stunned); e.vel.x = -e.facing * 160; break; }
                if (r == HitResult::Deflected) e.vel.x = -e.facing * 120;
            }
        }
        if (e.stT >= a.active) {
            if (a.chain >= 0) { e.atk = a.chain; setE(e, EState::Windup); }
            else setE(e, EState::Recover);
        }
        break;
    }
    case EState::Recover: {
        const EAttack& a = D.attacks[std::max(0, e.atk)];
        e.vel.x = approach(e.vel.x, 0, 2400 * dt);
        if (e.stT >= a.recover) {
            releaseToken();
            e.cooldown = e.boss ? g_aiRng.range(0.25f, 0.8f) / (e.phase2 ? 1.3f : 1.f) : g_aiRng.range(0.7f, 1.5f);
            e.atk = -1;
            setE(e, EState::Engage);
        }
        break;
    }
    case EState::Guarding: {
        e.vel.x = approach(e.vel.x, 0, 2000 * dt);
        if (e.stT > 0.55f) {
            setE(e, EState::Engage);
            // riposte after a flurry is blocked
            if (e.blocksInRow >= 3 || (e.boss && e.blocksInRow >= 2)) {
                e.blocksInRow = 0;
                e.atk = 0;
                setE(e, EState::Windup);
                e.stT = D.attacks[0].windup * 0.45f;
            }
        }
        break;
    }
    case EState::Stunned:
        e.vel.x = approach(e.vel.x, 0, 1400 * dt);
        if (e.stT > 0.4f) { setE(e, EState::Engage); e.cooldown = std::max(e.cooldown, 0.2f); }
        break;
    case EState::Broken:
        e.vel.x = approach(e.vel.x, 0, 2000 * dt);
        if (e.stT > (e.boss ? 3.0f : 2.4f)) { e.posture = D.posture * 0.5f; setE(e, EState::Engage); e.cooldown = 0.2f; }
        break;
    case EState::BeingKilled:
        e.vel = {0, e.vel.y};
        if (p.state != PState::Execute && p.state != PState::StealthKill) setE(e, EState::Engage);
        break;
    case EState::Kneel:
        e.vel.x = 0;
        if (e.stT > 0.6f && g_aiRng.chance(0.5f)) emit(PType::Ember, e.pos + Vec2{g_aiRng.range(-40, 40), -g_aiRng.range(0, e.h())}, 1, 120, rgb(0xff6a20), 5, 0.8f, 1, -PI / 2);
        if (e.stT > 3.2f) {
            setE(e, EState::Engage);
            e.cooldown = 0.6f;
            sfx(Sfx::Ignite, e.pos, 1.0f);
            emit(PType::Ember, e.pos + Vec2{0, -60}, 60, 500, rgb(0xff7a2a), 6, 0.9f);
            shake(0.6f);
            flash = settings && settings->reduceFlashes ? 0.1f : 0.35f; flashColor = rgb(0xff7a2a);
        }
        break;
    case EState::Dead: break;
    }

    // physics
    e.vel.y = std::min(1300.f, e.vel.y + 2900 * dt);
    moveBody(e.pos, e.vel, e.w(), e.h(), dt);
    enemyAnim(e, dt);
}

void World::enemyAnim(Enemy& e, float dt) {
    const EDef& D = *e.def;
    Pose target;
    float t = e.stT;
    bool spear = e.type == EType::Spearman, bow = e.type == EType::Archer;
    const Pose& ready = spear ? eposes::spearReady : (bow ? eposes::bowReady : eposes::ready);
    float sp = std::abs(e.vel.x);
    switch (e.st) {
    case EState::Patrol: case EState::Search: case EState::Suspicious: case EState::Idle: case EState::Engage: {
        if (sp > 20) {
            e.walkPhase += dt * sp / 30.f;
            float s = std::sin(e.walkPhase), c = std::cos(e.walkPhase);
            float k = clamp01(sp / D.run);
            target = (e.st == EState::Engage) ? ready : eposes::bowReady;
            if (e.st != EState::Engage && !bow) { target.armFU = 10 * DEG; target.armFL = 30 * DEG; target.sword = -40 * DEG; }
            target.legFU = (6 + 32 * s * (0.5f + k)) * DEG; target.legBU = (6 - 32 * s * (0.5f + k)) * DEG;
            target.legFL = (-10 - 50 * std::max(0.f, c)) * DEG; target.legBL = (-10 - 50 * std::max(0.f, -c)) * DEG;
            target.armBU = (-24 * s) * DEG;
            target.lean = (6 + 12 * k) * DEG;
            target.hipY = 4 - 3 * std::abs(c);
        } else {
            target = e.st == EState::Engage ? ready : eposes::bowReady;
            if (e.st != EState::Engage && !bow) { target.armFU = 10 * DEG; target.armFL = 30 * DEG; target.sword = -40 * DEG; }
            float br = std::sin(time * 2 + e.id);
            target.hipY += br; target.lean += br * DEG;
            if (e.sleeping) { target = eposes::kneelP; target.head = 40 * DEG; }
            if (e.st == EState::Suspicious) target.head = (-14 + 10 * std::sin(time * 3)) * DEG;
        }
        break;
    }
    case EState::Windup: {
        const EAttack& a = D.attacks[e.atk];
        Pose w = windupPose(a.poseSet);
        if (spear && a.poseSet == 2) { w.armFU = 30 * DEG; w.armFL = 40 * DEG; }
        float k = ease::outCubic(clamp01(t / (a.windup * 0.7f)));
        target = lerp(e.fromPose, w, k);
        // trembling anticipation in the last third
        if (t > a.windup * 0.6f) target.hipX += std::sin(time * 60) * 0.8f;
        e.blend = 1;
        break;
    }
    case EState::Attack: {
        const EAttack& a = D.attacks[e.atk];
        target = lerp(windupPose(a.poseSet), strikePose(a.poseSet), ease::outExpo(clamp01(t / std::max(0.04f, a.active * 0.6f))));
        e.blend = 1;
        break;
    }
    case EState::Recover: {
        const EAttack& a = D.attacks[std::max(0, e.atk)];
        target = lerp(strikePose(a.poseSet), ready, ease::inOutQuad(clamp01(t / a.recover)));
        e.blend = 1;
        break;
    }
    case EState::Guarding: target = eposes::guardP; break;
    case EState::Stunned: target = eposes::stunned; break;
    case EState::Broken: case EState::BeingKilled: {
        target = eposes::broken;
        target.hipY += std::sin(time * 3) * 1.5f;
        break;
    }
    case EState::Kneel: target = eposes::kneelP; break;
    case EState::Dead: target = eposes::deadP; break;
    }
    if (e.blend < 1) {
        e.blend = std::min(1.f, e.blend + dt / 0.1f);
        e.pose = lerp(e.fromPose, target, ease::outQuad(e.blend));
    } else e.pose = target;
    Rig rig; rig.scale = D.scale;
    Joints j = solve(rig, e.pose, e.pos, e.facing);
    e.sash.update(j.chest + Vec2{-e.facing * 8, -14}, {0, 600}, palette.wind * 3 + Vec2{-e.vel.x * 2, 0}, dt, 0.95f, e.pos.y - 2);
    if (e.st == EState::Attack && !D.attacks[e.atk].projectile) e.trail.push_back({lerp(j.swordBase, j.swordTip, 0.35f), j.swordTip, time});
    while (!e.trail.empty() && time - e.trail.front().t > 0.12f) e.trail.pop_front();
}

} // namespace sv
