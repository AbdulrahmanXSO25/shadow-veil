#include "game/world.h"
#include "core/util.h"

namespace sv {

static Rng g_fxRng(4242);

Rect Mechanism::solidRect() const {
    switch (kind) {
    case MechKind::Gate: {
        float hpx = h * TILE * (1 - ease::inOutCubic(clamp01(t)));
        return {pos.x - TILE * 0.5f, pos.y - h * TILE, TILE, std::max(0.f, hpx)};
    }
    case MechKind::Bridge: return {pos.x, pos.y, w * TILE, TILE * 0.5f};
    default: return {0, 0, 0, 0};
    }
}

// =================================================================== setup

bool World::init(const std::string& levelPath, SaveData* s, const Settings* st, Audio* a, Input* in, std::string& err) {
    save = s; settings = st; audio = a; input = in;
    if (!loadLevel(levelPath, level, err)) return false;
    triggerFired.assign(level.triggers.size(), false);
    for (size_t i = 0; i < level.triggers.size(); i++)
        if (level.triggers[i].once && save->has("t:" + level.triggers[i].id)) triggerFired[i] = true;
    stats = computeStats(*save);
    buildBackground();
    spawnFromLevel();
    palette = paletteByName(level.rooms[curRoom].palette);
    return true;
}

void World::spawnFromLevel() {
    enemies.clear(); projectiles.clear(); pickups.clear(); mechs.clear(); npcs.clear(); anchors.clear();
    lights.clear(); particles.clear();
    bossActive = false; bossIdx = -1; tokensInUse = 0;
    trialActive = false; trialEnemies.clear();
    style = 0; styleRank = 0; styleRecent.clear();
    int nextId = 0;
    for (auto& sp : level.spawns) {
        const std::string& k = sp.kind;
        bool isFoe = false;
        EType ft = EType::Swordsman;
        if (k == "boss") { isFoe = true; ft = etypeByName(sp.get("who", "tetsu"), &isFoe); }
        else ft = etypeByName(k, &isFoe);
        if (isFoe) {
            const EDef& D = enemyDef(ft);
            if (D.boss && !D.flag.empty() && save->has(D.flag)) continue;
            Enemy e = makeEnemy(ft, sp.pos, sp.get("face", "left") == "right" ? 1.f : -1.f);
            e.id = nextId++;
            e.spawnId = sp.id;
            float patrol = sp.getf("patrol", (D.ranged || ft == EType::Lurker || D.boss) ? 0.f : 3.f) * TILE;
            e.patrolL = sp.pos.x - patrol; e.patrolR = sp.pos.x + patrol;
            e.sleeping = sp.get("sleep") == "1";
            e.lurking = ft == EType::Lurker && sp.get("lurk", "1") == "1";
            e.st = e.boss ? EState::Idle : EState::Patrol;
            enemies.push_back(e);
            if (e.boss) bossIdx = (int)enemies.size() - 1;
        } else if (k == "anchor") {
            anchors.push_back(sp.pos);
        } else if (k == "mon" || k == "shuriken" || k == "bead" || k == "material") {
            if (save->has(sp.id)) continue;
            Pickup p; p.kind = k; p.pos = sp.pos - Vec2{0, 18}; p.spawnId = sp.id;
            p.amount = k == "mon" ? (int)sp.getf("amount", 40) : (k == "shuriken" ? 3 : 1);
            p.item = sp.get("item", k == "material" ? "iron_sand" : "");
            p.bob = sp.pos.x * 0.01f;
            pickups.push_back(p);
        } else if (k == "npc") {
            Npc n; n.id = sp.get("id", sp.id); n.kind = sp.get("who", "fuki");
            n.pos = n.prevPos = sp.pos; n.facing = sp.get("face", "left") == "right" ? 1.f : -1.f;
            n.visible = sp.get("hidden") != "1";
            n.alpha = n.visible ? 1.f : 0.f;
            n.cloth.init(n.pos + Vec2{0, -60}, 6, 6);
            npcs.push_back(n);
            Mechanism m; m.kind = MechKind::Npc; m.pos = sp.pos; m.id = n.id; m.text = sp.get("talk"); m.spawnId = sp.id; m.name = sp.get("name", "Talk");
            if (!m.text.empty()) mechs.push_back(m);
        } else {
            Mechanism m; m.spawnId = sp.id; m.pos = sp.pos;
            m.id = sp.get("id", sp.id); m.link = sp.get("link"); m.item = sp.get("item"); m.text = sp.get("text");
            m.name = sp.get("name");
            m.w = sp.getf("w", 1); m.h = sp.getf("h", 4);
            if (k == "gate") { m.kind = MechKind::Gate; m.active = save->has("open:" + m.id) || sp.get("open") == "1"; m.t = m.active ? 1.f : 0.f; m.solid = true; }
            else if (k == "bell") { m.kind = MechKind::Bell; m.active = save->has("open:" + m.link); m.pos.y = (sp.pos.y - TILE); }
            else if (k == "rope") { m.kind = MechKind::Rope; m.active = save->has("open:" + m.link); }
            else if (k == "bridge") { m.kind = MechKind::Bridge; m.w = sp.getf("len", 6); m.active = save->has("open:" + m.id); m.t = m.active ? 1.f : 0.f; }
            else if (k == "chest") { m.kind = MechKind::Chest; m.active = save->has(sp.id); m.t = m.active ? 1.f : 0.f; }
            else if (k == "scroll") { m.kind = MechKind::Scroll; m.item = sp.get("lore"); m.active = save->has(sp.id); }
            else if (k == "shrine") { m.kind = MechKind::Shrine; m.name = level.rooms[sp.room].name; m.active = save->has("shrine:" + sp.id); }
            else if (k == "examine") { m.kind = MechKind::Examine; }
            else if (k == "trial") { m.kind = MechKind::Trial; m.item = sp.get("reward", "gale"); m.text = sp.get("foes"); m.w = sp.getf("time", 75); m.active = save->has(m.item); }
            else if (k == "sluice") { m.kind = MechKind::Sluice; m.active = save->has("open:" + m.link); m.t = m.active ? 1.f : 0.f; }
            else if (k == "drain") {
                // a body of water (w x h tiles, bottom-left at the spawn) that drains or fills when its id opens
                m.kind = MechKind::Drain;
                m.text = sp.get("mode", "drain");
                m.area = Rect{sp.pos.x - TILE * 0.5f, sp.pos.y - m.h * TILE, m.w * TILE, m.h * TILE};
                m.active = save->has("open:" + m.id);
            }
            else if (k == "beacon") { m.kind = MechKind::Beacon; m.active = true; m.range = sp.getf("range", 9); m.speed = sp.getf("speed", 0.7f); m.h = sp.getf("h", 3); m.timer = sp.pos.x * 0.013f; }
            else if (k == "player" || k == "boss_door") continue;
            else { logWarn("unknown spawn kind '%s'", k.c_str()); continue; }
            if (m.kind == MechKind::Scroll && m.active) continue;
            mechs.push_back(m);
        }
    }
    resetEnvironment();
    // lights
    for (auto& m : mechs) if (m.kind == MechKind::Beacon) lights.push_back({m.pos + Vec2{0, -m.h * TILE}, rgb(0xffb35c), 360, 0.25f});
    for (auto& p : level.props) {
        if (p.kind == "lantern") lights.push_back({p.pos + Vec2{TILE * 0.5f, -TILE * 1.1f}, rgb(0xffb35c), 260, 0.15f});
        else if (p.kind == "hanging_lantern") lights.push_back({p.pos + Vec2{TILE * 0.5f, -TILE * 0.6f}, rgb(0xff9a4a), 220, 0.2f});
        else if (p.kind == "fire") lights.push_back({p.pos + Vec2{TILE * 0.5f, -TILE * 0.8f}, rgb(0xff6a20), 420, 0.5f});
    }
    for (auto& m : mechs) if (m.kind == MechKind::Shrine) lights.push_back({m.pos + Vec2{0, -70}, rgb(0xffd27a), 320, 0.05f});

    // place the player
    const Spawn* start = level.findSpawn("player");
    Vec2 at = start->pos;
    if (!save->checkpoint.empty())
        if (const Spawn* cp = level.findById(save->checkpoint)) at = cp->pos + Vec2{40, 0};
    Player& p = player;
    float keepHp = save->hp;
    p = Player();
    p.pos = p.prevPos = p.lastSafe = at;
    p.hp = keepHp > 0 ? keepHp : maxHpOf(*save);
    p.ki = save->ki;
    p.facing = 1;
    p.scarf.init(at + Vec2{0, -70}, 12, 7);
    p.tailA.init(at + Vec2{0, -78}, 5, 5);
    p.tailB.init(at + Vec2{0, -78}, 4, 5);
    cam.pos = cam.prevPos = cam.target = at + Vec2{0, -150};
    curRoom = level.roomAt(at.x);
    cam.zoom = cam.targetZoom = level.rooms[curRoom].zoom;
}

void World::respawnAtCheckpoint() {
    save->hp = maxHpOf(*save);
    save->gourds = save->maxGourds;
    save->ki = 0;
    stats = computeStats(*save);
    spawnFromLevel();
    hitstop = 0; slowmo = 0; flash = 0; desat = 0; controlLocked = false;
    hasMusicOverride = false;
}

// =================================================================== physics

bool World::solidTile(int tx, int ty) const { return level.solidAt(tx, ty); }

bool World::rectSolid(const Rect& r, bool includeOneWay, float prevBottom) const {
    int x0 = (int)std::floor(r.x / TILE), x1 = (int)std::floor((r.right() - 0.001f) / TILE);
    int y0 = (int)std::floor(r.y / TILE), y1 = (int)std::floor((r.bottom() - 0.001f) / TILE);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            if (level.solidAt(x, y)) return true;
            if (includeOneWay && level.at(x, y) == Tile::OneWay && prevBottom <= y * TILE + 0.5f) return true;
        }
    for (auto& m : mechs) {
        if (m.kind == MechKind::Gate && m.t < 1) { if (m.solidRect().overlaps(r)) return true; }
        else if (m.kind == MechKind::Bridge && m.t >= 1) {
            Rect b = m.solidRect();
            if (b.overlaps(r) && (!includeOneWay ? false : prevBottom <= b.y + 0.5f)) return true;
        }
    }
    return false;
}

