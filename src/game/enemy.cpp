#include "game/world.h"
#include "core/util.h"
#include <map>

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

// small modifiers for the attack table
static EAttack proj(EAttack a, int kind, int count = 1) { a.proj = kind; a.projCount = count; a.projectile = true; return a; }
static EAttack hop(EAttack a, float v) { a.hop = v; return a; }
static EAttack dive(EAttack a, float v) { a.dive = v; return a; }
static EAttack blink(EAttack a) { a.blink = true; return a; }
static EAttack phase(EAttack a, int p) { a.minPhase = p; a.phase2 = p >= 2; return a; }
static EAttack summon(EAttack a) { a.summon = true; return a; }
static EAttack ambush(EAttack a) { a.ambush = true; return a; }

static std::vector<EDef> buildDefs() {
    std::vector<EDef> v((size_t)EType::Count);
    auto put = [&](EDef d) { v[(size_t)d.type] = d; };
    // ---------------------------------------------------------------- Chapter I: the iron vanguard
    put({EType::Swordsman, "Vanguard Swordsman", "足軽", 60, 75, 110, 270, 560, 1.0f, 0.30f, 1, 25, 12, {
        A("slash", 0.45f, 0.10f, 0.45f, 15, 18, false, {8, -82, 94, 66}, 220, 125, 1.0f, -1, 0),
        A("double1", 0.38f, 0.10f, 0.12f, 13, 14, false, {8, -82, 94, 66}, 160, 125, 0.8f, 2, 0),
        A("double2", 0.22f, 0.10f, 0.50f, 14, 16, false, {8, -112, 92, 92}, 200, 140, 0.0f, -1, 1),
        A("thrust", 0.70f, 0.16f, 0.70f, 26, 30, true, {0, -68, 124, 30}, 760, 270, 0.45f, -1, 2),
    }});
    put({EType::Spearman, "Vanguard Spearman", "槍兵", 70, 85, 100, 240, 580, 1.05f, 0.25f, 1, 30, 14, {
        A("poke", 0.50f, 0.12f, 0.45f, 16, 16, false, {10, -72, 168, 28}, 120, 175, 1.0f, -1, 2),
        A("poke2a", 0.40f, 0.10f, 0.14f, 14, 14, false, {10, -72, 168, 28}, 90, 175, 0.7f, 2, 2),
        A("poke2b", 0.22f, 0.10f, 0.50f, 15, 16, false, {10, -90, 168, 28}, 120, 185, 0.0f, -1, 2),
        A("sweep", 0.75f, 0.18f, 0.70f, 24, 28, true, {-30, -36, 214, 36}, 80, 200, 0.5f, -1, 3),
    }});
    {
        EDef d{EType::Archer, "Vanguard Archer", "弓兵", 40, 50, 100, 230, 760, 0.95f, 0.0f, 1, 20, 10, {
            proj(A("shoot", 0.85f, 0.05f, 0.55f, 12, 10, false, {0, 0, 0, 0}, 0, 900, 1.0f, -1, 4), 1),
            A("kick", 0.30f, 0.10f, 0.40f, 8, 25, false, {6, -62, 62, 42}, 150, 85, 0.0f, -1, 5),
        }};
        d.ranged = true; d.canGuard = false;
        put(d);
    }
    {
        EDef d{EType::Tetsugen, "Tetsu, the Iron Ronin", "鉄", 220, 150, 150, 310, 1400, 1.38f, 0.35f, 2, 300, 300, {
            A("cleave1", 0.55f, 0.12f, 0.16f, 20, 20, false, {10, -122, 156, 104}, 260, 175, 1.0f, 1, 0),
            A("cleave2", 0.30f, 0.12f, 0.16f, 20, 20, false, {10, -156, 146, 134}, 220, 175, 0.0f, 2, 1),
            A("cleave3", 0.40f, 0.14f, 0.62f, 24, 28, false, {10, -122, 166, 112}, 300, 175, 0.0f, -1, 0),
            A("overhead", 0.95f, 0.14f, 0.75f, 34, 40, false, {20, -186, 144, 186}, 200, 175, 0.8f, -1, 1),
            A("charge", 0.85f, 0.30f, 0.80f, 30, 30, true, {0, -94, 156, 44}, 1150, 540, 0.7f, -1, 2),
            A("sweep", 0.75f, 0.18f, 0.70f, 26, 26, true, {-60, -46, 290, 46}, 120, 230, 0.6f, -1, 3),
            proj(A("flamewave", 0.70f, 0.10f, 0.70f, 20, 10, true, {0, 0, 0, 0}, 0, 900, 0.8f, -1, 6, true, true), 2),
            A("grab", 0.60f, 0.20f, 0.90f, 36, 0, true, {0, -124, 116, 112}, 640, 280, 0.6f, -1, 2, false, true, true),
            A("fcleave1", 0.45f, 0.12f, 0.10f, 22, 20, false, {10, -122, 166, 112}, 260, 180, 1.0f, 9, 0, false, true),
            A("fcleave2", 0.25f, 0.12f, 0.10f, 22, 20, false, {10, -156, 156, 134}, 220, 180, 0.0f, 10, 1, false, true),
            A("fcleave3", 0.30f, 0.14f, 0.60f, 26, 26, false, {10, -122, 176, 112}, 320, 180, 0.0f, -1, 0, false, true),
        }};
        d.boss = true; d.flag = "boss_tetsugen"; d.achievement = "boss_tetsugen"; d.reward = "iron_vow";
        put(d);
    }
    // ---------------------------------------------------------------- Chapter II: reavers of the drowned terraces
    put({EType::Reaver, "Marsh Reaver", "鎌賊", 78, 80, 120, 300, 540, 1.0f, 0.2f, 1, 32, 16, {
        A("hook1", 0.34f, 0.09f, 0.10f, 14, 14, false, {8, -84, 92, 64}, 240, 122, 1.0f, 1, 0),
        A("hook2", 0.18f, 0.09f, 0.10f, 14, 14, false, {8, -102, 92, 86}, 200, 122, 0.0f, 2, 1),
        A("hook3", 0.20f, 0.10f, 0.50f, 17, 20, false, {8, -84, 98, 66}, 260, 130, 0.0f, -1, 0),
        A("reap", 0.42f, 0.10f, 0.45f, 16, 18, false, {8, -84, 96, 66}, 220, 125, 0.7f, -1, 0),
        A("lunge", 0.65f, 0.16f, 0.70f, 26, 28, true, {0, -70, 132, 34}, 820, 280, 0.45f, -1, 2),
    }});
    {
        EDef d{EType::Lurker, "Tide Lurker", "潜", 72, 70, 90, 230, 420, 0.95f, 0.15f, 1, 34, 14, {
            ambush(A("ambush", 0.32f, 0.20f, 0.80f, 26, 0, true, {0, -112, 112, 112}, 640, 210, 0.0f, -1, 2, false, false, true)),
            A("swipe", 0.45f, 0.10f, 0.50f, 15, 16, false, {8, -80, 100, 70}, 200, 130, 1.0f, -1, 0),
            A("jab", 0.50f, 0.12f, 0.45f, 15, 16, false, {10, -70, 150, 30}, 120, 170, 0.8f, -1, 2),
        }};
        put(d);
    }
    {
        EDef d{EType::TideWarden, "Funaki, the Tide Warden", "潮", 380, 190, 110, 250, 1400, 1.5f, 0.25f, 2, 420, 360, {
            A("swing1", 0.60f, 0.14f, 0.20f, 24, 22, false, {10, -150, 196, 130}, 200, 215, 1.0f, 1, 0),
            A("swing2", 0.40f, 0.14f, 0.60f, 26, 26, false, {10, -170, 190, 150}, 180, 215, 0.0f, -1, 1),
            A("slam", 1.00f, 0.16f, 0.80f, 38, 44, false, {20, -200, 176, 200}, 160, 200, 0.7f, -1, 1),
            A("chainsweep", 0.80f, 0.20f, 0.75f, 28, 26, true, {-60, -50, 330, 50}, 100, 300, 0.6f, -1, 3),
            proj(A("tidewave", 0.75f, 0.10f, 0.70f, 22, 10, true, {0, 0, 0, 0}, 0, 1000, 0.6f, -1, 6), 3),
            A("haul", 0.70f, 0.22f, 0.90f, 30, 0, true, {0, -132, 310, 120}, 300, 340, 0.5f, -1, 2, false, false, true),
            phase(proj(A("tidewave2", 0.55f, 0.10f, 0.60f, 22, 10, true, {0, 0, 0, 0}, 0, 1000, 0.9f, -1, 6), 3, 2), 2),
            phase(A("fswing1", 0.45f, 0.14f, 0.12f, 26, 24, false, {10, -150, 200, 130}, 220, 220, 1.0f, 8, 0), 2),
            phase(A("fswing2", 0.30f, 0.14f, 0.55f, 28, 26, false, {10, -170, 196, 150}, 200, 220, 0.0f, -1, 1), 2),
        }};
        d.boss = true; d.spared = true; d.flag = "boss_warden"; d.achievement = "boss_warden"; d.reward = "warden_chain";
        d.riseColor = rgb(0x6ad8e8);
        put(d);
    }
    // ---------------------------------------------------------------- Chapter III: the ridge company
    put({EType::Duelist, "Ridge Duelist", "剣客", 96, 110, 110, 280, 600, 1.02f, 0.55f, 1, 40, 20, {
        A("cut1", 0.40f, 0.10f, 0.12f, 17, 18, false, {8, -86, 100, 70}, 220, 130, 1.0f, 1, 0),
        A("cut2", 0.22f, 0.10f, 0.50f, 18, 20, false, {8, -112, 96, 96}, 220, 140, 0.0f, -1, 1),
        A("iai", 0.70f, 0.08f, 0.60f, 30, 34, false, {0, -80, 172, 50}, 700, 300, 0.6f, -1, 0),
        A("thrust", 0.70f, 0.16f, 0.70f, 28, 32, true, {0, -68, 130, 30}, 760, 280, 0.4f, -1, 2),
    }});
    {
        EDef d{EType::CrowScout, "Crow Scout", "烏", 56, 60, 130, 330, 640, 0.92f, 0.1f, 1, 30, 14, {
            A("peck", 0.32f, 0.08f, 0.30f, 12, 12, false, {6, -84, 86, 64}, 240, 115, 1.0f, -1, 0),
            hop(A("leapcut", 0.45f, 0.30f, 0.50f, 16, 18, false, {0, -112, 112, 112}, 380, 300, 0.7f, -1, 1), 820),
            proj(A("kunai", 0.60f, 0.05f, 0.60f, 10, 8, false, {0, 0, 0, 0}, 0, 700, 0.6f, -1, 6), 4, 3),
        }};
        put(d);
    }
    {
        EDef d{EType::CrowKing, "Karasu, the Crow King", "烏王", 420, 200, 150, 360, 1400, 1.25f, 0.4f, 2, 450, 380, {
            A("cut1", 0.45f, 0.12f, 0.14f, 22, 20, false, {10, -120, 160, 100}, 260, 180, 1.0f, 1, 0),
            A("cut2", 0.25f, 0.12f, 0.14f, 22, 20, false, {10, -150, 150, 130}, 220, 180, 0.0f, 2, 1),
            A("cut3", 0.30f, 0.14f, 0.55f, 26, 26, false, {10, -120, 170, 110}, 300, 180, 0.0f, -1, 0),
            hop(A("leap", 0.40f, 0.36f, 0.00f, 0, 0, false, {0, 0, 0, 0}, 420, 650, 0.6f, 4, 1), 1250),
            dive(A("dive", 0.05f, 0.40f, 0.70f, 34, 30, true, {-50, -130, 170, 150}, 200, 650, 0.0f, -1, 1), 1600),
            proj(A("feathers", 0.60f, 0.05f, 0.60f, 14, 10, false, {0, 0, 0, 0}, 0, 900, 0.6f, -1, 6), 4, 5),
            phase(A("tempest", 0.80f, 0.20f, 0.70f, 30, 28, true, {-130, -80, 380, 80}, 120, 260, 0.6f, -1, 3), 2),
            phase(hop(A("leap2", 0.30f, 0.32f, 0.00f, 0, 0, false, {0, 0, 0, 0}, 480, 700, 0.8f, 4, 1), 1300), 2),
        }};
        d.boss = true; d.spared = true; d.flag = "boss_crowking"; d.achievement = "boss_crowking"; d.reward = "crow_mask";
        d.riseColor = rgb(0xffb070);
        put(d);
    }
    // ---------------------------------------------------------------- Chapter IV: Castle Hakuro
    {
        EDef d{EType::Hound, "Castle Hound", "犬", 46, 50, 160, 420, 520, 0.8f, 0.0f, 1, 22, 6, {
            A("bite", 0.30f, 0.10f, 0.40f, 12, 14, false, {0, -50, 92, 50}, 360, 110, 1.0f, -1, 0),
            hop(A("pounce", 0.40f, 0.26f, 0.50f, 18, 20, false, {0, -80, 112, 80}, 520, 260, 0.6f, -1, 0), 520),
        }};
        d.canGuard = false; d.sniffs = true; d.bodyW = 64; d.bodyH = 46;
        put(d);
    }
    {
        EDef d{EType::Gunner, "Castle Gunner", "鉄砲", 52, 55, 100, 220, 980, 1.0f, 0.1f, 1, 36, 18, {
            proj(A("fire", 1.35f, 0.05f, 0.90f, 34, 0, true, {0, 0, 0, 0}, 0, 1000, 1.0f, -1, 7), 5),
            A("butt", 0.35f, 0.10f, 0.45f, 10, 24, false, {6, -80, 72, 60}, 160, 90, 0.0f, -1, 0),
        }};
        d.ranged = true;
        put(d);
    }
    {
        EDef d{EType::Hakuro, "Lord Hakuro, the White Wolf", "白狼", 480, 220, 140, 330, 1400, 1.35f, 0.35f, 2, 500, 420, {
            A("c1", 0.50f, 0.12f, 0.14f, 26, 24, false, {10, -130, 180, 110}, 280, 200, 1.0f, 1, 0),
            A("c2", 0.28f, 0.12f, 0.14f, 26, 24, false, {10, -160, 170, 140}, 240, 200, 0.0f, 2, 1),
            A("c3", 0.34f, 0.14f, 0.60f, 30, 30, false, {10, -130, 190, 120}, 320, 200, 0.0f, -1, 0),
            A("fang", 0.80f, 0.30f, 0.80f, 34, 32, true, {0, -96, 170, 44}, 1250, 560, 0.6f, -1, 2),
            A("spin", 0.75f, 0.20f, 0.70f, 30, 28, true, {-90, -60, 330, 60}, 140, 240, 0.6f, -1, 3),
            A("crush", 0.95f, 0.14f, 0.75f, 38, 44, false, {20, -200, 160, 200}, 200, 190, 0.6f, -1, 1),
            phase(summon(A("howl", 1.00f, 0.10f, 0.60f, 0, 0, false, {0, 0, 0, 0}, 0, 3000, 0.35f, -1, 6)), 2),
            phase(A("fc1", 0.38f, 0.12f, 0.10f, 28, 24, false, {10, -130, 190, 110}, 300, 210, 1.0f, 8, 0), 2),
            phase(A("fc2", 0.22f, 0.12f, 0.10f, 28, 24, false, {10, -160, 180, 140}, 260, 210, 0.0f, 9, 1), 2),
            phase(A("fc3", 0.26f, 0.14f, 0.55f, 32, 30, false, {10, -130, 200, 120}, 340, 210, 0.0f, -1, 2), 2),
        }};
        d.boss = true; d.spared = true; d.flag = "boss_hakuro"; d.achievement = "boss_hakuro"; d.reward = "hakuro_fang";
        d.riseColor = rgb(0xdfe8ff);
        put(d);
    }
    // ---------------------------------------------------------------- Chapter V: the Hollow Company
    put({EType::HollowBlade, "Hollow Blade", "虚刃", 112, 110, 120, 300, 620, 1.0f, 0.35f, 1, 46, 22, {
        A("slash", 0.42f, 0.10f, 0.42f, 20, 20, false, {8, -84, 98, 68}, 220, 128, 1.0f, -1, 0),
        A("d1", 0.34f, 0.10f, 0.12f, 18, 16, false, {8, -84, 98, 68}, 180, 128, 0.8f, 2, 0),
        A("d2", 0.20f, 0.10f, 0.48f, 19, 18, false, {8, -112, 96, 94}, 200, 140, 0.0f, -1, 1),
        blink(A("veilcut", 0.55f, 0.10f, 0.50f, 22, 24, false, {8, -86, 104, 70}, 200, 420, 0.6f, -1, 0)),
        A("thrust", 0.66f, 0.16f, 0.66f, 30, 32, true, {0, -68, 130, 30}, 780, 280, 0.4f, -1, 2),
    }});
    {
        EDef d{EType::HollowSnare, "Hollow Snare", "糸使", 72, 70, 100, 230, 820, 0.98f, 0.0f, 1, 40, 18, {
            proj(A("snare", 0.80f, 0.05f, 0.70f, 10, 10, false, {0, 0, 0, 0}, 0, 900, 1.0f, -1, 6), 6),
            A("palm", 0.30f, 0.10f, 0.45f, 10, 26, false, {6, -80, 72, 60}, 150, 90, 0.0f, -1, 5),
        }};
        d.ranged = true; d.canGuard = false;
        put(d);
    }
    {
        EDef d{EType::Kagami, "Kagami of the Hollow", "鏡", 460, 210, 160, 400, 1400, 1.0f, 0.55f, 2, 520, 420, {
            A("k1", 0.22f, 0.08f, 0.12f, 18, 16, false, {6, -86, 102, 72}, 230, 130, 1.0f, 1, 0),
            A("k2", 0.16f, 0.08f, 0.12f, 18, 16, false, {6, -100, 98, 92}, 230, 130, 0.0f, 2, 1),
            A("k3", 0.20f, 0.10f, 0.45f, 24, 24, false, {4, -118, 114, 116}, 300, 140, 0.0f, -1, 0),
            blink(A("veilcut", 0.50f, 0.10f, 0.50f, 24, 26, false, {8, -86, 112, 72}, 220, 700, 0.6f, -1, 0)),
            proj(A("stars", 0.45f, 0.05f, 0.50f, 12, 10, false, {0, 0, 0, 0}, 0, 800, 0.5f, -1, 6), 4, 3),
            A("sever", 0.70f, 0.25f, 0.70f, 32, 30, true, {0, -80, 152, 40}, 1300, 560, 0.5f, -1, 2),
            phase(proj(A("starrain", 0.55f, 0.05f, 0.55f, 12, 10, false, {0, 0, 0, 0}, 0, 900, 0.6f, -1, 6), 4, 5), 2),
            phase(A("low", 0.65f, 0.18f, 0.60f, 26, 26, true, {-40, -40, 230, 40}, 160, 220, 0.6f, -1, 3), 2),
        }};
        d.boss = true; d.spared = true; d.flag = "boss_kagami"; d.achievement = "boss_kagami"; d.reward = "mirror_mask";
        d.riseColor = rgb(0xe8dcff);
        put(d);
    }
    // ---------------------------------------------------------------- Chapter VI: the heart of the Veil
    put({EType::Sentinel, "Thread Sentinel", "番人", 175, 200, 80, 170, 620, 1.45f, 0.45f, 1, 70, 30, {
        A("sweep", 0.70f, 0.20f, 0.60f, 26, 26, false, {-20, -126, 236, 112}, 140, 225, 1.0f, -1, 3),
        A("overhead", 0.90f, 0.16f, 0.80f, 34, 40, false, {20, -196, 152, 196}, 160, 190, 0.7f, -1, 1),
        A("low", 0.80f, 0.20f, 0.70f, 30, 28, true, {-60, -46, 296, 46}, 120, 245, 0.6f, -1, 3),
        A("ram", 0.85f, 0.30f, 0.80f, 30, 30, true, {0, -110, 170, 60}, 1000, 480, 0.4f, -1, 2),
    }});
    {
        EDef d{EType::Odaka, "Odaka Sen", "師", 520, 240, 150, 360, 1500, 1.12f, 0.5f, 3, 0, 0, {
            A("s1", 0.40f, 0.10f, 0.12f, 24, 22, false, {8, -110, 172, 90}, 240, 192, 1.0f, 1, 0),
            A("s2", 0.22f, 0.10f, 0.12f, 24, 22, false, {8, -140, 162, 120}, 220, 192, 0.0f, 2, 1),
            A("s3", 0.30f, 0.12f, 0.55f, 28, 28, false, {8, -110, 192, 100}, 300, 200, 0.0f, -1, 2),
            A("staffsweep", 0.75f, 0.20f, 0.70f, 30, 28, true, {-80, -50, 324, 50}, 120, 262, 0.6f, -1, 3),
            A("threadpull", 0.70f, 0.22f, 0.90f, 32, 0, true, {0, -132, 324, 120}, 300, 360, 0.5f, -1, 2, false, false, true),
            phase(proj(A("threadwave", 0.70f, 0.10f, 0.70f, 22, 10, true, {0, 0, 0, 0}, 0, 1000, 0.6f, -1, 6), 7), 2),
            phase(blink(A("veilstep", 0.45f, 0.10f, 0.50f, 26, 26, false, {8, -100, 142, 90}, 220, 720, 0.7f, -1, 0)), 2),
            phase(proj(A("unravel", 0.80f, 0.10f, 0.70f, 24, 10, true, {0, 0, 0, 0}, 0, 3000, 0.8f, -1, 6), 7, 2), 3),
            phase(proj(A("needles", 0.50f, 0.05f, 0.55f, 14, 10, false, {0, 0, 0, 0}, 0, 900, 0.6f, -1, 6), 4, 5), 3),
        }};
        d.boss = true; d.spared = true; d.flag = "boss_odaka"; d.achievement = "boss_odaka";
        d.riseColor = rgb(0xffd890);
        put(d);
    }
    return v;
}

