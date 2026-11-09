// Environment systems added for Chapters II-VI: weather, water and floods, crumbling ledges,
// rising wind, fire-watch beacons, the Mirror Province layers, the Unravelling chase,
// and the stance effects that act on the world (thunder, mirror decoy).
#include "game/world.h"
#include "core/util.h"

namespace sv {

static Rng g_envRng(777);

// ------------------------------------------------------------------ thunder stance

void World::thunderStrike(Vec2 at) {
    // an execution calls the storm down on up to three foes nearby
    std::vector<std::pair<float, int>> near;
    for (size_t i = 0; i < enemies.size(); i++) {
        const Enemy& e = enemies[i];
        if (!e.alive() || e.st == EState::BeingKilled) continue;
        float d = (e.pos - at).len();
        if (d < 640) near.push_back({d, (int)i});
    }
    std::sort(near.begin(), near.end());
    int n = 0;
    for (auto& [d, i] : near) {
        if (n >= 3) break;
        Enemy& e = enemies[i];
        Vec2 top{e.pos.x + g_envRng.range(-60, 60), cam.pos.y - 700};
        bolts.push_back({top, e.pos + Vec2{0, -40}, 0.35f, (int)g_envRng.irange(0, 100000)});
        AttackInfo a; a.damage = 28 + stats.attack * 0.4f; a.posture = 45; a.projectile = true; a.origin = at;
        hitEnemy(i, a);
        if (enemies[i].alive() && enemies[i].st != EState::Broken && !enemies[i].boss) { enemies[i].st = EState::Stunned; enemies[i].stT = 0; enemies[i].atk = -1; }
        emit(PType::Spark, e.pos + Vec2{0, -40}, 24, 800, rgb(0xd8f0ff), 3, 0.4f);
        n++;
    }
    if (n > 0) {
        sfx(Sfx::Perilous, at, 0.9f, 1.8f);
        sfx(Sfx::HitHeavy, at, 1.0f, 0.5f);
        shake(0.5f);
        flash = std::max(flash, settings && settings->reduceFlashes ? 0.08f : 0.3f); flashColor = rgb(0xd8f0ff);
        if (n >= 3) achieve("thunder_three");
    }
}

// ------------------------------------------------------------------ mirror layers

bool World::tryFlipLayer() {
    int next = 1 - level.layer;
    // never flip into a wall
    Rect pb = playerBox();
    int x0 = (int)std::floor(pb.x / TILE), x1 = (int)std::floor((pb.right() - 1) / TILE);
    int y0 = (int)std::floor(pb.y / TILE), y1 = (int)std::floor((pb.bottom() - 1) / TILE);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (level.solidInLayer(x, y, next)) {
                sfx(Sfx::Clang, player.pos, 0.4f, 1.6f);
                emit(PType::Thread, player.pos + Vec2{0, -50}, 8, 200, rgb(0xff8080), 2, 0.3f);
                return false;
            }
    level.layer = next;
    layerFlips++;
    save->bump("layer_flips");
    if (save->counter("layer_flips") >= 50) achieve("mirror_50");
    ripple = 1.0f; rippleAt = player.pos + Vec2{0, -50};
    sfx(Sfx::Swoosh, player.pos, 0.8f, next ? 0.6f : 1.3f);
    emit(PType::Thread, player.pos + Vec2{0, -50}, 26, 380, next ? rgb(0xe8dcff) : rgb(0xfff2d0), 2.5f, 0.6f);
    return true;
}

// ------------------------------------------------------------------ floods

void World::floodRect(const Rect& area, int topRow) {
    // fill (or, for a negative topRow, drain) an area one row at a time
    bool fill = topRow >= 0;
    int row = fill ? (int)std::floor((area.bottom() - 1) / TILE) : (int)std::floor(area.y / TILE);
    floods.push_back({area, row, 0, fill});
    sfx(Sfx::Gate, area.center(), 1.0f, 0.6f);
    shake(0.3f);
}

static void applyFloodInstant(Level& L, const Rect& area, bool fill) {
    int x0 = (int)std::floor(area.x / TILE), x1 = (int)std::floor((area.right() - 1) / TILE);
    int y0 = (int)std::floor(area.y / TILE), y1 = (int)std::floor((area.bottom() - 1) / TILE);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            Tile t = L.at(x, y);
            if (fill && t == Tile::Empty) L.set(x, y, Tile::Water);
            if (!fill && t == Tile::Water) L.set(x, y, Tile::Empty);
        }
}