World::MoveResult World::moveBody(Vec2& pos, Vec2& vel, float w, float h, float dt, bool drop) {
    MoveResult res;
    Vec2 delta = vel * dt;
    int steps = std::max(1, (int)std::ceil(std::max(std::abs(delta.x), std::abs(delta.y)) / 12.f));
    Vec2 st = delta / (float)steps;
    for (int i = 0; i < steps; i++) {
        if (st.x != 0) {
            pos.x += st.x;
            Rect b{pos.x - w / 2, pos.y - h, w, h};
            if (rectSolid(b)) {
                // back out in small increments for exact contact against tiles and dynamic solids
                float dir = sign(st.x);
                for (int k = 0; k < 40 && rectSolid(Rect{pos.x - w / 2, pos.y - h, w, h}); k++) pos.x -= dir * 0.5f;
                res.hitX = true; res.wall = (int)dir; vel.x = 0; st.x = 0;
            }
        }
        if (st.y != 0) {
            float prevBottom = pos.y;
            pos.y += st.y;
            Rect b{pos.x - w / 2, pos.y - h, w, h};
            bool down = st.y > 0;
            if (rectSolid(b, down && !drop, prevBottom)) {
                if (down) {
                    // snap to the surface we landed on
                    float top = std::floor(pos.y / TILE) * TILE;
                    pos.y = top - 0.01f;
                    if (rectSolid(Rect{pos.x - w / 2, pos.y - h, w, h}, !drop, prevBottom)) {
                        pos.y = prevBottom;
                        for (int k = 0; k < 60 && !rectSolid(Rect{pos.x - w / 2, pos.y + 0.5f - h, w, h}, !drop, prevBottom); k++) pos.y += 0.5f;
                    }
                    res.ground = true;
                } else {
                    for (int k = 0; k < 60 && rectSolid(Rect{pos.x - w / 2, pos.y - h, w, h}); k++) pos.y += 0.5f;
                    res.ceiling = true;
                }
                res.hitY = true; vel.y = 0; st.y = 0;
            }
        }
    }
    return res;
}

bool World::onGroundAt(Vec2 pos, float w) const {
    Rect feet{pos.x - w / 2 + 1, pos.y, w - 2, 2};
    return rectSolid(feet, true, pos.y);
}

bool World::lineOfSight(Vec2 a, Vec2 b) const {
    Vec2 d = b - a; float L = d.len();
    int n = (int)(L / 12.f);
    for (int i = 1; i < n; i++) {
        Vec2 p = a + d * (i / (float)n);
        if (level.solidAt((int)std::floor(p.x / TILE), (int)std::floor(p.y / TILE))) return false;
    }
    for (auto& m : mechs)
        if (m.kind == MechKind::Gate && m.t < 0.5f) {
            Rect r = m.solidRect();
            if ((a.x < r.x) != (b.x < r.x) && std::max(a.y, b.y) > r.y) return false;
        }
    return true;
}

// =================================================================== services

void World::sfx(Sfx s, Vec2 at, float vol, float pitch) {
    if (!audio) return;
    Vec2 rel = at - cam.pos;
    float dist = rel.len();
    float att = clamp01(1.2f - dist / 1600.f);
    if (att <= 0.01f) return;
    float slow = slowmo > 0 ? lerp(1.f, 0.8f, 1 - slowmoScale) : 1.f;
    audio->play(s, vol * att, pitch * slow * (0.96f + g_fxRng.uniform() * 0.08f), clampf(rel.x / 900.f, -0.8f, 0.8f));
}

void World::shake(float amount) { cam.trauma = std::min(1.f, cam.trauma + amount); }
void World::hitStop(float secs) { hitstop = std::max(hitstop, secs); }

void World::emit(PType t, Vec2 pos, int n, float speed, Color col, float size, float life, float spread, float dir) {
    for (int i = 0; i < n; i++) {
        Particle p;
        p.type = t; p.pos = pos; p.col = col;
        float a = dir + (g_fxRng.uniform() - 0.5f) * spread;
        float sp = speed * (0.35f + 0.65f * g_fxRng.uniform());
        p.vel = Vec2{std::cos(a), std::sin(a)} * sp;
        p.size = size * (0.6f + 0.8f * g_fxRng.uniform());
        p.life = p.maxLife = life * (0.6f + 0.8f * g_fxRng.uniform());
        p.rot = g_fxRng.range(0, TAU); p.rotVel = g_fxRng.range(-8, 8);
        switch (t) {
        case PType::Spark: p.drag = 3; p.gravity = 900; p.fg = true; break;
        case PType::Ink: p.drag = 2.5f; p.gravity = 1100; p.fg = true; break;
        case PType::Ember: p.drag = 0.8f; p.gravity = -60; break;
        case PType::Dust: p.drag = 4; p.gravity = -20; break;
        case PType::Smoke: p.drag = 1.5f; p.gravity = -40; break;
        case PType::Flash: p.fg = true; break;
        case PType::Petal: case PType::Leaf: p.drag = 1.2f; p.gravity = 120; p.fg = g_fxRng.chance(0.3f); break;
        case PType::Thread: p.drag = 1; p.gravity = -30; p.fg = true; break;
        default: break;
        }
        particles.push_back(p);
    }
}