const EDef& enemyDef(EType t) {
    static const std::vector<EDef> defs = buildDefs();
    return defs[(size_t)t];
}

EType etypeByName(const std::string& n, bool* ok) {
    static const std::map<std::string, EType> m = {
        {"swordsman", EType::Swordsman}, {"spearman", EType::Spearman}, {"archer", EType::Archer}, {"tetsu", EType::Tetsugen},
        {"reaver", EType::Reaver}, {"lurker", EType::Lurker}, {"warden", EType::TideWarden},
        {"duelist", EType::Duelist}, {"crow", EType::CrowScout}, {"crowking", EType::CrowKing},
        {"hound", EType::Hound}, {"gunner", EType::Gunner}, {"hakuro", EType::Hakuro},
        {"hollow", EType::HollowBlade}, {"snare", EType::HollowSnare}, {"kagami", EType::Kagami},
        {"sentinel", EType::Sentinel}, {"odaka", EType::Odaka}};
    auto it = m.find(n);
    if (ok) *ok = it != m.end();
    return it == m.end() ? EType::Swordsman : it->second;
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
    case 7: return P(-2, 6, 6, -4, 84, 10, 78, 20, 22, -14, -24, -8, -88);
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
    case 7: return P(-8, 6, -6, -4, 92, 4, 84, 14, 22, -14, -24, -8, -94);
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
    // summoned allies join after the loop so references stay valid
    for (auto& n : pendingSpawns) { n.id = (int)enemies.size(); enemies.push_back(n); }
    pendingSpawns.clear();
}