void World::resetEnvironment() {
    // crumbled ledges re-form; drains and floods take their saved state; beacons relight
    for (auto& t : level.tiles) if (t == Tile::CrumbleGone) t = Tile::Crumble;
    crumbling.clear();
    floods.clear();
    bolts.clear();
    decoyT = 0;
    chaseActive = false;
    spottedT = 0; beaconSpotted = false;
    for (auto& m : mechs)
        if (m.kind == MechKind::Drain && save->has("open:" + m.id)) applyFloodInstant(level, m.area, m.text == "fill");
}

// ------------------------------------------------------------------ per-frame

void World::updateEnvironment(float dt) {
    Player& p = player;
    decoyT = std::max(0.f, decoyT - dt);
    for (auto& b : bolts) b.life -= dt;
    bolts.erase(std::remove_if(bolts.begin(), bolts.end(), [](const Bolt& b) { return b.life <= 0; }), bolts.end());
    layerBlend = damp(layerBlend, (float)level.layer, 10, dt);

    // crumbling ledges: step on one and it gives way; it returns a little later
    if (p.onGround && p.state != PState::Dead) {
        int ty = (int)std::floor((p.pos.y + 2) / TILE);
        int x0 = (int)std::floor((p.pos.x - p.w / 2 + 2) / TILE), x1 = (int)std::floor((p.pos.x + p.w / 2 - 2) / TILE);
        for (int x = x0; x <= x1; x++) {
            if (level.at(x, ty) != Tile::Crumble) continue;
            int idx = ty * level.width + x;
            bool known = false;
            for (auto& c : crumbling) if (c.first == idx) known = true;
            if (!known) {
                crumbling.push_back({idx, 0.45f});
                sfx(Sfx::Break, Vec2{(x + 0.5f) * TILE, (float)ty * TILE}, 0.35f, 1.5f);
            }
        }
    }
    for (auto& c : crumbling) {
        int x = c.first % level.width, y = c.first / level.width;
        float before = c.second;
        c.second -= dt;
        Vec2 at{(x + 0.5f) * TILE, (y + 0.5f) * TILE};
        if (level.at(x, y) == Tile::Crumble) {
            if (g_envRng.chance(0.3f)) emit(PType::Dust, at + Vec2{g_envRng.range(-20, 20), -20}, 1, 40, palette.fog.withA(0.5f), 6, 0.4f);
            if (c.second <= 0) {
                level.set(x, y, Tile::CrumbleGone);
                c.second = 4.0f;
                emit(PType::Dust, at, 16, 220, palette.fog.withA(0.6f), 14, 0.8f);
                emit(PType::Leaf, at, 6, 200, palette.grass * 3.f, 5, 1.2f);
                sfx(Sfx::Break, at, 0.7f);
            }
        } else if (c.second <= 0) {
            // re-form only when nobody is standing in the space
            Rect r{x * (float)TILE, y * (float)TILE, (float)TILE, (float)TILE};
            if (!r.overlaps(playerBox())) { level.set(x, y, Tile::Crumble); c.first = -1; emit(PType::Dust, at, 8, 80, palette.fog.withA(0.4f), 8, 0.5f); }
            else c.second = 0.3f;
        }
        (void)before;
    }
    crumbling.erase(std::remove_if(crumbling.begin(), crumbling.end(), [](const auto& c) { return c.first < 0; }), crumbling.end());

    // floods advance a row at a time
    for (auto& f : floods) {
        f.t += dt;
        if (f.t < 0.12f) continue;
        f.t = 0;
        int x0 = (int)std::floor(f.area.x / TILE), x1 = (int)std::floor((f.area.right() - 1) / TILE);
        int y0 = (int)std::floor(f.area.y / TILE), y1 = (int)std::floor((f.area.bottom() - 1) / TILE);
        if (f.row < y0 || f.row > y1) { f.row = -9999; continue; }
        for (int x = x0; x <= x1; x++) {
            Tile t = level.at(x, f.row);
            if (f.fill && t == Tile::Empty) level.set(x, f.row, Tile::Water);
            if (!f.fill && t == Tile::Water) level.set(x, f.row, Tile::Empty);
            if (g_envRng.chance(0.25f)) emit(PType::Splash, Vec2{(x + 0.5f) * TILE, (float)f.row * TILE + 4}, 1, 160, palette.water * 2.5f, 3, 0.5f, 1.0f, -PI / 2);
        }
        f.row += f.fill ? -1 : 1;
    }
    floods.erase(std::remove_if(floods.begin(), floods.end(), [](const FloodJob& f) { return f.row == -9999; }), floods.end());

    // fire-watch beacons sweep their light; being seen raises the whole garrison
    beaconSpotted = false;
    for (auto& m : mechs) {
        if (m.kind != MechKind::Beacon) continue;
        m.timer += dt * m.speed;
        if (!m.active) continue;
        float ang = PI / 2 + std::sin(m.timer) * 0.75f;   // pointing down, swinging
        Vec2 src = m.pos + Vec2{0, -m.h * TILE};
        Vec2 to = p.pos + Vec2{0, -50} - src;
        float d = to.len();
        if (d > m.range * TILE || d < 1) continue;
        float a = std::atan2(to.y, to.x);
        float diff = std::abs(std::remainder(a - ang, TAU));
        if (diff < 0.22f && !p.hidden && p.state != PState::Dead && lineOfSight(src, p.pos + Vec2{0, -50})) {
            beaconSpotted = true;
            spottedT += dt;
            if (spottedT > 0.35f) {
                spottedT = 0;
                int roused = 0;
                for (auto& e : enemies) {
                    if (!e.alive() || e.boss || std::abs(e.pos.x - p.pos.x) > 1400) continue;
                    if (e.awareness < 1) roused++;
                    e.awareness = 1; e.lastKnown = p.pos; e.sleeping = false;
                    if (e.st == EState::Patrol || e.st == EState::Idle || e.st == EState::Search || e.st == EState::Suspicious) { e.st = EState::Engage; e.stT = 0; e.alertIcon = 1; }
                }
                groveSpotted = true;
                if (roused) { sfx(Sfx::Alert, m.pos, 1.0f); save->flags.insert("spotted_by_beacon"); }
            }
        }
    }
    if (!beaconSpotted) spottedT = std::max(0.f, spottedT - dt);

    // the Unravelling: the world comes apart behind you
    if (chaseActive) {
        chaseX += chaseSpeed * dt;
        chaseX = std::max(chaseX, p.pos.x - 1300);   // never fall hopelessly far behind
        if (g_envRng.chance(0.9f)) emit(PType::Thread, Vec2{chaseX + g_envRng.range(-30, 30), cam.pos.y + g_envRng.range(-500, 500)}, 1, 160, palette.thread, 3, 0.8f);
        if (p.pos.x < chaseX + 24 && p.state != PState::Dead && p.iframes <= 0) {
            AttackInfo a; a.damage = 34; a.posture = 0; a.perilous = true; a.knock = 0; a.origin = Vec2{chaseX - 100, p.pos.y};
            hitPlayer(a);
            chaseHit = true;
            p.vel = {720, -560};
            p.pos.x = std::max(p.pos.x, chaseX + 40);
            chaseX -= 260;
        }
    }

    // weather
    float vh = 1080.f / (cam.zoom * kBaseZoom), vw = vh * 16.f / 9.f * 1.3f;
    Rect view{cam.pos.x - vw / 2, cam.pos.y - vh / 2, vw, vh};
    auto spawnRate = [&](float rate) { float n = rate * dt; int k = (int)n; if (g_envRng.uniform() < n - k) k++; return k; };
    for (int i = spawnRate(palette.rain * 260); i > 0; i--) {
        Particle q; q.type = PType::Rain; q.pos = {g_envRng.range(view.x - 200, view.right()), view.y - 20};
        q.vel = {palette.wind.x * 2 + 160, g_envRng.range(1500, 1900)};
        q.life = q.maxLife = 1.2f; q.size = g_envRng.range(14, 26);
        q.col = rgb(0xb8d8e8, g_envRng.range(0.25f, 0.5f)); q.fg = g_envRng.chance(0.3f);
        particles.push_back(q);
    }
    for (int i = spawnRate(palette.snow * 60); i > 0; i--) {
        Particle q; q.type = PType::Snow; q.pos = {g_envRng.range(view.x - 100, view.right()), view.y - 20};
        q.vel = {palette.wind.x + g_envRng.range(-30, 30), g_envRng.range(60, 140)};
        q.life = q.maxLife = g_envRng.range(6, 10); q.size = g_envRng.range(1.5f, 4.f);
        q.col = rgb(0xffffff, g_envRng.range(0.5f, 0.9f)); q.drag = 0.1f; q.fg = g_envRng.chance(0.25f);
        particles.push_back(q);
    }
    for (int i = spawnRate(palette.threads * 16); i > 0; i--) {
        Particle q; q.type = PType::Thread; q.pos = {g_envRng.range(view.x, view.right()), g_envRng.range(view.y, view.bottom())};
        q.vel = {g_envRng.range(-30, 30), g_envRng.range(-50, -10)};
        q.life = q.maxLife = g_envRng.range(2, 4); q.size = g_envRng.range(1.5f, 2.5f);
        q.col = palette.thread.withA(0.7f);
        particles.push_back(q);
    }
}

} // namespace sv