void World::inkBurst(Vec2 pos, Vec2 dir, int n) {
    float a = std::atan2(dir.y, dir.x);
    if (settings && !settings->inkSplatter) {
        emit(PType::Petal, pos, n, 420, rgb(0xf3e9e0), 6, 1.4f, 1.6f, a);
        return;
    }
    emit(PType::Ink, pos, n, 650, rgb(0x0a0204), 5, 0.7f, 1.3f, a);
    emit(PType::Ink, pos, n / 2, 380, rgb(0x5a0812), 4, 0.6f, 1.8f, a);
}

void World::achieve(const std::string& id) { event(WorldEvent::Achievement, id); }
void World::event(WorldEvent::Type t, const std::string& a, const std::string& b, float num) { events.push_back({t, a, b, num}); }

void World::gainKi(float base) { player.ki = std::min(100.f, player.ki + base * (1 + stats.kiGain)); }

void World::gainEssence(int n) {
    save->essence += n;
    while (save->essence >= kEssencePerPoint) {
        save->essence -= kEssencePerPoint;
        save->skillPoints++;
        event(WorldEvent::Toast, "Skill point earned", "Spend it at a camp.");
    }
}

// =================================================================== combat

HitResult World::hitPlayer(const AttackInfo& a) {
    Player& p = player;
    if (p.state == PState::Dead || p.state == PState::Execute || p.state == PState::StealthKill || controlLocked) return HitResult::None;
    if (p.state == PState::Dash || p.iframes > 0) {
        if (p.state == PState::Dash && time - p.lastHitT > 0.5f) {
            p.lastHitT = time;   // one reward per step
            addStyle(SDodge, 40);
            if (save->skills.count("aura_tail")) {
                // Tailwind: slipping through a blow bends time around you
                slowmo = 0.7f; slowmoScale = 0.35f;
                if (audio) audio->setLowpass(0.4f);
                gainKi(15);
                ripple = 0.8f; rippleAt = p.pos + Vec2{0, -40};
                emit(PType::Thread, p.pos + Vec2{0, -40}, 24, 320, rgb(0x9ff0e0), 2.5f, 0.6f);
            }
        }
        return HitResult::Dodged;
    }
    float toSource = sign(a.origin.x - p.pos.x);
    bool facingSource = toSource == 0 || toSource == p.facing;
    Vec2 contact = p.pos + Vec2{p.facing * 16, -50};
    if (!a.perilous && p.state == PState::Guard && facingSource) {
        if (p.deflectT > 0) {
            sfx(Sfx::Deflect, contact, 1.0f, 1.0f + g_fxRng.range(-0.05f, 0.05f));
            emit(PType::Spark, contact, 26, 900, rgb(0xfff2c0), 3, 0.35f);
            emit(PType::Flash, contact, 1, 0, rgb(0xfff6dd), 70, 0.12f);
            hitStop(0.075f);
            shake(0.22f);
            gainKi(12);
            addStyle(SDeflect, 45);
            save->bump("deflects");
            if (save->skills.count("aura_eye")) {
                // Eye of the Storm: a perfect deflect bursts outward as wind
                for (auto& o : enemies) {
                    if (!o.alive() || (o.pos - p.pos).len() > 220) continue;
                    o.posture += 18 * (1 + stats.postureDmg); o.postureDelay = 1.4f;
                    o.vel.x += sign(o.pos.x - p.pos.x) * 260;
                    if (o.posture >= o.def->posture) { o.posture = o.def->posture; o.st = EState::Broken; o.stT = 0; o.atk = -1; }
                }
                emit(PType::Thread, contact, 30, 520, rgb(0x9ff0e0), 2.5f, 0.5f);
                sfx(Sfx::Swoosh, contact, 0.8f, 0.8f);
            }
            if (save->counter("deflects") == 1) achieve("first_deflect");
            if (save->counter("deflects") >= 50) achieve("deflect_50");
            input->rumble(0.3f, 0.7f, 90);
            if (a.sourceEnemy >= 0 && a.sourceEnemy < (int)enemies.size()) {
                Enemy& e = enemies[a.sourceEnemy];
                if (hasStance("stillwater")) { e.chill = 2.2f; emit(PType::Frost, contact, 18, 260, rgb(0xbfeaff), 3, 0.6f); }
                e.posture += (a.posture * 1.4f + 16) * (1 + stats.postureDmg);
                e.postureDelay = 1.6f;
                if (e.posture >= e.def->posture) {
                    e.posture = e.def->posture;
                    e.st = EState::Broken; e.stT = 0; e.atk = -1;
                    sfx(Sfx::Perilous, e.pos, 0.6f, 1.6f);
                } else if (!e.boss || (e.atk >= 0 && e.def->attacks[e.atk].chain < 0)) {
                    if (!e.boss) { e.st = EState::Stunned; e.stT = 0; e.vel.x = -e.facing * 180; }
                }
            }
            p.deflectT = 0.10f;  // chained deflects stay crisp
            return HitResult::Deflected;
        }
        float dmg = a.damage * 0.2f * (1 - stats.defense);
        p.hp -= dmg;
        p.vel.x = -p.facing * 260;
        sfx(Sfx::Clang, contact, 0.8f);
        loseStyle(25);
        emit(PType::Spark, contact, 10, 500, rgb(0xffd890), 2.5f, 0.25f);
        hitStop(0.035f);
        shake(0.12f);
        if (p.hp <= 0) { p.hp = 0; p.state = PState::Dead; p.stateT = 0; }
        return HitResult::Blocked;
    }
    float dmg = a.damage * (1 - stats.defense);
    if (a.grab) dmg *= 1.0f;
    p.hp -= dmg;
    p.lastHitT = time;
    p.hurtFlash = 1;
    loseStyle(180);
    if (bossActive) { p.tookDamageInBoss = true; playerDamageTakenBoss += dmg; }
    if (p.state == PState::Heal) p.healDone = true;  // interrupted: the gourd is spent
    p.state = p.hp > 0 ? PState::Hurt : PState::Dead;
    p.stateT = 0;
    p.iframes = a.grab ? 1.2f : 0.75f;
    float dir = -toSource; if (dir == 0) dir = -p.facing;
    p.vel = Vec2{dir * a.knock * (a.grab ? 2.2f : 1.f), a.grab ? -700.f : -260.f};
    p.onGround = false;
    sfx(Sfx::Hurt, p.pos, 1.0f);
    inkBurst(p.pos + Vec2{0, -45}, Vec2{dir, -0.3f}, 10);
    hitStop(a.grab ? 0.12f : 0.06f);
    shake(a.grab ? 0.6f : 0.4f);
    flash = std::max(flash, settings && settings->reduceFlashes ? 0.05f : 0.18f);
    flashColor = rgb(0x9c1020);
    chroma = 1.0f;
    input->rumble(0.8f, 0.4f, 180);
    if (p.hp <= 0) { p.hp = 0; p.deathT = 0; }
    return HitResult::Hit;
}

