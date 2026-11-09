#include "game/world.h"
#include "core/util.h"

namespace sv {

bool g_fxRngSeedTick(float t);

// ------------------------------------------------------------------ tuning

namespace tune {
constexpr float RUN = 440, CROUCH = 170, CROUCH_SOFT = 240, GUARD_WALK = 120;
constexpr float ACCEL = 4600, DECEL = 5600, AIR_ACCEL = 2800;
constexpr float GRAVITY = 2900, MAX_FALL = 1350, JUMP_V = 1030, WALL_JUMP_X = 540, WALL_JUMP_Y = 930, WALL_SLIDE = 170;
constexpr float COYOTE = 0.10f, JUMP_BUFFER = 0.13f, ATTACK_BUFFER = 0.22f;
constexpr float DASH_SPEED = 1180, DASH_TIME = 0.17f, DASH_CD = 0.30f;
constexpr float DEFLECT_WINDOW = 0.17f;
}

// Combo strings: windup, active, recover, damage multiplier, posture, lunge, poses
struct Swing { float windup, active, recover, dmg, posture, lunge; Rect box; };
static const Swing kCombo[4] = {
    {0.06f, 0.08f, 0.24f, 1.0f, 14, 230, {6, -86, 100, 72}},
    {0.06f, 0.08f, 0.26f, 1.0f, 14, 230, {6, -100, 96, 92}},
    {0.11f, 0.10f, 0.34f, 1.6f, 26, 300, {4, -118, 112, 116}},
    {0.08f, 0.10f, 0.36f, 1.3f, 20, 180, {0, -140, 92, 140}},
};
static const Swing kAir = {0.05f, 0.10f, 0.18f, 1.0f, 12, 0, {-10, -104, 116, 116}};

// ------------------------------------------------------------------ poses

namespace poses {
const Pose idle = P(0, 3, 8, -4, 25, 55, -10, 20, 10, -8, -14, -6, -20);
const Pose crouch = P(4, 24, 30, -12, 30, 60, 10, 40, 72, -118, 10, -100, -30);
const Pose jump = P(0, -2, 10, -6, -25, 60, -45, 55, 38, -75, -18, -35, 75);
const Pose fall = P(0, 0, 4, 4, -10, 50, -70, 40, 22, -24, -12, -48, 55);
const Pose wall = P(-2, 6, -12, 0, 30, 60, -100, -20, 42, -70, 12, -60, -10);
const Pose dash = P(6, 10, 48, -24, -70, 10, -55, 20, 55, -70, -55, -20, 95);
const Pose guard = P(-2, 9, 6, 4, 62, 82, 30, 70, 26, -22, -26, -10, 18);
const Pose deflect = P(2, 10, 14, 6, 78, 66, 30, 60, 34, -30, -30, -8, 8);
const Pose hurt = P(-6, 4, -26, 18, -30, 40, -60, 30, 20, -10, -30, -20, 60);
const Pose throwP = P(2, 6, 18, -4, 20, 40, 105, -5, 30, -20, -26, -10, -30);
const Pose cast = P(4, 8, 16, -4, 80, 10, 95, -5, 34, -26, -30, -8, -60);
const Pose heal = P(0, 4, -6, -18, 25, 45, 150, 125, 10, -8, -14, -6, -20);
const Pose grapple = P(0, -2, -6, -10, -30, 40, 165, 0, 20, -50, -20, -30, 90);
const Pose dead = P(-10, 34, 96, 10, 70, 10, 100, 20, -92, 0, -96, -6, 30);
const Pose kneel = P(-4, 30, 2, 8, 30, 32, 24, 36, 60, -150, 54, -146, -20);
const Pose stand = P(0, 1, 2, 0, 8, 10, -6, 12, 4, -2, -4, -2, -10);
const Pose lookUp = P(0, 1, -4, -20, 8, 10, -6, 12, 4, -2, -4, -2, -10);
// combo: windup / strike / follow
const Pose a1w = P(-2, 8, -6, -2, -50, 60, -20, 30, 30, -24, -30, -10, 62);
const Pose a1s = P(8, 11, 26, -8, 88, 8, -40, 30, 46, -46, -36, -6, 8);
const Pose a1f = P(6, 10, 20, -6, 118, 26, -40, 30, 42, -40, -34, -6, 26);
const Pose a2w = P(2, 10, 18, -4, 10, 20, 20, 30, 40, -40, -34, -8, -40);
const Pose a2s = P(6, 6, 6, -10, 150, 10, -30, 40, 34, -20, -30, -10, 20);
const Pose a2f = P(4, 6, 2, -12, 170, 20, -30, 40, 30, -16, -28, -10, 34);
const Pose a3w = P(-4, 4, -12, -10, 172, 22, 160, 30, 26, -16, -30, -10, 44);
const Pose a3s = P(10, 20, 38, 6, 70, 8, 60, 10, 58, -62, -40, -4, 18);
const Pose a3f = P(10, 20, 36, 6, 46, 8, 40, 10, 58, -62, -40, -4, 4);
const Pose a4w = P(0, 22, 30, -6, -10, 20, -20, 40, 70, -110, 10, -90, -70);
const Pose a4s = P(4, -4, -8, -16, 168, 4, 120, 20, 20, -10, -40, -20, 4);
const Pose airW = P(0, -2, -6, -6, 150, 20, -60, 40, 40, -90, -10, -70, 80);
const Pose airS = P(0, -2, 24, 0, 40, 10, -60, 40, 50, -100, 0, -80, 30);
const Pose exW = P(-6, 12, 6, 0, -40, 100, 40, 60, 34, -30, -34, -8, 0);
const Pose exS = P(14, 16, 38, 0, 92, 0, 70, 20, 60, -60, -46, -4, -2);
const Pose skW = P(2, 10, 20, 4, 30, 120, 80, 30, 34, -30, -30, -8, 60);
const Pose skS = P(4, 12, 30, 10, 40, 110, 70, 30, 40, -36, -34, -8, 100);
}

static Pose runPose(float ph, float speed01) {
    float s = std::sin(ph), c = std::cos(ph);
    float sb = std::sin(ph + PI), cb = std::cos(ph + PI);
    Pose p = poses::idle;
    p.hipY = 5 - 4 * std::abs(c) * speed01;
    p.lean = (12 + 14 * speed01) * DEG;
    p.head = -8 * DEG;
    p.legFU = (40 * s * speed01 + 6) * DEG;
    p.legFL = (-12 - 70 * std::max(0.f, c) * speed01) * DEG;
    p.legBU = (40 * sb * speed01 + 6) * DEG;
    p.legBL = (-12 - 70 * std::max(0.f, cb) * speed01) * DEG;
    p.armBU = (-38 * s * speed01) * DEG;
    p.armBL = (40 + 20 * speed01) * DEG;
    // sword hand trails behind at speed: classic shinobi run
    p.armFU = lerp(25.f, -38.f, speed01) * DEG + 6 * sb * DEG;
    p.armFL = lerp(55.f, 18.f, speed01) * DEG;
    p.sword = lerp(-20.f, 96.f, speed01) * DEG;
    return p;
}

static Pose sneakPose(float ph, float moving) {
    Pose p = poses::crouch;
    float s = std::sin(ph), c = std::cos(ph);
    p.legFU += (24 * s * moving) * DEG; p.legBU += (-24 * s * moving) * DEG;
    p.legFL += (-20 * std::max(0.f, c) * moving) * DEG; p.legBL += (-20 * std::max(0.f, -c) * moving) * DEG;
    p.hipY += 2 * std::abs(c) * moving;
    return p;
}

static void setState(Player& p, PState s) {
    p.state = s; p.stateT = 0;
    p.fromPose = p.pose; p.blend = 0;
    p.blendDur = (s == PState::Attack || s == PState::AirAttack || s == PState::Guard) ? 0.04f : 0.09f;
}

// ------------------------------------------------------------------ update

void World::updatePlayer(float dt) {
    Player& p = player;
    Input& in = *input;
    p.prevPos = p.pos;
    p.stateT += dt;
    p.iframes = std::max(0.f, p.iframes - dt);
    p.dashCd = std::max(0.f, p.dashCd - dt);
    p.deflectT = std::max(0.f, p.deflectT - dt);
    p.airAttackCd = std::max(0.f, p.airAttackCd - dt);
    p.wallJumpLock = std::max(0.f, p.wallJumpLock - dt);
    p.hurtFlash = std::max(0.f, p.hurtFlash - dt * 4);
    p.landT = std::max(0.f, p.landT - dt);
    p.strokeCd = std::max(0.f, p.strokeCd - dt);
    p.snared = std::max(0.f, p.snared - dt);
    prompt.clear();

    const bool locked = controlLocked;
    float mx = locked ? 0 : in.moveX();
    if (p.snared > 0) {
        // bound by a Hollow thread: struggle (mash Attack/Jump) to tear free
        mx *= 0.2f;
        if (!locked && (in.pressed(Act::Attack) || in.pressed(Act::Jump) || in.pressed(Act::Dash))) {
            p.snared -= 0.3f;
            in.consume(Act::Attack); in.consume(Act::Jump); in.consume(Act::Dash);
            emit(PType::Thread, p.pos + Vec2{0, -50}, 6, 200, palette.thread, 2, 0.3f);
            if (p.snared <= 0) sfx(Sfx::Slash, p.pos, 0.6f, 1.5f);
        }
    }
    auto pressed = [&](Act a) { return !locked && in.pressed(a); };
    auto held = [&](Act a) { return !locked && in.held(a); };
    auto buffered = [&](Act a, float w) { return !locked && in.buffered(a, w); };

    bool grounded = p.onGround;
    if (grounded) { p.coyote = tune::COYOTE; p.airDashes = save->skills.count("veil_second") ? 2 : 1; }
    else p.coyote -= dt;

    // ---------------------------------------------------------- global transitions
    auto canAct = [&]() {
        switch (p.state) {
        case PState::Ground: case PState::Air: case PState::WallSlide: case PState::Guard: return true;
        case PState::Attack: case PState::AirAttack: {
            // cancel windows: after the strike lands
            const Swing& sw = p.state == PState::Attack ? kCombo[p.combo] : kAir;
            return p.stateT > sw.windup + sw.active + 0.04f;
        }
        default: return false;
        }
    };

    if (p.state == PState::Dead) {
        p.deathT += dt;
        p.vel.x = damp(p.vel.x, 0.f, 6, dt);
        p.vel.y = std::min(tune::MAX_FALL, p.vel.y + tune::GRAVITY * dt);
        auto r = moveBody(p.pos, p.vel, p.w, p.h, dt);
        p.onGround = r.ground || onGroundAt(p.pos, p.w);
        if (p.deathT > 0.05f && p.deathT - dt <= 0.05f) {
            sfx(Sfx::Death, p.pos, 1.0f);
            slowmo = 1.2f; slowmoScale = 0.35f;
            if (audio) audio->setLowpass(0.7f);
            desat = 1.0f;
        }
        if (p.deathT > 2.4f && p.deathT - dt <= 2.4f) event(WorldEvent::Death);
        playerAnim(dt);
        return;
    }

    // spikes & falling out of the world
    {
        Rect feet{p.pos.x - p.w / 2 + 4, p.pos.y - 16, p.w - 8, 16};
        int x0 = (int)std::floor(feet.x / TILE), x1 = (int)std::floor(feet.right() / TILE);
        int ty = (int)std::floor((p.pos.y - 4) / TILE);
        bool spiked = false;
        for (int x = x0; x <= x1; x++) if (level.at(x, ty) == Tile::Spikes) spiked = true;
        if ((spiked && p.iframes <= 0) || p.pos.y > level.pixelHeight() + 200) {
            p.hp -= 20 * (1 - stats.defense);
            sfx(Sfx::Hurt, p.pos, 1.0f);
            inkBurst(p.pos + Vec2{0, -30}, {0, -1}, 10);
            shake(0.4f);
            if (p.hp <= 0) { p.hp = 0; setState(p, PState::Dead); return; }
            p.pos = p.prevPos = p.lastSafe;
            p.vel = {0, 0};
            p.iframes = 1.0f;
            setState(p, PState::Ground);
            flash = 0.3f; flashColor = rgb(0x000000);
        }
    }

    // execution / stealth kill prompts take priority on attack
    int execIdx = (!locked && canAct()) ? findExecutable() : -1;
    int stealthIdx = (!locked && canAct() && execIdx < 0) ? findStealthTarget() : -1;
    if (execIdx >= 0) { prompt = "{Attack}  Execute"; promptPos = enemies[execIdx].pos + Vec2{0, -enemies[execIdx].h() - 40}; }
    else if (stealthIdx >= 0) { prompt = "{Attack}  Assassinate"; promptPos = enemies[stealthIdx].pos + Vec2{0, -enemies[stealthIdx].h() - 40}; }

    // interactables
    Mechanism* near = nullptr;
    if (!locked && (p.state == PState::Ground) && execIdx < 0 && stealthIdx < 0) {
        float bd = 80;
        for (auto& m : mechs) {
            bool usable = (m.kind == MechKind::Chest && !m.active) || (m.kind == MechKind::Scroll && !m.active) || m.kind == MechKind::Shrine ||
                          m.kind == MechKind::Examine || m.kind == MechKind::Npc || (m.kind == MechKind::Trial && !m.active && !trialActive) ||
                          (m.kind == MechKind::Sluice && !m.active);
            if (!usable) continue;
            float d = std::abs(m.pos.x - p.pos.x);
            if (d < bd && std::abs(m.pos.y - p.pos.y) < 60) { bd = d; near = &m; }
        }
        if (near) {
            const char* verb = "Examine";
            switch (near->kind) {
            case MechKind::Chest: verb = "Open"; break;
            case MechKind::Scroll: verb = "Read"; break;
            case MechKind::Shrine: verb = "Rest"; break;
            case MechKind::Npc: verb = "Talk"; break;
            case MechKind::Trial: verb = "Begin Trial"; break;
            case MechKind::Sluice: verb = "Turn the wheel"; break;
            default: break;
            }
            prompt = std::string("{Interact}  ") + verb;
            promptPos = near->pos + Vec2{0, -130};
        }
    }

    if (canAct() && in.buffered(Act::Attack, tune::ATTACK_BUFFER) && !locked) {
        if (execIdx >= 0) {
            in.consume(Act::Attack);
            p.target = execIdx;
            Enemy& e = enemies[execIdx];
            e.st = EState::BeingKilled; e.stT = 0;
            p.facing = sign(e.pos.x - p.pos.x); if (p.facing == 0) p.facing = 1;
            setState(p, PState::Execute);
            p.vel = {0, 0};
            slowmo = 0.5f; slowmoScale = 0.3f;
            if (audio) audio->setLowpass(0.5f);
            cam.punch = 0.25f;
            sfx(Sfx::Execute, e.pos, 1.0f);
            desat = 0.8f;
            return;
        }
        if (stealthIdx >= 0) {
            in.consume(Act::Attack);
            p.target = stealthIdx;
            Enemy& e = enemies[stealthIdx];
            e.st = EState::BeingKilled; e.stT = 0;
            p.facing = e.facing;   // we strike from behind, facing the same way
            p.pos.x = e.pos.x - e.facing * 34;
            if (!p.onGround) p.pos.y = e.pos.y;
            p.vel = {0, 0};
            setState(p, PState::StealthKill);
            sfx(Sfx::Swoosh, p.pos, 0.7f);
            return;
        }
    }

    // ---------------------------------------------------------- per-state logic
    auto startAttack = [&]() {
        in.consume(Act::Attack);
        p.swingId++; p.swingHits.clear(); p.hitDone = false;
        p.trail.clear();
        if (p.onGround) {
            int maxCombo = save->skills.count("blade_fourth") ? 4 : 3;
            bool chaining = (p.state == PState::Attack) && p.combo + 1 < maxCombo;
            p.combo = chaining ? p.combo + 1 : 0;
            setState(p, PState::Attack);
        } else {
            setState(p, PState::AirAttack);
            p.airAttackCd = 0.32f;
        }
    };
    auto tryDash = [&]() {
        if (!buffered(Act::Dash, 0.1f) || p.dashCd > 0) return false;
        if (!p.onGround && p.airDashes <= 0) return false;
        in.consume(Act::Dash);
        if (!p.onGround) p.airDashes--;
        if (std::abs(mx) > 0.2f) p.facing = sign(mx);
        setState(p, PState::Dash);
        p.vel = {p.facing * tune::DASH_SPEED, 0};
        p.dashCd = tune::DASH_CD - stats.dashCooldown - (hasGale() ? 0.08f : 0.f);
        p.iframes = std::max(p.iframes, tune::DASH_TIME + 0.03f + (save->skills.count("veil_mantle") ? 0.05f : 0.f));
        sfx(Sfx::Dash, p.pos, 0.8f);
        ripple = 0.6f; rippleAt = p.pos + Vec2{0, -40};
        emit(PType::Thread, p.pos + Vec2{0, -40}, 10, 120, rgb(0xd8c8ff), 2, 0.5f);
        if (hasStance("mirror")) {
            // Mirror Step: the place you left keeps your shape a moment longer
            decoyPos = p.pos; decoyT = 1.6f;
        }
        // in the Mirror Province the step crosses between the Bright and the Hollow
        if (level.rooms[curRoom].layers && save->has("mirror_thread")) tryFlipLayer();
        return true;
    };
    auto tryJump = [&]() {
        if (!buffered(Act::Jump, tune::JUMP_BUFFER)) return false;
        if (p.onGround && held(Act::Down)) {
            // drop through one-way platforms
            int tx0 = (int)std::floor((p.pos.x - p.w / 2) / TILE), tx1 = (int)std::floor((p.pos.x + p.w / 2) / TILE);
            int ty = (int)std::floor((p.pos.y + 2) / TILE);
            bool oneway = false;
            for (int x = tx0; x <= tx1; x++) if (level.at(x, ty) == Tile::OneWay) oneway = true;
            if (oneway) { in.consume(Act::Jump); p.pos.y += 4; p.onGround = false; setState(p, PState::Air); return true; }
        }
        if (p.inWater) {
            if (!p.headUnder) {
                // kick out of the water
                in.consume(Act::Jump);
                p.vel.y = -tune::JUMP_V * 0.82f;
                setState(p, PState::Air);
                sfx(Sfx::Jump, p.pos, 0.6f, 0.8f);
                emit(PType::Splash, p.pos + Vec2{0, -40}, 16, 300, palette.water * 2.4f, 4, 0.6f, 1.2f, -PI / 2);
                return true;
            }
            if (p.strokeCd <= 0) {
                // a swimming stroke upward
                in.consume(Act::Jump);
                p.vel.y = std::min(p.vel.y, -430.f);
                p.strokeCd = 0.28f;
                sfx(Sfx::Swoosh, p.pos, 0.35f, 0.6f);
                emit(PType::Splash, p.pos + Vec2{0, -20}, 6, 90, palette.water * 2.6f, 3, 0.5f);
                return true;
            }
            return false;
        }
        if (p.coyote > 0) {
            in.consume(Act::Jump);
            p.vel.y = -tune::JUMP_V;
            p.coyote = 0; p.onGround = false;
            setState(p, PState::Air);
            sfx(Sfx::Jump, p.pos, 0.7f);
            emit(PType::Dust, p.pos, 6, 120, palette.fog.withA(0.5f), 10, 0.4f, PI, -PI / 2);
            return true;
        }
        return false;
    };
    auto tryGuard = [&]() {
        if (!held(Act::Guard)) return false;
        if (pressed(Act::Guard)) {
            float window = tune::DEFLECT_WINDOW * (1 + stats.deflectWindow) * (settings ? settings->deflectAssist : 1.f);
            if (time - p.lastGuardPress < 0.22f) window *= 0.5f;   // spam protection
            p.deflectT = window;
            p.lastGuardPress = time;
        }
        setState(p, PState::Guard);
        return true;
    };
    auto tryTools = [&]() {
        if (pressed(Act::Shuriken) && save->shuriken > 0) {
            save->shuriken--;
            setState(p, PState::Throw);
            // aim: right stick freely; keyboard leans the throw with Up/Down
            Vec2 dir{p.facing, 0};
            if (in.aiming()) dir = Vec2{in.aimX(), in.aimY()}.norm();
            else if (held(Act::Up)) dir = Vec2{p.facing, -0.8f}.norm();
            else if (held(Act::Down) && !p.onGround) dir = Vec2{p.facing, 0.8f}.norm();
            if (std::abs(dir.x) > 0.15f) p.facing = sign(dir.x);
            Projectile pr; pr.type = ProjType::Shuriken; pr.pos = pr.prevPos = p.pos + Vec2{dir.x * 20, -58 + dir.y * 20};
            pr.vel = dir * 1500; pr.damage = 6 + stats.attack * 0.3f; pr.posture = 6; pr.life = 1.2f;
            projectiles.push_back(pr);
            sfx(Sfx::Throw, p.pos, 0.8f);
            return true;
        }
        float cost = save->skills.count("veil_well") ? 25.f : 35.f;
        if (pressed(Act::Art) && save->has("ember") && !near) {
            if (p.ki >= cost) {
                p.ki -= cost;
                setState(p, PState::Cast);
                sfx(Sfx::Ignite, p.pos, 0.9f);
                return true;
            }
            event(WorldEvent::Toast, "Not enough Sparks", "Land hits and deflections to gather Sparks.");
        }
        if (pressed(Act::Heal)) {
            if (save->gourds > 0 && p.hp < maxHpOf(*save)) {
                setState(p, PState::Heal);
                p.healDone = false;
                return true;
            }
            if (save->gourds <= 0) event(WorldEvent::Toast, "The gourd is empty", "Rest at a camp to refill it.");
        }
        if (pressed(Act::Grapple)) {
            int a = findAnchor();
            if (a >= 0) {
                p.anchorIdx = a; p.anchor = anchors[a];
                p.ropeLen = std::max(120.f, (p.anchor - (p.pos + Vec2{0, -60})).len());
                setState(p, PState::Grapple);
                p.ropeT = 0;
                sfx(Sfx::Grapple, p.pos, 0.9f);
                if (save->counter("grapples") == 0) achieve("first_grapple");
                addStyle(SGrapple, 8);
                save->bump("grapples");
                // a little lift so ground grapples swing instead of drag
                if (p.onGround) { p.vel.y = -420; p.onGround = false; }
                return true;
            }
        }
        return false;
    };

    float gravScale = 1;
    switch (p.state) {
    case PState::Ground: case PState::Air: case PState::WallSlide: {
        if (tryDash()) break;
        if (canAct() && buffered(Act::Attack, tune::ATTACK_BUFFER) && (p.onGround || p.airAttackCd <= 0)) { startAttack(); break; }
        if (tryGuard()) break;
        if (p.state == PState::Ground && near && pressed(Act::Interact) && interactCooldown <= 0) {
            interactCooldown = 0.5f;
            Mechanism& m = *near;
            switch (m.kind) {
            case MechKind::Chest:
                m.active = true;
                save->flags.insert(m.spawnId);
                sfx(Sfx::Pickup, m.pos, 1.0f, 0.8f);
                if (!m.item.empty()) {
                    const ItemDef* d = findItem(m.item);
                    if (d && d->slot != ItemSlot::Material && d->slot != ItemSlot::Gem && save->count(m.item) > 0) {
                        // already carried (a replayed chapter or New Game+): the chest holds coin instead
                        save->mon += 150;
                        event(WorldEvent::Toast, "150 mon", d->name + " is already yours");
                        emit(PType::Ember, m.pos + Vec2{0, -30}, 20, 220, rgb(0xffd27a), 3, 0.8f);
                        break;
                    }
                    save->add(m.item);
                    event(WorldEvent::Toast, d ? d->name : m.item, d ? std::string(slotName(d->slot)) + " acquired" : "");
                    if (m.item == "hollow_moon") achieve("hollow_moon");
                    if (m.item == "fox_mask") achieve("fox_mask");
                }
                emit(PType::Ember, m.pos + Vec2{0, -30}, 20, 220, rgb(0xffd27a), 3, 0.8f);
                break;
            case MechKind::Scroll:
                m.active = true;
                save->flags.insert(m.spawnId);
                save->flags.insert("lore:" + m.item);
                sfx(Sfx::Pickup, m.pos, 0.8f, 1.1f);
                event(WorldEvent::Lore, m.item);
                {
                    int ch = chapterOf(save->level).number, found = 0, foundAll = 0;
                    for (auto& l : loreDefs()) {
                        if (!save->has("lore:" + l.id)) continue;
                        foundAll++;
                        if (l.chapter == ch) found++;
                    }
                    if (found >= loreCountForChapter(ch)) achieve("all_scrolls_c" + std::to_string(ch));
                    if (foundAll >= (int)loreDefs().size()) { achieve("all_scrolls"); save->flags.insert("all_scrolls"); }
                }
                break;
            case MechKind::Shrine:
                save->checkpoint = m.spawnId;
                if (!m.active) { m.active = true; save->flags.insert("shrine:" + m.spawnId); }
                sfx(Sfx::CampRest, m.pos, 0.9f);
                event(WorldEvent::ShrineMenu, m.name);
                break;
            case MechKind::Examine: event(WorldEvent::Dialogue, "REI", m.text); break;
            case MechKind::Npc: event(WorldEvent::Cutscene, m.text); break;
            case MechKind::Trial: startTrial(m); break;
            case MechKind::Sluice:
                m.active = true;
                sfx(Sfx::Gate, m.pos, 0.9f, 1.2f);
                shake(0.2f);
                activateLink(m.link);
                break;
            default: break;
            }
            break;
        }
        if (tryTools()) break;
        if (p.state != PState::Ground && !p.onGround && tryJump()) break;
        if (p.onGround && tryJump()) break;
        // wall jump
        if (p.state == PState::WallSlide && buffered(Act::Jump, tune::JUMP_BUFFER)) {
            in.consume(Act::Jump);
            p.vel = {-p.wallDir * tune::WALL_JUMP_X, -tune::WALL_JUMP_Y};
            p.facing = (float)-p.wallDir;
            p.wallJumpLock = 0.15f;
            setState(p, PState::Air);
            sfx(Sfx::Jump, p.pos, 0.7f, 1.1f);
            emit(PType::Dust, p.pos + Vec2{p.wallDir * 14.f, -30}, 6, 120, palette.fog.withA(0.5f), 8, 0.4f);
            break;
        }
        // horizontal movement
        p.crouch = p.onGround && held(Act::Down);
        float maxSpeed = p.crouch ? (save->skills.count("shadow_soft") ? tune::CROUCH_SOFT : tune::CROUCH) : tune::RUN * (hasGale() ? 1.12f : 1.f);
        if (p.inWater) maxSpeed *= 0.55f;
        float target = mx * maxSpeed;
        if (p.wallJumpLock > 0) target = p.vel.x;
        float accel = p.onGround ? (std::abs(target) > std::abs(p.vel.x) ? tune::ACCEL : tune::DECEL) : tune::AIR_ACCEL;
        p.vel.x = approach(p.vel.x, target, accel * dt);
        if (std::abs(mx) > 0.2f && p.wallJumpLock <= 0) p.facing = sign(mx);
        // variable jump height + apex hang
        if (p.vel.y < 0 && !held(Act::Jump) && p.state == PState::Air) p.vel.y = std::max(p.vel.y, -tune::JUMP_V * 0.45f);
        if (std::abs(p.vel.y) < 140 && held(Act::Jump) && !p.onGround) gravScale = 0.55f;
        // wall slide detection
        if (!p.onGround && !p.inWater) {
            int dir = mx > 0.3f ? 1 : (mx < -0.3f ? -1 : 0);
            bool touching = dir != 0 && rectSolid(Rect{p.pos.x - p.w / 2 + dir * 3, p.pos.y - p.h + 10, p.w, p.h - 30});
            if (touching && p.vel.y > -100) {
                if (p.state != PState::WallSlide) { setState(p, PState::WallSlide); p.airDashes = save->skills.count("veil_second") ? 2 : 1; }
                p.wallDir = dir;
                p.facing = (float)-dir;
                p.vel.y = std::min(p.vel.y, tune::WALL_SLIDE);
                if (g_fxRngSeedTick(time)) emit(PType::Dust, p.pos + Vec2{dir * 14.f, -40}, 1, 40, palette.fog.withA(0.4f), 6, 0.4f);
            } else if (p.state == PState::WallSlide) setState(p, PState::Air);
            else if (p.state == PState::Ground) setState(p, PState::Air);
        } else if (p.inWater && !p.onGround) {
            if (p.state != PState::Air) setState(p, PState::Air);
        } else if (p.state != PState::Ground) {
            setState(p, PState::Ground);
        }
        break;
    }
    case PState::Dash: {
        p.vel.y = 0;
        gravScale = 0;
        if (p.stateT > 0.02f && (int)(p.stateT / 0.035f) != (int)((p.stateT - dt) / 0.035f)) {
            Afterimage ai; ai.j = solve(Rig{}, p.pose, p.pos, p.facing); ai.life = 0.28f; ai.facing = p.facing;
            p.afterimages.push_back(ai);
        }
        if (p.stateT >= tune::DASH_TIME) {
            p.vel.x *= 0.42f;
            setState(p, p.onGround ? PState::Ground : PState::Air);
        }
        // dash-cancel into attack
        if (p.stateT > 0.08f && buffered(Act::Attack, 0.1f)) { p.vel.x *= 0.5f; startAttack(); }
        break;
    }
    case PState::Attack: case PState::AirAttack: {
        bool air = p.state == PState::AirAttack;
        const Swing& sw = air ? kAir : kCombo[p.combo];
        float t = p.stateT;
        if (!air) {
            if (t < sw.windup) p.vel.x = approach(p.vel.x, 0, tune::DECEL * dt);
            else if (t - dt < sw.windup) p.vel.x = p.facing * sw.lunge;
            else p.vel.x = approach(p.vel.x, 0, 2400 * dt);
            if (!p.onGround) setState(p, PState::Air);
        } else {
            p.vel.x = approach(p.vel.x, mx * tune::RUN, tune::AIR_ACCEL * 0.6f * dt);
            if (p.onGround && t > sw.windup + sw.active) { setState(p, PState::Ground); break; }
            if (t < sw.windup + sw.active) gravScale = 0.5f;
        }
        if (t >= sw.windup && t - dt < sw.windup) {
            sfx(p.combo == 2 && !air ? Sfx::SlashHeavy : Sfx::Slash, p.pos, 0.85f, 1.0f + p.combo * 0.04f);
            // the Gale Stance looses a wind blade on finishing cuts
            bool storm = save->skills.count("aura_storm") && styleRank >= 3 && !air && p.combo >= 1;
            if (hasGale() && ((!air && p.combo >= 2) || storm || (air && save->skills.count("aura_rising")))) spawnGaleBlade(air ? 0.8f : 1.f);
        }
        if (t >= sw.windup && t < sw.windup + sw.active) playerAttackBoxes();
        float total = sw.windup + sw.active + sw.recover;
        // chain: buffered attack after the strike
        if (t > sw.windup + sw.active + 0.04f) {
            if (tryDash()) break;
            if (buffered(Act::Attack, tune::ATTACK_BUFFER) && !air) { startAttack(); break; }
            if (held(Act::Guard)) { tryGuard(); break; }
            if (!air && buffered(Act::Jump, 0.1f) && p.onGround) { tryJump(); break; }
        }
        if (t >= total) setState(p, p.onGround ? PState::Ground : PState::Air);
        break;
    }
    case PState::Guard: {
        if (!held(Act::Guard)) { setState(p, p.onGround ? PState::Ground : PState::Air); break; }
        if (pressed(Act::Guard)) tryGuard();
        p.guardT += dt;
        p.vel.x = approach(p.vel.x, mx * tune::GUARD_WALK, tune::ACCEL * dt);
        if (tryDash()) break;
        if (buffered(Act::Attack, tune::ATTACK_BUFFER)) { startAttack(); break; }
        if (p.onGround && buffered(Act::Jump, tune::JUMP_BUFFER)) { tryJump(); break; }
        break;
    }
    case PState::Hurt: {
        p.vel.x = approach(p.vel.x, 0, 1600 * dt);
        if (p.stateT > 0.32f) setState(p, p.onGround ? PState::Ground : PState::Air);
        break;
    }
    case PState::Throw: {
        p.vel.x = approach(p.vel.x, mx * tune::RUN * 0.5f, tune::ACCEL * dt);
        if (p.stateT > 0.18f) setState(p, p.onGround ? PState::Ground : PState::Air);
        break;
    }
    case PState::Cast: {
        p.vel.x = approach(p.vel.x, 0, tune::DECEL * dt);
        if (!p.onGround) gravScale = 0.3f;
        if (p.stateT >= 0.22f && p.stateT - dt < 0.22f) {
            Projectile pr; pr.type = ProjType::Ember; pr.pos = pr.prevPos = p.pos + Vec2{p.facing * 30, -52};
            pr.vel = {p.facing * 950, 0}; pr.damage = 24 * (1 + stats.emberDmg) + stats.attack * 0.5f; pr.posture = 22; pr.life = 0.9f;
            projectiles.push_back(pr);
            sfx(Sfx::Ember, p.pos, 1.0f);
            shake(0.15f);
            emit(PType::Ember, pr.pos, 24, 400, rgb(0xff9a40), 5, 0.5f, 1.2f, p.facing > 0 ? 0 : PI);
        }
        if (p.stateT > 0.42f) setState(p, p.onGround ? PState::Ground : PState::Air);
        break;
    }
    case PState::Heal: {
        p.vel.x = approach(p.vel.x, mx * 90, tune::ACCEL * dt);
        if (p.stateT >= 0.55f && !p.healDone) {
            p.healDone = true;
            save->gourds--;
            p.hp = std::min(maxHpOf(*save), p.hp + 45 + stats.healAmt);
            sfx(Sfx::Heal, p.pos, 0.9f);
            emit(PType::Ember, p.pos + Vec2{0, -50}, 22, 140, rgb(0xa8ffcf), 3, 0.9f, TAU, -PI / 2);
        }
        if (p.stateT > 0.9f) setState(p, p.onGround ? PState::Ground : PState::Air);
        break;
    }
    case PState::Grapple: {
        p.ropeT += dt;
        // shorten the rope a little at first for a satisfying pull
        p.ropeLen = std::max(110.f, p.ropeLen - dt * 260 * clamp01(1 - p.ropeT * 2));
        p.vel.x += mx * 1100 * dt;
        p.vel.y += tune::GRAVITY * dt;
        Vec2 hand = p.pos + Vec2{0, -60};
        Vec2 next = hand + p.vel * dt;
        Vec2 d = next - p.anchor;
        float L = d.len();
        if (L > p.ropeLen) {
            Vec2 n = d / L;
            next = p.anchor + n * p.ropeLen;
            float radial = dot(p.vel, n);
            if (radial > 0) p.vel -= n * radial;
        }
        Vec2 move = (next - hand) / dt;
        p.vel = move;
        gravScale = 0;
        if (std::abs(p.vel.x) > 30) p.facing = sign(p.vel.x);
        bool release = !held(Act::Grapple) || pressed(Act::Jump);
        if (release || p.stateT > 4.0f) {
            float boost = pressed(Act::Jump) ? 520.f : 300.f;
            p.vel.y = std::min(p.vel.y, 0.f) - boost;
            p.vel.x *= 1.12f;
            p.airDashes = save->skills.count("veil_second") ? 2 : 1;
            setState(p, PState::Air);
            sfx(Sfx::Swoosh, p.pos, 0.7f);
            emit(PType::Thread, p.pos + Vec2{0, -60}, 8, 100, rgb(0xd8c8ff), 2, 0.5f);
        }
        break;
    }
    case PState::Execute: {
        Enemy* e = p.target >= 0 && p.target < (int)enemies.size() ? &enemies[p.target] : nullptr;
        if (e) p.pos.x = damp(p.pos.x, e->pos.x - p.facing * 60, 14, dt);
        if (p.stateT >= 0.36f && p.stateT - dt < 0.36f && e) {
            p.swingId++;
            flash = settings && settings->reduceFlashes ? 0.15f : 0.55f; flashColor = rgb(0xfff4e0);
            hitStop(0.12f);
            shake(0.7f);
            input->rumble(1.0f, 0.8f, 260);
            emit(PType::Spark, e->pos + Vec2{0, -50}, 30, 900, rgb(0xfff2c0), 3, 0.5f);
            Vec2 boltFrom = e->pos;
            killEnemy(p.target, true, false);
            gainKi(20);
            if (hasStance("thunder")) thunderStrike(boltFrom);
            if (stats.executeHeal > 0) p.hp = std::min(maxHpOf(*save), p.hp + stats.executeHeal);
            save->bump("executions");
            if (save->counter("executions") == 1) achieve("first_execution");
            for (int k = 0; k < 6; k++) {
                TrailSample ts{p.pos + Vec2{p.facing * 10, -60}, p.pos + Vec2{p.facing * (60 + k * 20.f), -56}, time};
                p.trail.push_back(ts);
            }
        }
        if (p.stateT > 0.9f) { p.target = -1; setState(p, PState::Ground); }
        break;
    }
    case PState::StealthKill: {
        Enemy* e = p.target >= 0 && p.target < (int)enemies.size() ? &enemies[p.target] : nullptr;
        gravScale = 0;
        p.vel = {0, 0};
        if (p.stateT >= 0.32f && p.stateT - dt < 0.32f && e) {
            hitStop(0.09f);
            shake(0.35f);
            input->rumble(0.6f, 0.6f, 160);
            killEnemy(p.target, false, true);
            save->bump("stealthKills");
            if (save->counter("stealthKills") == 1) achieve("stealth_kill");
            if (save->skills.count("shadow_drop")) { gainKi(30); p.hp = std::min(maxHpOf(*save), p.hp + 8); }
            if (save->skills.count("shadow_breath")) save->shuriken = std::min(save->maxShuriken(), save->shuriken + 2);
        }
        if (p.stateT > 0.7f) { p.target = -1; setState(p, p.onGround ? PState::Ground : PState::Air); }
        break;
    }
    case PState::Locked: case PState::Grabbed:
        p.vel.x = approach(p.vel.x, 0, tune::DECEL * dt);
        break;
    case PState::Dead: break;
    }

    if (locked && (p.state == PState::Ground || p.state == PState::Air)) p.vel.x = approach(p.vel.x, 0, tune::DECEL * dt);

    // ---------------------------------------------------------- physics
    // water and rising wind
    {
        int bx = (int)std::floor(p.pos.x / TILE);
        bool wasIn = p.inWater;
        p.inWater = level.waterAt(bx, (int)std::floor((p.pos.y - 30) / TILE));
        p.headUnder = level.waterAt(bx, (int)std::floor((p.pos.y - 78) / TILE));
        if (p.inWater && !wasIn && p.vel.y > 200) {
            sfx(Sfx::Land, p.pos, 0.7f, 0.6f);
            emit(PType::Splash, p.pos + Vec2{0, -30}, (int)(10 + p.vel.y / 60), 380, palette.water * 2.4f, 4, 0.7f, 1.3f, -PI / 2);
        }
        if (p.inWater && p.state != PState::Grapple && p.state != PState::Dash) {
            gravScale *= 0.3f;
            p.vel.y = std::min(p.vel.y, held(Act::Down) ? 320.f : 170.f);
            p.vel.x *= 1 - std::min(1.f, 2.5f * dt);
            if (p.headUnder && !held(Act::Down) && p.vel.y > -60) p.vel.y = approach(p.vel.y, -120, 600 * dt);   // gentle buoyancy
            p.airDashes = std::max(p.airDashes, 1);
        }
        bool up = level.updraftAt(bx, (int)std::floor((p.pos.y - 40) / TILE));
        if (up && !p.inUpdraft) sfx(Sfx::Swoosh, p.pos, 0.6f, 0.7f);
        p.inUpdraft = up;
        if (up && p.state != PState::Grapple && p.state != PState::Dash) {
            float want = held(Act::Down) ? 140.f : -620.f;
            p.vel.y = approach(p.vel.y, want, 3600 * dt);
            gravScale = 0;
            p.airDashes = save->skills.count("veil_second") ? 2 : 1;
            if (p.onGround && !held(Act::Down)) { p.onGround = false; p.pos.y -= 2; if (p.state == PState::Ground) setState(p, PState::Air); }
        }
    }
    if (p.state != PState::Grapple) p.vel.y = std::min(tune::MAX_FALL, p.vel.y + tune::GRAVITY * gravScale * dt);
    float h = p.crouch ? 58.f : 82.f;
    if (!p.crouch && p.h < 82 && rectSolid(Rect{p.pos.x - p.w / 2, p.pos.y - 82, p.w, 82})) { h = p.h; p.crouch = true; }
    p.h = h;
    bool drop = false;
    float vyBefore = p.vel.y;
    auto r = moveBody(p.pos, p.vel, p.w, p.h, dt, drop);
    bool wasGround = p.onGround;
    p.onGround = r.ground || (p.vel.y >= 0 && onGroundAt(p.pos, p.w));
    if (p.state == PState::Grapple && r.ground) setState(p, PState::Ground);
    if (p.onGround && !wasGround && vyBefore > 300) {
        p.landT = clamp01(vyBefore / 1200.f) * 0.16f;
        sfx(Sfx::Land, p.pos, clamp01(vyBefore / 1000.f));
        emit(PType::Dust, p.pos, (int)(4 + vyBefore / 120), 160, palette.fog.withA(0.5f), 12, 0.5f, PI * 0.8f, -PI / 2);
    }
    if (p.onGround && (p.state == PState::Ground)) {
        // remember solid footing for spike/fall respawns
        int tx = (int)std::floor(p.pos.x / TILE), ty = (int)std::floor((p.pos.y + 2) / TILE);
        if (level.solidAt(tx, ty) && level.at(tx, ty - 1) != Tile::Spikes) p.lastSafe = p.pos;
    }
    // footsteps
    if (p.onGround && p.state == PState::Ground && std::abs(p.vel.x) > 80) {
        float prev = p.runPhase;
        p.runPhase += dt * std::abs(p.vel.x) / 34.f;
        if ((int)(p.runPhase / PI) != (int)(prev / PI)) sfx(Sfx::Step, p.pos, p.crouch ? 0.15f : 0.4f, p.crouch ? 0.8f : 1.0f);
    } else if (p.state == PState::Ground) p.runPhase = 0;

    // stealth: tall grass and noise
    int gx = (int)std::floor(p.pos.x / TILE), gy = (int)std::floor((p.pos.y - 10) / TILE);
    p.inGrass = level.grassAt(gx, gy);
    // Shadow Stance: stillness in a crouch becomes a cloak
    if (hasStance("shadow") && p.crouch && p.onGround && std::abs(p.vel.x) < 20 && p.state == PState::Ground) p.stillT += dt;
    else p.stillT = 0;
    bool wasCloaked = p.cloaked;
    p.cloaked = p.stillT > (save->skills.count("shadow_breath") ? 0.4f : 0.8f);
    if (p.cloaked && !wasCloaked) { sfx(Sfx::Swoosh, p.pos, 0.4f, 0.5f); emit(PType::Smoke, p.pos + Vec2{0, -30}, 14, 60, rgb(0x101018, 0.6f), 18, 0.9f); }
    p.hidden = (p.inGrass && p.crouch) || p.headUnder || p.cloaked;
    p.noise = (p.onGround && std::abs(p.vel.x) > 200 && !p.crouch) ? 1.f : (p.state == PState::Attack ? 1.f : 0.f);

    for (auto& ai : p.afterimages) ai.life -= dt;
    p.afterimages.erase(std::remove_if(p.afterimages.begin(), p.afterimages.end(), [](const Afterimage& a) { return a.life <= 0; }), p.afterimages.end());

    // wind motes drift around Rei once the Gale is hers
    if (hasGale() && hash32((uint32_t)(time * 120)) % 5 == 0) {
        float a = time * 3.1f;
        Particle m; m.type = PType::Thread; m.pos = p.pos + Vec2{std::cos(a) * 34, -46 + std::sin(a * 1.3f) * 30};
        m.vel = Vec2{-std::sin(a), std::cos(a)} * 90 + Vec2{-p.vel.x * 0.2f, 0}; m.life = m.maxLife = 0.5f; m.size = 1.5f;
        m.col = rgb(0x9ff0e0, 0.8f); m.fg = true;
        particles.push_back(m);
    }
    playerAnim(dt);
    save->hp = p.hp; save->ki = p.ki;
}

// The g_fxRng lives in world.cpp; a cheap deterministic sprinkle here instead.
bool g_fxRngSeedTick(float t) { return hash32((uint32_t)(t * 120)) % 7 == 0; }

void World::playerAttackBoxes() {
    Player& p = player;
    const Swing& sw = p.state == PState::AirAttack ? kAir : kCombo[p.combo];
    Rect hb = sw.box;
    Rect box{p.facing > 0 ? p.pos.x + hb.x : p.pos.x - hb.x - hb.w, p.pos.y + hb.y, hb.w, hb.h};
    float dmgMul = p.state == PState::AirAttack ? kAir.dmg : sw.dmg;
    bool emberProc = stats.comboEmber > 0 && p.state == PState::Attack && p.combo == 2;
    for (size_t i = 0; i < enemies.size(); i++) {
        Enemy& e = enemies[i];
        if (!e.alive() || !e.box().overlaps(box)) continue;
        if (std::find(p.swingHits.begin(), p.swingHits.end(), (int)i) != p.swingHits.end()) continue;
        p.swingHits.push_back((int)i);
        float atk = stats.attack + (save->skills.count("blade_tide") && p.hp < maxHpOf(*save) * 0.4f ? 10.f : 0.f);
        AttackInfo a; a.damage = atk * dmgMul; a.posture = sw.posture; a.origin = p.pos; a.ember = emberProc;
        if (emberProc) { a.damage *= 1.25f; emit(PType::Ember, e.pos + Vec2{0, -50}, 16, 300, rgb(0xff8a3a), 5, 0.5f); }
        hitEnemy((int)i, a);
    }
    // breakable fences, bells and ropes
    int x0 = (int)std::floor(box.x / TILE), x1 = (int)std::floor(box.right() / TILE);
    int y0 = (int)std::floor(box.y / TILE), y1 = (int)std::floor(box.bottom() / TILE);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (level.at(x, y) == Tile::Breakable) {
                level.set(x, y, Tile::Broken);
                Vec2 c{(x + 0.5f) * TILE, (y + 0.5f) * TILE};
                sfx(Sfx::Break, c, 0.9f);
                emit(PType::Leaf, c, 8, 300, palette.nearC, 8, 1.5f);
                emit(PType::Dust, c, 8, 150, palette.fog.withA(0.5f), 14, 0.6f);
                shake(0.15f);
            }
    for (auto& m : mechs) {
        if (m.kind == MechKind::Rope && !m.active) {
            Rect r{m.pos.x - 14, m.pos.y - m.h * TILE, 28, m.h * TILE};
            if (r.overlaps(box)) { m.active = true; sfx(Sfx::Slash, m.pos, 0.9f, 1.3f); activateLink(m.link); }
        }
        if (m.kind == MechKind::Bell && !m.active) {
            Rect r{m.pos.x - 28, m.pos.y - 10, 56, 70};
            if (r.overlaps(box)) { m.swingVel = p.facing * 4; sfx(Sfx::Catch, m.pos, 0.6f, 1.0f); event(WorldEvent::Hint, "bell"); }
        }
    }
}

// ------------------------------------------------------------------ animation

void World::playerAnim(float dt) {
    Player& p = player;
    Pose target;
    float t = p.stateT;
    auto phase3 = [&](const Pose& w, const Pose& s, const Pose& f, float wu, float ac, float rc) {
        if (t < wu) return lerp(p.fromPose, w, ease::outQuad(clamp01(t / wu)));
        if (t < wu + ac) return lerp(w, s, ease::outExpo(clamp01((t - wu) / ac)));
        return lerp(s, f, ease::outCubic(clamp01((t - wu - ac) / std::max(0.01f, rc))));
    };
    bool useBlend = true;
    switch (p.state) {
    case PState::Ground: {
        float sp = clamp01(std::abs(p.vel.x) / tune::RUN);
        if (p.crouch) target = sneakPose(p.runPhase, clamp01(std::abs(p.vel.x) / 150.f));
        else if (sp > 0.05f) target = runPose(p.runPhase, std::max(0.35f, sp));
        else {
            p.idleT += dt;
            target = poses::idle;
            target.hipY += std::sin(p.idleT * 2.2f) * 1.0f;
            target.lean += std::sin(p.idleT * 2.2f) * 1.2f * DEG;
            target.armFU += std::sin(p.idleT * 2.2f + 0.5f) * 2 * DEG;
        }
        if (!p.lockedPose.empty()) {
            if (p.lockedPose == "kneel") target = poses::kneel;
            else if (p.lockedPose == "stand") target = poses::stand;
            else if (p.lockedPose == "lookup") target = poses::lookUp;
        }
        if (p.landT > 0) target.hipY += 14 * (p.landT / 0.16f);
        break;
    }
    case PState::Air: {
        float k = clamp01((p.vel.y + 600) / 1200.f);
        target = lerp(poses::jump, poses::fall, k);
        break;
    }
    case PState::WallSlide: target = poses::wall; break;
    case PState::Dash: target = poses::dash; break;
    case PState::Attack: {
        const Swing& sw = kCombo[p.combo];
        switch (p.combo) {
        case 0: target = phase3(poses::a1w, poses::a1s, poses::a1f, sw.windup, sw.active, sw.recover); break;
        case 1: target = phase3(poses::a2w, poses::a2s, poses::a2f, sw.windup, sw.active, sw.recover); break;
        case 2: target = phase3(poses::a3w, poses::a3s, poses::a3f, sw.windup, sw.active, sw.recover); break;
        default: target = phase3(poses::a4w, poses::a4s, poses::a2f, sw.windup, sw.active, sw.recover); break;
        }
        useBlend = false;
        break;
    }
    case PState::AirAttack: target = phase3(poses::airW, poses::airS, poses::fall, kAir.windup, kAir.active, kAir.recover); useBlend = false; break;
    case PState::Guard: target = p.deflectT > 0 ? poses::deflect : poses::guard; break;
    case PState::Hurt: target = poses::hurt; break;
    case PState::Throw: target = poses::throwP; break;
    case PState::Cast: target = poses::cast; break;
    case PState::Heal: target = poses::heal; break;
    case PState::Grapple: {
        target = poses::grapple;
        // aim the throwing arm at the anchor
        Vec2 d = p.anchor - (p.pos + Vec2{0, -60});
        float a = std::atan2(d.x * p.facing, d.y);   // angle from down, positive forward
        target.armBU = a + target.lean;
        target.armBL = 0;
        float swing = clampf(p.vel.x * p.facing / 900.f, -1, 1);
        target.legFU += swing * 30 * DEG; target.legBU += swing * 20 * DEG;
        break;
    }
    case PState::Execute: target = phase3(poses::exW, poses::exS, poses::exS, 0.32f, 0.06f, 0.5f); useBlend = false; break;
    case PState::StealthKill: target = phase3(poses::skW, poses::skS, poses::skS, 0.28f, 0.06f, 0.3f); useBlend = false; break;
    case PState::Dead: target = poses::dead; break;
    case PState::Locked: case PState::Grabbed: target = p.lockedPose == "kneel" ? poses::kneel : poses::stand; break;
    }
    if (useBlend && p.blend < 1) {
        p.blend = std::min(1.f, p.blend + dt / p.blendDur);
        p.pose = lerp(p.fromPose, target, ease::outQuad(p.blend));
    } else {
        p.blend = 1;
        p.pose = target;
    }
    // secondary motion: scarf and headband tails
    Joints j = solve(Rig{}, p.pose, p.pos, p.facing);
    Vec2 wind = palette.wind * 4 + Vec2{-p.vel.x * 2.2f, -p.vel.y * 0.6f};
    float floorY = p.onGround ? p.pos.y - 2 : 1e9f;
    p.scarf.update(j.neck + Vec2{-p.facing * 4, 2}, {0, 900}, wind, dt, 0.96f, floorY);
    Vec2 knot = j.head + Vec2{-p.facing * 7, -2};
    p.tailA.update(knot, {0, 700}, wind * 1.2f, dt, 0.95f, floorY);
    p.tailB.update(knot + Vec2{0, 2}, {0, 700}, wind * 1.05f, dt, 0.95f, floorY);
    // blade trail
    bool trailing = p.state == PState::Attack || p.state == PState::AirAttack;
    if (trailing) {
        const Swing& sw = p.state == PState::Attack ? kCombo[p.combo] : kAir;
        if (p.stateT >= sw.windup * 0.6f && p.stateT <= sw.windup + sw.active + 0.06f)
            p.trail.push_back({lerp(j.swordBase, j.swordTip, 0.35f), j.swordTip, time});
    }
    while (!p.trail.empty() && time - p.trail.front().t > 0.14f) p.trail.pop_front();
}

} // namespace sv