Enemy World::makeEnemy(EType t, Vec2 pos, float facing) {
    Enemy e;
    e.type = t;
    e.def = &enemyDef(t);
    e.pos = e.prevPos = e.home = pos;
    e.facing = facing;
    float ng = save ? (float)save->ngPlus : 0.f;
    e.hpScale = 1 + 0.5f * ng; e.dmgScale = 1 + 0.3f * ng;
    e.hp = e.def->hp * e.hpScale;
    e.deathblows = e.def->deathblows;
    e.boss = e.def->boss;
    e.patrolL = e.patrolR = pos.x;
    e.sash.init(e.pos + Vec2{0, -70}, 7, 6);
    return e;
}

static bool usable(const Enemy& e, const EAttack& a, float dist) {
    if (a.weight <= 0 || a.range < dist - 20) return false;
    if (a.minPhase > e.phase || (a.phase2 && e.phase < 2 && !e.phase2)) return false;
    if (a.summon && e.summonCd > 0) return false;   // the howl needs a long breath between calls
    if (a.blink && dist < 220) return false;        // a step through the Veil only makes sense from afar
    return true;
}

static bool chooseAttack(Enemy& e, float dist) {
    float total = 0;
    for (auto& a : e.def->attacks) if (usable(e, a, dist)) total += a.weight;
    if (total <= 0) return false;
    float r = g_aiRng.uniform() * total;
    for (size_t i = 0; i < e.def->attacks.size(); i++) {
        auto& a = e.def->attacks[i];
        if (!usable(e, a, dist)) continue;
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
        if (e.yielded) {
            // a spared champion kneels where they fell
            e.vel.x = damp(e.vel.x, 0.f, 6, dt);
            e.vel.y = std::min(1300.f, e.vel.y + 2900 * dt);
            moveBody(e.pos, e.vel, e.w(), e.h(), dt);
            enemyAnim(e, dt);
            return;
        }
        // the iron ronin lingers for his last words
        if (!e.boss || save->has("boss_outro_done")) e.dissolve += dt * 0.6f;
        if (e.dissolve < 1 && g_aiRng.chance(0.5f)) emit(PType::Ink, e.pos + Vec2{g_aiRng.range(-20, 20), -g_aiRng.range(0, 60)}, 1, 40, rgb(0x0a0204, 0.6f), 3, 0.8f, 1, -PI / 2);
        e.vel.y = std::min(1300.f, e.vel.y + 2900 * dt);
        e.vel.x = damp(e.vel.x, 0.f, 6, dt);
        moveBody(e.pos, e.vel, e.w(), 40, dt);
        enemyAnim(e, dt);
        return;
    }
    if (e.summonCd > 0) e.summonCd -= dt;
    if (e.chill > 0) {
        // Stillwater: the deflected foe moves as if through cold water and cannot steady itself
        e.chill -= dt;
        dt *= 0.65f;
        e.postureDelay = std::max(e.postureDelay, 0.3f);
        if (g_aiRng.chance(0.25f)) emit(PType::Frost, e.pos + Vec2{g_aiRng.range(-16, 16), -g_aiRng.range(10, e.h())}, 1, 30, rgb(0xbfeaff, 0.8f), 3, 0.7f);
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

    // a Mirror Step afterimage draws the eye of nearby foes
    Vec2 tp = p.pos;
    if (decoyT > 0 && (decoyPos - e.pos).len() < 560 && !e.boss) tp = decoyPos;
    float dx = tp.x - e.pos.x, dy = tp.y - e.pos.y;
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
        if (D.sniffs && dist < 150 && std::abs(dy) < 90 && !p.cloaked) rate = std::max(rate, 1.6f);   // a hound's nose ignores grass
        if (e.lurking) rate = (dist < 150 && std::abs(dy) < 140 && !p.hidden) ? 50.f : 0.f;
        if (e.sleeping) rate *= 0.3f;
        if (rate > 0) { e.awareness += rate * dt; e.lastKnown = p.pos; }
        else if (e.awareness < 1) e.awareness = std::max(0.f, e.awareness - 0.12f * dt);
        if (e.awareness >= 1) {
            e.awareness = 1;
            e.sleeping = false;
            if (e.lurking) {
                // erupt from the water straight into the ambush
                e.lurking = false;
                for (size_t ai = 0; ai < D.attacks.size(); ai++) if (D.attacks[ai].ambush) e.atk = (int)ai;
                e.facing = sign(dx) != 0 ? sign(dx) : e.facing;
                emit(PType::Splash, e.pos + Vec2{0, -e.h() * 0.7f}, 30, 420, palette.water * 2.2f, 5, 0.7f, PI, -PI / 2);
                sfx(Sfx::Swoosh, e.pos, 1.0f, 0.7f);
                if (e.atk >= 0) { setE(e, EState::Windup); e.alertIcon = 1; return; }
            }
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
        bool ground = level.solidAt(tx, ty) || t == Tile::OneWay || (e.type == EType::Lurker && level.waterAt(tx, ty - 1));
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

        if (D.ranged) {
            if (dist < 85 && e.cooldown <= 0) { e.atk = 1; setE(e, EState::Windup); break; }
            if (dist < 280) {
                // retreat while keeping eyes on the target
                float away = -sign(dx);
                if (groundAhead(away)) e.vel.x = approach(e.vel.x, away * D.run * 0.8f, 1600 * dt);
                else e.vel.x = approach(e.vel.x, 0, 2000 * dt);
                e.facing = sign(dx);
            } else e.vel.x = approach(e.vel.x, 0, 2000 * dt);
            if (e.cooldown <= 0 && los && dist < D.attacks[0].range && dist > 120) {
                e.atk = 0; setE(e, EState::Windup);
                if (e.type == EType::Archer) sfx(Sfx::BowDraw, e.pos, 0.7f);
                else if (e.type == EType::Gunner) sfx(Sfx::Ignite, e.pos, 0.35f, 1.8f);   // the match touches the pan
                else sfx(Sfx::Swoosh, e.pos, 0.5f, 1.4f);
            }
            break;
        }
        if (!e.hasToken && (tokensInUse < 2 || e.boss) && e.cooldown <= 0) { e.hasToken = true; tokensInUse++; }
        if (e.hasToken) {
            if (e.cooldown <= 0 && std::abs(dy) < 120 && chooseAttack(e, dist)) { setE(e, EState::Windup); break; }
            float want = e.boss ? 150.f : 105.f;
            if (dist > want) walkToward(tp.x, D.run);
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
        if (e.stT - dt <= 0 && a.blink && playerAlive) {
            // step through the Veil: reappear behind the target
            float bx = p.pos.x - p.facing * 96;
            Rect nb{bx - e.w() / 2, p.pos.y - e.h(), e.w(), e.h()};
            if (!rectSolid(nb) && onGroundAt({bx, p.pos.y}, e.w())) {
                emit(PType::Thread, e.pos + Vec2{0, -e.h() * 0.5f}, 24, 300, palette.thread, 2.5f, 0.6f);
                e.pos.x = e.prevPos.x = bx; e.pos.y = e.prevPos.y = p.pos.y;
                e.facing = p.facing;
                emit(PType::Thread, e.pos + Vec2{0, -e.h() * 0.5f}, 24, 300, palette.thread, 2.5f, 0.6f);
                sfx(Sfx::Dash, e.pos, 0.8f, 0.85f);
                ripple = std::max(ripple, 0.5f); rippleAt = e.pos + Vec2{0, -50};
            }
        }
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
            if (a.hop > 0) { e.vel.y = -a.hop; e.vel.x = e.facing * a.lunge; sfx(Sfx::Jump, e.pos, 0.7f, 0.8f); }
            if (a.dive > 0) { e.vel.y = a.dive; e.vel.x *= 0.3f; }
            if (a.summon) {
                // the White Wolf calls his hounds
                for (float side : {-1.f, 1.f}) {
                    Enemy h = makeEnemy(EType::Hound, e.pos + Vec2{side * 5 * TILE, -4}, -side);
                    if (rectSolid(Rect{h.pos.x - h.w() / 2, h.pos.y - h.h(), h.w(), h.h()})) h.pos = h.prevPos = e.pos;
                    h.awareness = 1; h.st = EState::Engage; h.summoned = true; h.lastKnown = p.pos; h.flash = 1;
                    pendingSpawns.push_back(h);
                    emit(PType::Snow, h.pos + Vec2{0, -30}, 30, 300, rgb(0xffffff), 4, 0.8f);
                }
                e.summonCd = 14;
                shake(0.4f);
                sfx(Sfx::Perilous, e.pos, 0.7f, 0.6f);
            }
            if (a.projectile) {
                Vec2 tgt = tp + Vec2{0, -50};
                auto shot = [&](ProjType type, Vec2 from, Vec2 vel, float life) {
                    Projectile pr; pr.fromPlayer = false; pr.type = type;
                    pr.pos = pr.prevPos = from; pr.vel = vel; pr.life = life;
                    pr.damage = a.damage * e.dmgScale; pr.posture = a.posture;
                    projectiles.push_back(pr);
                };
                switch (a.proj) {
                case 2: case 3: case 7: {
                    ProjType ty = a.proj == 2 ? ProjType::FlameWave : (a.proj == 3 ? ProjType::TideWave : ProjType::ThreadWave);
                    for (int k = 0; k < a.projCount; k++) {
                        float dir = (a.projCount == 2 && k == 1) ? -e.facing : e.facing;
                        shot(ty, Vec2{e.pos.x + dir * 80, e.pos.y}, {dir * (a.proj == 3 ? 600.f : 640.f), 0}, 2.6f);
                    }
                    sfx(a.proj == 2 ? Sfx::Ember : Sfx::Swoosh, e.pos, 1.0f, a.proj == 2 ? 0.8f : 0.6f);
                    shake(0.35f);
                    break;
                }
                case 4: {
                    Vec2 from = e.pos + Vec2{e.facing * 20, -70};
                    Vec2 d = (tgt - from).norm();
                    float base = std::atan2(d.y, d.x);
                    int n = a.projCount;
                    for (int k = 0; k < n; k++) {
                        float ang = base + (n > 1 ? (k - (n - 1) * 0.5f) * 0.16f : 0.f);
                        shot(ProjType::Kunai, from, Vec2{std::cos(ang), std::sin(ang)} * 980, 1.4f);
                    }
                    sfx(Sfx::Throw, e.pos, 0.9f, 0.85f);
                    break;
                }
                case 5: {
                    Vec2 from = e.pos + Vec2{e.facing * 64, -70};
                    shot(ProjType::Ball, from, (tgt - from).norm() * 2600, 1.0f);
                    sfx(Sfx::HitHeavy, e.pos, 1.0f, 1.6f);
                    emit(PType::Smoke, from, 14, 160, rgb(0xb8b8c0, 0.5f), 18, 1.2f, 0.8f, e.facing > 0 ? 0 : PI);
                    emit(PType::Spark, from, 10, 600, rgb(0xffd080), 2, 0.2f, 0.5f, e.facing > 0 ? 0 : PI);
                    shake(0.25f);
                    break;
                }
                case 6: {
                    Vec2 from = e.pos + Vec2{e.facing * 24, -70};
                    shot(ProjType::Snare, from, (tgt - from).norm() * 640, 2.0f);
                    sfx(Sfx::Swoosh, e.pos, 0.8f, 1.5f);
                    break;
                }
                default: {
                    Vec2 from = e.pos + Vec2{e.facing * 24, -64};
                    Vec2 d = tgt - from;
                    float spd = 1100, tt = d.len() / spd;
                    Vec2 v = d.norm() * spd; v.y -= 0.5f * 300 * tt;
                    shot(ProjType::Arrow, from, v, 2.5f);
                    sfx(Sfx::Arrow, e.pos, 0.8f);
                    break;
                }
                }
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
        {
            PType pt = e.type == EType::Tetsugen ? PType::Ember : (e.type == EType::TideWarden ? PType::Splash : (e.type == EType::Hakuro ? PType::Snow : PType::Thread));
            if (e.stT > 0.6f && g_aiRng.chance(0.5f)) emit(pt, e.pos + Vec2{g_aiRng.range(-40, 40), -g_aiRng.range(0, e.h())}, 1, 120, D.riseColor, 5, 0.8f, 1, -PI / 2);
            if (e.stT > 3.2f) {
                setE(e, EState::Engage);
                e.cooldown = 0.6f;
                sfx(e.type == EType::Tetsugen ? Sfx::Ignite : Sfx::Swoosh, e.pos, 1.0f, e.type == EType::Tetsugen ? 1.f : 0.6f);
                emit(pt, e.pos + Vec2{0, -60}, 60, 500, D.riseColor, 6, 0.9f);
                shake(0.6f);
                flash = settings && settings->reduceFlashes ? 0.1f : 0.35f; flashColor = D.riseColor;
            }
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
    bool spear = e.type == EType::Spearman || e.type == EType::Sentinel || e.type == EType::Lurker, bow = e.type == EType::Archer;
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
    case EState::Dead: target = e.yielded ? eposes::kneelP : eposes::deadP; break;
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