HitResult World::hitEnemy(int idx, const AttackInfo& a) {
    Enemy& e = enemies[idx];
    if (!e.alive() || e.st == EState::Kneel) return HitResult::None;
    bool facingPlayer = sign(player.pos.x - e.pos.x) == e.facing;
    Vec2 contact = e.pos + Vec2{-e.facing * 4, -e.h() * 0.6f};
    bool wasAware = e.awareness >= 1;
    e.awareness = 1; e.lastKnown = player.pos;
    bool canGuard = wasAware && facingPlayer && !a.ember && e.st != EState::Broken && e.st != EState::Windup && e.st != EState::Attack &&
                    e.st != EState::Stunned && e.def->canGuard && !e.lurking;
    float guardP = e.def->guardChance + (e.st == EState::Guarding ? 0.35f : 0) + e.blocksInRow * 0.05f;
    if (canGuard && g_fxRng.uniform() < guardP) {
        e.posture += a.posture * (1 + stats.postureDmg);
        e.postureDelay = 1.4f;
        e.blocksInRow++;
        e.st = EState::Guarding; e.stT = 0;
        addStyle(SHit, 4);
        sfx(Sfx::Clang, contact, 0.9f, 1.05f);
        emit(PType::Spark, contact + Vec2{e.facing * 16, 0}, 12, 600, rgb(0xffe0a0), 2.5f, 0.25f);
        hitStop(0.045f);
        shake(0.1f);
        if (!a.projectile) player.vel.x = -player.facing * 120;
        if (e.posture >= e.def->posture) {
            e.posture = e.def->posture;
            e.st = EState::Broken; e.stT = 0; e.atk = -1;
            sfx(Sfx::Perilous, e.pos, 0.6f, 1.6f);
            return HitResult::Broken;
        }
        return HitResult::Blocked;
    }
    e.blocksInRow = 0;
    float dmg = a.damage;
    if (!wasAware && !e.boss) dmg *= 2.0f;   // ambush bonus
    e.hp -= dmg;
    e.posture += a.posture * (1 + stats.postureDmg) * 0.65f;
    e.postureDelay = 1.2f;
    e.flash = 1;
    e.hitsTaken++;
    if (a.ember) e.burn = 2.0f;
    addStyle(a.projectile ? SProjectile : SHit, 14);
    sfx(a.ember ? Sfx::Ember : Sfx::Hit, contact, 0.9f);
    inkBurst(contact, Vec2{-e.facing * (facingPlayer ? 1.f : -1.f), -0.2f}, a.projectile ? 6 : 12);
    hitStop(a.projectile ? 0.02f : 0.05f);
    shake(0.12f);
    input->rumble(0.2f, 0.3f, 60);
    gainKi(5);
    if (!e.boss && e.st != EState::Broken && (e.st != EState::Windup || !(e.atk >= 0 && e.def->attacks[e.atk].perilous))) {
        e.st = EState::Stunned; e.stT = 0; e.atk = -1;
        e.vel.x = (player.pos.x < e.pos.x ? 1.f : -1.f) * (a.projectile ? 60.f : 140.f);
    }
    if (e.hp <= 0) {
        if (e.boss && e.deathblows > 1) {
            e.hp = 1; e.posture = e.def->posture;
            e.st = EState::Broken; e.stT = 0; e.atk = -1;
            return HitResult::Broken;
        }
        killEnemy(idx, false, false);
        return HitResult::Killed;
    }
    if (e.posture >= e.def->posture) {
        e.posture = e.def->posture;
        e.st = EState::Broken; e.stT = 0; e.atk = -1;
        sfx(Sfx::Perilous, e.pos, 0.6f, 1.6f);
        return HitResult::Broken;
    }
    return HitResult::Hit;
}

void World::killEnemy(int idx, bool execution, bool stealth) {
    Enemy& e = enemies[idx];
    if (e.boss && e.deathblows > 1) {
        // a deathblow that does not end it: the champion kneels, then rises changed
        e.deathblows--;
        e.hp = e.def->hp * e.hpScale;
        e.posture = 0;
        e.st = EState::Kneel; e.stT = 0; e.atk = -1;
        e.phase++;
        e.phase2 = true;
        if (e.hasToken) { e.hasToken = false; tokensInUse--; }
        for (auto& o : enemies) if (o.summoned && o.alive()) { o.hp = 0; o.st = EState::Dead; o.stT = 0; }   // the pack scatters
        event(WorldEvent::Cutscene, "boss_phase" + std::to_string(e.phase));
        return;
    }
    e.st = EState::Dead; e.stT = 0; e.hp = 0;
    if (e.boss && e.def->spared) {
        // the final blow is turned aside: the champion yields and lives
        e.yielded = true;
        sfx(Sfx::Clang, e.pos + Vec2{0, -60}, 1.0f, 0.7f);
        emit(PType::Spark, e.pos + Vec2{0, -60}, 30, 700, rgb(0xfff2c0), 3, 0.4f);
        slowmo = 0.9f; slowmoScale = 0.3f;
    } else {
        Vec2 c = e.pos + Vec2{0, -e.h() * 0.55f};
        inkBurst(c, Vec2{-e.facing, -0.4f}, 26);
        sfx(Sfx::Kill, c, 1.0f);
    }
    addStyle(execution ? SExecute : (stealth ? SStealth : SKill), execution ? 90.f : (stealth ? 80.f : 30.f));
    if (e.hasToken) { e.hasToken = false; tokensInUse--; }
    if (!e.summoned) save->bump("kills");
    gainEssence(e.summoned ? e.def->essence / 3 : e.def->essence + (execution ? 10 : 0) + (stealth ? 10 : 0));
    // a pouch of mon drops
    if (e.def->mon > 0 && !e.summoned) {
        Pickup p; p.kind = "mon"; p.pos = e.pos - Vec2{0, 20}; p.amount = e.def->mon + g_fxRng.irange(0, e.def->mon / 2);
        p.spawnId = ""; p.bob = g_fxRng.uniform() * 6;
        pickups.push_back(p);
    }
    if (e.boss) {
        bossActive = false;
        const EDef& D = *e.def;
        if (!D.flag.empty()) save->flags.insert(D.flag);
        if (!D.achievement.empty()) achieve(D.achievement);
        if (!player.tookDamageInBoss) achieve("boss_nodamage");
        if (!D.reward.empty()) save->add(D.reward);
        for (auto& o : enemies) if (o.summoned && o.alive()) { o.hp = 0; o.st = EState::Dead; o.stT = 0; }
        for (auto& m : mechs) if (m.kind == MechKind::Gate && m.id == "boss_door") m.active = true;
        event(WorldEvent::BossDefeated, D.name);
    }
}

void World::activateLink(const std::string& link) {
    if (link.empty()) return;
    save->flags.insert("open:" + link);
    for (auto& m : mechs) {
        if (m.kind == MechKind::Drain && m.id == link && !m.active) {
            m.active = true;
            floodRect(m.area, m.text == "fill" ? 0 : -1);
            if (m.text != "fill") achieve("sluice_puzzle");
        }
        if ((m.kind == MechKind::Gate || m.kind == MechKind::Bridge) && m.id == link && !m.active) {
            m.active = true;
            sfx(m.kind == MechKind::Gate ? Sfx::Gate : Sfx::Break, m.pos, 1.0f);
            shake(0.3f);
            if (link == "bellgate") { achieve("bell_puzzle"); event(WorldEvent::Cutscene, "bell_rung"); }
            else if (m.kind == MechKind::Gate) event(WorldEvent::Cutscene, "opened_" + link);
            if (m.kind == MechKind::Bridge) achieve("bridge_puzzle");
        }
    }
}

void World::startBoss() {
    if (bossIdx < 0 || bossActive) return;
    Enemy& b = enemies[bossIdx];
    if (!b.alive()) return;
    bossActive = true;
    player.tookDamageInBoss = false;
    playerDamageTakenBoss = 0;
    b.st = EState::Engage; b.awareness = 1; b.stT = 0; b.cooldown = 1.0f;
    for (auto& m : mechs) if (m.kind == MechKind::Gate && m.id == "boss_door") { m.active = false; m.t = 0; }
    event(WorldEvent::BossStart, b.def->name, b.def->kanji);
}

// =================================================================== targeting helpers

int World::findExecutable() const {
    int best = -1; float bd = 1e9f;
    for (size_t i = 0; i < enemies.size(); i++) {
        const Enemy& e = enemies[i];
        if (e.st != EState::Broken) continue;
        float dx = e.pos.x - player.pos.x, dy = std::abs(e.pos.y - player.pos.y);
        if (std::abs(dx) < 120 && dy < 70 && std::abs(dx) < bd) { bd = std::abs(dx); best = (int)i; }
    }
    return best;
}

int World::findStealthTarget() const {
    const Player& p = player;
    for (size_t i = 0; i < enemies.size(); i++) {
        const Enemy& e = enemies[i];
        if (!e.alive() || e.boss || e.awareness >= 1 || e.st == EState::Engage) continue;
        float dx = p.pos.x - e.pos.x, dy = p.pos.y - e.pos.y;
        bool behind = dx * e.facing < 0 && std::abs(dx) < 85 && std::abs(dy) < 40;
        bool above = !p.onGround && p.vel.y > 0 && std::abs(dx) < 70 && dy < -20 && dy > -260;
        if (behind || above || e.sleeping) {
            if (e.sleeping && !(std::abs(dx) < 85 && std::abs(dy) < 40) && !above) continue;
            return (int)i;
        }
    }
    return -1;
}

int World::findAnchor() const {
    float range = 400.f * (save->skills.count("veil_thread") ? 1.3f : 1.f);
    int best = -1; float bestScore = 1e9f;
    Vec2 from = player.pos + Vec2{0, -60};
    bool aim = input && input->aiming();
    Vec2 aimDir = aim ? Vec2{input->aimX(), input->aimY()}.norm() : Vec2{0, 0};
    for (size_t i = 0; i < anchors.size(); i++) {
        Vec2 d = anchors[i] - from;
        float L = d.len();
        if (L > range || d.y > -30) continue;
        // with the right stick, the ring you point at wins; otherwise the nearest one ahead
        float score = aim ? (1 - dot(d / L, aimDir)) * 900 + L * 0.2f : L - d.x * player.facing * 0.6f;
        if (score < bestScore && lineOfSight(from, anchors[i])) { bestScore = score; best = (int)i; }
    }
    return best;
}

Npc* World::npc(const std::string& id) {
    for (auto& n : npcs) if (n.id == id) return &n;
    return nullptr;
}
Mechanism* World::mech(const std::string& id) {
    for (auto& m : mechs) if (m.id == id) return &m;
    return nullptr;
}

// =================================================================== main update

void World::update(float dtIn) {
    float dt = dtIn;
    time += dt;
    if (hitstop > 0) {
        hitstop -= dt;
        updateCamera(dt);
        return;
    }
    if (slowmo > 0) { slowmo -= dt; dt *= slowmoScale; if (slowmo <= 0) { slowmoScale = 1; if (audio) audio->setLowpass(0); } }
    flash = std::max(0.f, flash - dtIn * 2.5f);
    chroma = std::max(0.f, chroma - dtIn * 3.f);
    desat = std::max(0.f, desat - dtIn * 1.2f);
    ripple = std::max(0.f, ripple - dtIn * 1.5f);
    interactCooldown = std::max(0.f, interactCooldown - dt);

    updatePlayer(dt);
    updateEnemies(dt);
    updateTrial(dt);
    // Kage Rank decays when you stop expressing yourself
    styleIdle += dt;
    stylePop = std::max(0.f, stylePop - dtIn * 2.5f);
    if (styleIdle > 2.5f && style > 0) {
        style = std::max(0.f, style - (10.f + 9.f * styleRank) * dt);
        while (styleRank > 0 && style < rankThreshold(styleRank)) styleRank--;
    }
    updateProjectiles(dt);
    updateMechs(dt);
    updateEnvironment(dt);
    updateParticles(dt);
    updateTriggers();
    updateCamera(dtIn);
    updateAmbient(dt);
    for (auto& n : npcs) {
        n.prevPos = n.pos;
        n.t += dt;
        if (n.walking) {
            float d = n.walkTo - n.pos.x;
            if (std::abs(d) < 3) { n.walking = false; n.pose = "idle"; }
            else { n.facing = sign(d); n.pos.x += sign(d) * std::min(std::abs(d), n.walkSpeed * dt); }
        }
        n.cloth.update(n.pos + Vec2{-n.facing * 6, -62}, {0, 700}, palette.wind * 3, dt, 0.97f, n.pos.y - 2);
    }
}

void World::updateTriggers() {
    Rect pb = playerBox();
    for (size_t i = 0; i < level.triggers.size(); i++) {
        auto& t = level.triggers[i];
        bool inside = t.area.overlaps(pb);
        if (!inside || triggerFired[i]) continue;
        if (player.state == PState::Dead) continue;
        triggerFired[i] = true;
        if (t.once) save->flags.insert("t:" + t.id);
        auto tok = tokenize(t.action);
        if (tok.empty()) continue;
        const std::string& a = tok[0];
        std::string arg = tok.size() > 1 ? tok[1] : "";
        if (a == "cutscene") event(WorldEvent::Cutscene, arg);
        else if (a == "hint") event(WorldEvent::Hint, arg);
        else if (a == "boss") startBoss();
        else if (a == "music") {
            hasMusicOverride = arg != "none";
            if (arg == "tension") musicOverride = MusicMode::Tension;
            else if (arg == "elegy") musicOverride = MusicMode::Requiem;
            else if (arg == "silent") musicOverride = MusicMode::Silent;
            else if (arg == "camp") musicOverride = MusicMode::Shrine;
        }
        else if (a == "flag") save->flags.insert(arg);
        else if (a == "grove_enter" || a == "unseen_enter") { groveSpotted = false; save->flags.insert("grove_tracking"); }
        else if (a == "grove_exit" || a == "unseen_exit") {
            if (save->has("grove_tracking") && !groveSpotted) achieve(a == "grove_exit" ? "grove_unseen" : (arg.empty() ? "grove_unseen" : arg));
            save->flags.erase("grove_tracking");
        }
        else if (a == "chase") {
            chaseActive = true; chaseSpeed = arg.empty() ? 380.f : std::stof(arg);
            chaseX = player.pos.x - 900; chaseHit = false;
        }
        else if (a == "chase_stop") { if (chaseActive && !chaseHit) achieve("outrun"); chaseActive = false; }
        else if (a == "flood") activateLink(arg);
        else if (a == "toast") event(WorldEvent::Toast, arg, tok.size() > 2 ? tok[2] : "");
        else if (a == "chapter_end") event(WorldEvent::ChapterEnd);
        if (!t.once) triggerFired[i] = false;
        if (!t.once && inside) triggerFired[i] = true;  // re-arm on exit (handled below)
    }
    // re-arm repeatable triggers when the player leaves them
    for (size_t i = 0; i < level.triggers.size(); i++)
        if (!level.triggers[i].once && triggerFired[i] && !level.triggers[i].area.overlaps(pb)) triggerFired[i] = false;
}

void World::updateCamera(float dt) {
    cam.prevPos = cam.pos;
    const Player& p = player;
    Room& room = level.rooms[level.roomAt(cam.pos.x)];
    // zoom & palette follow the room under the camera, blended near boundaries
    int ri = level.roomAt(cam.pos.x);
    curRoom = ri;
    const Room& R = level.rooms[ri];
    float bx0 = R.x0 * TILE, bx1 = (R.x0 + R.w) * TILE;
    const float blendW = 12 * TILE;
    Palette pal = paletteByName(R.palette);
    float zoomT = R.zoom;
    if (ri > 0 && cam.pos.x < bx0 + blendW * 0.5f) {
        float t = 0.5f + (cam.pos.x - bx0) / blendW;
        auto& P = level.rooms[ri - 1];
        pal = lerp(paletteByName(P.palette), pal, smoothstep(0, 1, t));
        zoomT = lerp(P.zoom, zoomT, smoothstep(0, 1, t));
    } else if (ri + 1 < (int)level.rooms.size() && cam.pos.x > bx1 - blendW * 0.5f) {
        float t = 0.5f - (bx1 - cam.pos.x) / blendW;
        auto& N = level.rooms[ri + 1];
        pal = lerp(pal, paletteByName(N.palette), smoothstep(0, 1, t));
        zoomT = lerp(zoomT, N.zoom, smoothstep(0, 1, t));
    }
    palette = pal;
    (void)room;

    if (cam.cinematic) {
        if (cam.cinDur > 0) {
            cam.cinT = std::min(cam.cinDur, cam.cinT + dt);
            float t = ease::inOutCubic(cam.cinT / cam.cinDur);
            cam.pos = lerp(cam.cinFrom, cam.cinTo, t);
            cam.zoom = lerp(cam.cinZoomFrom, cam.cinZoomTo, t);
        } else { cam.pos = cam.cinTo; cam.zoom = cam.cinZoomTo; }
    } else {
        float moving = std::abs(p.vel.x) > 60 ? p.facing : 0;
        cam.lookAhead = damp(cam.lookAhead, moving * 150.f, 1.6f, dt);
        Vec2 tgt = p.pos + Vec2{cam.lookAhead, -185};
        // right-stick camera peek (when not winding up a throw)
        Vec2 peekT = (input && !input->held(Act::Shuriken) && !controlLocked) ? Vec2{input->aimX() * 320, input->aimY() * 240} : Vec2{0, 0};
        cam.peek = damp(cam.peek, peekT, 4.f, dt);
        tgt += cam.peek;
        if (p.state == PState::Grapple) tgt = lerp(tgt, p.anchor, 0.25f);
        // keep the boss in frame
        if (bossActive && bossIdx >= 0) tgt.x = lerp(tgt.x, enemies[bossIdx].pos.x, 0.3f);
        cam.target = tgt;
        float yRate = p.vel.y > 600 ? 7.f : 3.2f;
        cam.pos.x = damp(cam.pos.x, tgt.x, 5.0f, dt);
        cam.pos.y = damp(cam.pos.y, tgt.y, yRate, dt);
        cam.targetZoom = zoomT * (bossActive ? 0.9f : 1.f) * cam.zoomBias;
        cam.zoom = damp(cam.zoom, cam.targetZoom + cam.punch, 4.f, dt);
        // bounds
        float vh = 1080.f / (cam.zoom * kBaseZoom), vw = vh * 16.f / 9.f;
        float minX = vw / 2, maxX = level.pixelWidth() - vw / 2;
        if (bossActive) { minX = std::max(minX, bx0 + vw / 2); maxX = std::min(maxX, bx1 - vw / 2); }
        if (minX < maxX) cam.pos.x = clampf(cam.pos.x, minX, maxX);
        cam.pos.y = clampf(cam.pos.y, vh / 2, level.pixelHeight() - vh / 2);
    }
    cam.punch = damp(cam.punch, 0.f, 6.f, dt);
    // trauma-based shake
    cam.trauma = std::max(0.f, cam.trauma - dt * 1.4f);
    float sh = cam.trauma * cam.trauma * (settings ? settings->shake : 1.f);
    cam.shakeOffset = {(noise1(time * 28, 1) - 0.5f) * 2 * 26 * sh, (noise1(time * 28, 2) - 0.5f) * 2 * 20 * sh};
    cam.shakeRot = (noise1(time * 20, 3) - 0.5f) * 0.03f * sh;
}

void World::updateAmbient(float dt) {
    if (!audio) return;
    const Room& R = level.rooms[curRoom];
    AmbienceParams amb;
    amb.wind = R.wind; amb.fire = R.fire; amb.crickets = R.crickets;
    amb.rumble = bossActive ? 0.15f : (chaseActive ? 0.25f : 0.f);
    amb.rain = std::max(R.rain, palette.rain);
    amb.water = R.water;
    // standing in water, the river is all around you
    if (player.inWater) amb.water = std::max(amb.water, 0.6f);
    audio->setAmbience(amb);
    // combat music follows the threat level
    int alerted = 0, suspicious = 0;
    for (auto& e : enemies) {
        if (!e.alive()) continue;
        float d = std::abs(e.pos.x - player.pos.x);
        if (e.awareness >= 1 && d < 1100) alerted++;
        else if (e.awareness > 0.3f && d < 900) suspicious++;
    }
    combatIntensity = damp(combatIntensity, clamp01(alerted / 3.f + (alerted ? 0.3f : 0)), 2.f, dt);
    MusicMode m;
    if (bossActive) m = MusicMode::Boss;
    else if (hasMusicOverride) m = musicOverride;
    else if (alerted > 0) m = MusicMode::Combat;
    else if (suspicious > 0) m = MusicMode::Tension;
    else if (R.music == "title") m = MusicMode::Title;
    else if (R.music == "camp") m = MusicMode::Shrine;
    else if (R.music == "tension") m = MusicMode::Tension;
    else if (R.music == "silent") m = MusicMode::Silent;
    else m = MusicMode::Explore;
    audio->setMusic(m);
    audio->setIntensity(bossActive ? 1.f : combatIntensity);
}

// =================================================================== projectiles

void World::updateProjectiles(float dt) {
    for (auto& pr : projectiles) {
        if (pr.dead) continue;
        pr.prevPos = pr.pos;
        pr.life -= dt;
        if (pr.life <= 0) { pr.dead = true; continue; }
        if (pr.stuck) continue;
        if (pr.type == ProjType::Arrow) pr.vel.y += 300 * dt;
        pr.pos += pr.vel * dt;
        pr.rot += dt * (pr.type == ProjType::Shuriken ? 30.f : 0.f);
        int tx = (int)std::floor(pr.pos.x / TILE), ty = (int)std::floor(pr.pos.y / TILE);
        bool wall = level.solidAt(tx, ty);
        bool wave = pr.type == ProjType::FlameWave || pr.type == ProjType::TideWave || pr.type == ProjType::ThreadWave;
        if (wave) {
            // hugs the ground; dies at walls
            if (wall) pr.dead = true;
            if (pr.type == ProjType::FlameWave && g_fxRng.chance(0.6f)) emit(PType::Ember, pr.pos + Vec2{g_fxRng.range(-20, 20), -g_fxRng.range(0, 60)}, 1, 160, rgb(0xff7a2a), 4, 0.6f, 1.0f, -PI / 2);
            if (pr.type == ProjType::TideWave && g_fxRng.chance(0.8f)) emit(PType::Splash, pr.pos + Vec2{g_fxRng.range(-30, 10), -g_fxRng.range(0, 80)}, 1, 220, rgb(0xa8e8f0, 0.8f), 4, 0.6f, 1.0f, -PI / 2);
            if (pr.type == ProjType::ThreadWave && g_fxRng.chance(0.7f)) emit(PType::Thread, pr.pos + Vec2{g_fxRng.range(-20, 20), -g_fxRng.range(0, 90)}, 1, 120, palette.thread, 2.5f, 0.5f, 1.0f, -PI / 2);
        } else if (wall) {
            if (pr.type == ProjType::Shuriken || pr.type == ProjType::Arrow || pr.type == ProjType::Kunai) {
                pr.stuck = true; pr.life = std::min(pr.life, 1.5f);
                sfx(Sfx::ShurikenHit, pr.pos, 0.5f);
                emit(PType::Spark, pr.pos, 5, 300, rgb(0xffe0a0), 2, 0.2f);
                // breakable tiles crack under shuriken? no: only blades break fences
            } else if (pr.type == ProjType::Gale) { pr.dead = true; emit(PType::Thread, pr.pos, 12, 260, rgb(0x9ff0e0), 2, 0.4f); }
            else if (pr.type == ProjType::Ball) { pr.dead = true; sfx(Sfx::ShurikenHit, pr.pos, 0.6f, 0.6f); emit(PType::Dust, pr.pos, 10, 200, palette.fog, 10, 0.6f); }
            else if (pr.type == ProjType::Snare) { pr.dead = true; emit(PType::Thread, pr.pos, 12, 200, palette.thread, 2, 0.4f); }
            else { pr.dead = true; emit(PType::Ember, pr.pos, 14, 300, rgb(0xff8a3a), 5, 0.5f); }
            continue;
        }
        if (pr.type == ProjType::Gale && g_fxRng.chance(0.7f))
            emit(PType::Thread, pr.pos + Vec2{g_fxRng.range(-10, 10), g_fxRng.range(-26, 26)}, 1, 60, rgb(0x9ff0e0), 2, 0.35f);
        if (pr.type == ProjType::Ember) {
            if (g_fxRng.chance(0.8f)) emit(PType::Ember, pr.pos + Vec2{g_fxRng.range(-10, 10), g_fxRng.range(-30, 30)}, 1, 80, rgb(0xff9a40), 5, 0.5f);
        }
        if (pr.type == ProjType::Snare && g_fxRng.chance(0.6f)) emit(PType::Thread, pr.pos, 1, 40, palette.thread, 2, 0.3f);
        if (pr.type == ProjType::Ball && g_fxRng.chance(0.5f)) emit(PType::Smoke, pr.pos, 1, 10, rgb(0xb8b8c0, 0.25f), 6, 0.4f);
        if (pr.fromPlayer || pr.reflected) {
            // beacons and ropes respond to shuriken
            for (auto& m : mechs) {
                if (m.kind == MechKind::Beacon && m.active) {
                    Rect r{m.pos.x - 30, m.pos.y - 90, 60, 90};
                    if (r.contains(pr.pos)) {
                        m.active = false; pr.dead = true;
                        sfx(Sfx::ShurikenHit, m.pos, 0.9f, 0.8f);
                        emit(PType::Smoke, m.pos + Vec2{0, -70}, 20, 120, rgb(0x404048, 0.6f), 16, 1.4f, 1.0f, -PI / 2);
                        save->bump("beacons_doused");
                        if (save->counter("beacons_doused") >= 6) achieve("douse_beacons");
                    }
                }
            }
            for (auto& m : mechs) {
                if (m.kind == MechKind::Bell && !m.active) {
                    Rect r{m.pos.x - 28, m.pos.y - 10, 56, 70};
                    if (r.contains(pr.pos)) {
                        m.active = true; m.swingVel = sign(pr.vel.x) * 6;
                        sfx(Sfx::Catch, m.pos, 1.0f);
                        emit(PType::Spark, pr.pos, 8, 300, rgb(0xffe0a0), 2, 0.3f);
                        pr.dead = true;
                        activateLink(m.link);
                    }
                }
                if (m.kind == MechKind::Rope && !m.active) {
                    Rect r{m.pos.x - 12, m.pos.y - m.h * TILE, 24, m.h * TILE};
                    if (r.contains(pr.pos)) { m.active = true; pr.dead = true; sfx(Sfx::Slash, m.pos, 0.8f, 1.3f); activateLink(m.link); }
                }
            }
            for (size_t i = 0; i < enemies.size(); i++) {
                Enemy& e = enemies[i];
                if (!e.alive() || std::find(pr.hits.begin(), pr.hits.end(), (int)i) != pr.hits.end()) continue;
                if (!e.box().contains(pr.pos)) continue;
                pr.hits.push_back((int)i);
                AttackInfo a; a.damage = pr.damage; a.posture = pr.posture; a.projectile = true; a.ember = pr.type == ProjType::Ember; a.origin = pr.pos - pr.vel * 0.1f;
                // shuriken interrupt archers and wind-ups
                hitEnemy((int)i, a);
                if (pr.type == ProjType::Shuriken && e.alive() && (e.def->ranged || e.st == EState::Windup) && !e.boss) { e.st = EState::Stunned; e.stT = 0; e.atk = -1; }
                if (pr.type != ProjType::Ember && pr.type != ProjType::Gale) { pr.dead = true; break; }
            }
        } else {
            Rect pb = playerBox();
            Rect hit = wave ? Rect{pr.pos.x - 22, pr.pos.y - (pr.type == ProjType::TideWave ? 80.f : 50.f), 44, pr.type == ProjType::TideWave ? 80.f : 50.f}
                            : Rect{pr.pos.x - 6, pr.pos.y - 6, 12, 12};
            // a decoy absorbs the first bolt that finds it
            if (decoyT > 0 && !wave && Rect{decoyPos.x - 18, decoyPos.y - 96, 36, 96}.contains(pr.pos)) { pr.dead = true; decoyT = 0; emit(PType::Thread, decoyPos + Vec2{0, -50}, 20, 300, rgb(0xe8dcff), 2.5f, 0.5f); continue; }
            if (hit.overlaps(pb)) {
                AttackInfo a; a.damage = pr.damage; a.posture = pr.posture; a.projectile = true; a.origin = pr.pos - pr.vel;
                a.perilous = wave || pr.type == ProjType::Ball;
                HitResult r = hitPlayer(a);
                if (r == HitResult::Hit && pr.type == ProjType::Snare) { player.snared = 1.4f; sfx(Sfx::Grapple, player.pos, 0.7f, 1.4f); }
                if (r == HitResult::Hit && pr.type == ProjType::TideWave) player.vel.x += sign(pr.vel.x) * 380;
                if (r == HitResult::Deflected && (pr.type == ProjType::Arrow || pr.type == ProjType::Kunai || pr.type == ProjType::Snare)) {
                    pr.vel = Vec2{-pr.vel.x * 1.2f, -std::abs(pr.vel.y) * 0.3f - 60};
                    pr.reflected = true; pr.fromPlayer = true; pr.damage *= 2.0f; pr.hits.clear();
                    pr.life = 2;
                } else if (r == HitResult::Blocked) pr.dead = true;
                else if (r == HitResult::Hit) pr.dead = !wave;
            }
        }
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return p.dead; }), projectiles.end());
}

// =================================================================== mechanisms & pickups

void World::updateMechs(float dt) {
    for (auto& m : mechs) {
        switch (m.kind) {
        case MechKind::Gate:
            if (m.active) m.t = std::min(1.f, m.t + dt / 2.0f);
            else m.t = std::max(0.f, m.t - dt / 0.6f);
            break;
        case MechKind::Bridge:
            if (m.active && m.t < 1) {
                m.t = std::min(1.f, m.t + dt * (0.4f + m.t * 2.5f));
                if (m.t >= 1) { sfx(Sfx::Thud, m.pos + Vec2{m.w * TILE * 0.5f, 0}, 1.0f); shake(0.5f); emit(PType::Dust, m.pos + Vec2{m.w * TILE, -6}, 20, 200, palette.fog, 18, 1.0f, PI, -PI / 2); }
            }
            break;
        case MechKind::Bell:
            m.swingVel += -m.swing * 30 * dt;
            m.swingVel *= (1 - 1.2f * dt);
            m.swing += m.swingVel * dt;
            break;
        case MechKind::Chest: if (m.active) m.t = std::min(1.f, m.t + dt * 2.5f); break;
        case MechKind::Sluice: if (m.active) m.t = std::min(1.f, m.t + dt * 0.8f); break;
        default: break;
        }
    }
    // pickups
    Rect pb = playerBox();
    for (auto& p : pickups) {
        if (p.taken) continue;
        p.bob += dt;
        Rect r{p.pos.x - 22, p.pos.y - 22, 44, 44};
        if (!r.overlaps(pb) || player.state == PState::Dead) continue;
        p.taken = true;
        if (!p.spawnId.empty()) save->flags.insert(p.spawnId);
        if (p.kind == "mon") { save->mon += p.amount; sfx(Sfx::Pickup, p.pos, 0.5f, 1.4f); }
        else if (p.kind == "shuriken") {
            save->shuriken = std::min(save->maxShuriken(), save->shuriken + p.amount);
            sfx(Sfx::Pickup, p.pos, 0.6f, 1.2f);
            event(WorldEvent::Toast, strfmt("Shuriken +%d", p.amount));
        } else if (p.kind == "bead") {
            save->beads++;
            sfx(Sfx::Pickup, p.pos, 0.8f, 0.9f);
            event(WorldEvent::Toast, "Resin Bead", save->beads % 4 == 0 ? "Four beads: maximum vitality increased." : strfmt("%d of 4 for a stronger body.", save->beads % 4));
            stats = computeStats(*save);
        } else if (p.kind == "material") {
            save->add(p.item, p.amount);
            sfx(Sfx::Pickup, p.pos, 0.7f);
            const ItemDef* d = findItem(p.item);
            event(WorldEvent::Toast, d ? d->name : p.item, "Material");
        }
        emit(PType::Ember, p.pos, 12, 200, rgb(0xffd27a), 3, 0.6f);
    }
    pickups.erase(std::remove_if(pickups.begin(), pickups.end(), [](const Pickup& p) { return p.taken; }), pickups.end());
}

// =================================================================== particles

void World::updateParticles(float dt) {
    // ambient emitters tied to the palette
    float vh = 1080.f / (cam.zoom * kBaseZoom), vw = vh * 16.f / 9.f * 1.3f;
    Rect view{cam.pos.x - vw / 2, cam.pos.y - vh / 2, vw, vh};
    auto spawnRate = [&](float rate) { float n = rate * dt; int k = (int)n; if (g_fxRng.uniform() < n - k) k++; return k; };
    for (int i = spawnRate(palette.embers * 14); i > 0; i--) {
        Particle p; p.type = PType::Ember; p.pos = {g_fxRng.range(view.x, view.right()), view.bottom() + 20};
        p.vel = {g_fxRng.range(-20, 40) + palette.wind.x * 0.5f, g_fxRng.range(-160, -60)};
        p.life = p.maxLife = g_fxRng.range(3, 6); p.size = g_fxRng.range(1.5f, 3.5f);
        p.col = lerp(rgb(0xffb04a), rgb(0xff4a1a), g_fxRng.uniform()); p.drag = 0.2f; p.gravity = -10;
        particles.push_back(p);
    }
    for (int i = spawnRate(palette.ash * 18); i > 0; i--) {
        Particle p; p.type = PType::Ash; p.pos = {g_fxRng.range(view.x, view.right()), view.y - 10};
        p.vel = {g_fxRng.range(-10, 20) + palette.wind.x * 0.6f, g_fxRng.range(30, 70)};
        p.life = p.maxLife = g_fxRng.range(5, 9); p.size = g_fxRng.range(1.2f, 2.6f);
        p.col = rgb(0x8a8078, 0.55f); p.rotVel = g_fxRng.range(-3, 3); p.fg = g_fxRng.chance(0.3f);
        particles.push_back(p);
    }
    for (int i = spawnRate(palette.leaves * 5); i > 0; i--) {
        Particle p; p.type = PType::Leaf; p.pos = {g_fxRng.range(view.x, view.right()), view.y - 10};
        p.vel = {g_fxRng.range(10, 60) + palette.wind.x * 0.5f, g_fxRng.range(40, 90)};
        p.life = p.maxLife = g_fxRng.range(6, 10); p.size = g_fxRng.range(5, 9);
        p.col = palette.nearC; p.rot = g_fxRng.range(0, TAU); p.rotVel = g_fxRng.range(-2, 2); p.fg = g_fxRng.chance(0.25f);
        particles.push_back(p);
    }
    for (int i = spawnRate(palette.petals * 8); i > 0; i--) {
        Particle p; p.type = PType::Petal; p.pos = {g_fxRng.range(view.x, view.right()), view.y - 10};
        p.vel = {g_fxRng.range(20, 80) + palette.wind.x * 0.6f, g_fxRng.range(40, 100)};
        p.life = p.maxLife = g_fxRng.range(6, 10); p.size = g_fxRng.range(5, 8);
        p.col = lerp(rgb(0xb3121f), rgb(0x6e0a12), g_fxRng.uniform()); p.rot = g_fxRng.range(0, TAU); p.rotVel = g_fxRng.range(-3, 3);
        p.fg = g_fxRng.chance(0.3f);
        particles.push_back(p);
    }
    for (int i = spawnRate(palette.fireflies * 3); i > 0; i--) {
        Particle p; p.type = PType::Firefly; p.pos = {g_fxRng.range(view.x, view.right()), g_fxRng.range(view.y + vh * 0.3f, view.bottom())};
        p.vel = {g_fxRng.range(-15, 15), g_fxRng.range(-15, 5)};
        p.life = p.maxLife = g_fxRng.range(4, 8); p.size = g_fxRng.range(2, 3.5f);
        p.col = rgb(0xd8ff9a); p.rot = g_fxRng.range(0, TAU);
        particles.push_back(p);
    }
    for (int i = spawnRate(palette.mist * 1.2f); i > 0; i--) {
        Particle p; p.type = PType::Mist; p.pos = {g_fxRng.range(view.x - 200, view.right()), cam.pos.y + g_fxRng.range(0, vh * 0.45f)};
        p.vel = {palette.wind.x * 0.4f + g_fxRng.range(-5, 15), 0};
        p.life = p.maxLife = g_fxRng.range(8, 14); p.size = g_fxRng.range(160, 320);
        p.col = palette.fog.withA(0.12f); p.fg = g_fxRng.chance(0.25f);
        particles.push_back(p);
    }
    for (auto& p : particles) {
        p.life -= dt;
        p.vel *= std::max(0.f, 1 - p.drag * dt);
        p.vel.y += p.gravity * dt;
        if (p.type == PType::Leaf || p.type == PType::Petal) {
            p.vel.x += std::sin(time * 2.3f + p.rot) * 60 * dt + palette.wind.x * 0.2f * dt;
            p.rot += p.rotVel * dt;
        } else if (p.type == PType::Firefly) {
            p.vel += Vec2{std::sin(time * 1.7f + p.rot * 3) * 30, std::cos(time * 1.3f + p.rot * 2) * 30} * dt;
        } else if (p.type == PType::Ash) {
            p.vel.x += std::sin(time + p.rot) * 10 * dt;
            p.rot += p.rotVel * dt;
        } else p.rot += p.rotVel * dt;
        p.pos += p.vel * dt;
        if (p.type == PType::Snow) p.vel.x += std::sin(time * 1.3f + p.pos.y * 0.01f) * 20 * dt;
        if (p.type == PType::Rain && p.life > 0) {
            int tx = (int)std::floor(p.pos.x / TILE), ty = (int)std::floor(p.pos.y / TILE);
            if (level.solidAt(tx, ty) || level.waterAt(tx, ty)) {
                p.life = 0;
                if (g_fxRng.chance(0.35f)) {
                    Particle q; q.type = PType::Splash; q.pos = {p.pos.x, ty * (float)TILE}; q.vel = {g_fxRng.range(-60, 60), g_fxRng.range(-160, -60)};
                    q.gravity = 900; q.life = q.maxLife = 0.25f; q.size = 2; q.col = p.col.withA(0.6f); q.fg = p.fg;
                    particles.push_back(q);
                }
            }
        }
        if (p.type == PType::Ink && level.solidAt((int)std::floor(p.pos.x / TILE), (int)std::floor(p.pos.y / TILE))) { p.vel = {0, 0}; p.gravity = 0; p.drag = 0; }
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0; }), particles.end());
    if (particles.size() > 4000) particles.erase(particles.begin(), particles.begin() + (particles.size() - 4000));
}

} // namespace sv
