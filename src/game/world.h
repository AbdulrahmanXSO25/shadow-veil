#pragma once
#include "core/math.h"
#include "core/input.h"
#include "audio/audio.h"
#include "game/level.h"
#include "game/skeleton.h"
#include "game/progression.h"
#include <vector>
#include <deque>
#include <string>
#include <memory>

namespace sv {

class Renderer;
struct World;

constexpr float kDt = 1.0f / 120.0f;
// Gameplay framing: all camera zooms are relative to this.
constexpr float kBaseZoom = 1.25f;

// ------------------------------------------------------------------ combat primitives

struct AttackInfo {
    float damage = 0, posture = 0;
    bool perilous = false;
    bool projectile = false;
    bool ember = false;
    bool grab = false;
    int sourceEnemy = -1;   // enemy index, -1 = player
    Vec2 origin;
    float knock = 300;
};

enum class HitResult { None, Hit, Blocked, Deflected, Dodged, Killed, Broken };

// ------------------------------------------------------------------ effects

enum class PType : uint8_t { Spark, Ink, Ember, Ash, Leaf, Petal, Firefly, Smoke, Dust, Flash, Mist, Thread, Rain, Snow, Splash, Feather, Frost };

struct Particle {
    Vec2 pos, vel;
    float life = 1, maxLife = 1, size = 4, rot = 0, rotVel = 0, drag = 0, gravity = 0;
    Color col{1, 1, 1, 1};
    PType type = PType::Spark;
    bool fg = false;   // drawn in front of actors
};

struct TrailSample { Vec2 base, tip; float t; };

struct Afterimage { Joints j; float life; float facing; };

struct Camera {
    Vec2 pos, prevPos, target;
    float zoom = 1, targetZoom = 1;
    float trauma = 0;
    Vec2 shakeOffset;
    float shakeRot = 0;
    float lookAhead = 0;
    bool cinematic = false;            // cutscene has control
    Vec2 cinFrom, cinTo; float cinZoomFrom = 1, cinZoomTo = 1, cinT = 0, cinDur = 0;
    float punch = 0;                   // brief zoom-in kick on impacts
    Vec2 peek;                         // right-stick look-around
    float zoomBias = 1;                // directed framing (trailer capture)
};

// ------------------------------------------------------------------ player

enum class PState { Ground, Air, WallSlide, Dash, Attack, AirAttack, Guard, Hurt, Grapple, Heal, Throw, Cast, Execute, StealthKill, Grabbed, Locked, Dead };

struct Player {
    Vec2 pos, prevPos, vel;
    float w = 26, h = 82;
    float facing = 1;
    PState state = PState::Air;
    float stateT = 0;
    bool onGround = false, crouch = false;
    int wallDir = 0;
    float coyote = 0, wallJumpLock = 0;
    int airDashes = 1;
    float dashCd = 0;
    float hp = 100, ki = 0;
    float iframes = 0;
    float deflectT = 0, lastGuardPress = -10, guardT = 0;
    int combo = 0;
    bool comboQueued = false, hitDone = false;
    int swingId = 0;
    std::vector<int> swingHits;
    float airAttackCd = 0;
    // grapple
    Vec2 anchor; float ropeLen = 0, ropeT = 0; int anchorIdx = -1;
    // execution / stealth
    int target = -1;
    // stealth state
    bool inGrass = false, hidden = false;
    float noise = 0;
    // animation
    Pose pose, fromPose;
    float blend = 1, blendDur = 0.08f;
    float runPhase = 0, landT = 0, idleT = 0;
    Cloth scarf, tailA, tailB;
    std::deque<TrailSample> trail;
    std::vector<Afterimage> afterimages;
    Vec2 lastSafe;
    float lastHitT = -10;
    bool healDone = false;
    bool tookDamageInBoss = false;
    std::string lockedPose;            // cutscene-driven pose
    float hurtFlash = 0;
    float deathT = 0;
    // environment
    bool inWater = false, headUnder = false, inUpdraft = false;
    float strokeCd = 0;
    float crumbleT = 0;
    // stances
    float stillT = 0;          // Shadow Stance: crouched stillness builds into a cloak
    bool cloaked = false;
    float snared = 0;          // bound by a Hollow thread
};

// ------------------------------------------------------------------ enemies

enum class EType {
    Swordsman, Spearman, Archer, Tetsugen,          // I
    Reaver, Lurker, TideWarden,                     // II
    Duelist, CrowScout, CrowKing,                   // III
    Hound, Gunner, Hakuro,                          // IV
    HollowBlade, HollowSnare, Kagami,               // V
    Sentinel, Odaka,                                // VI
    Count
};
EType etypeByName(const std::string& n, bool* ok = nullptr);
enum class EState { Patrol, Suspicious, Search, Engage, Windup, Attack, Recover, Guarding, Stunned, Broken, BeingKilled, Dead, Kneel, Idle };

struct EAttack {
    std::string name;
    float windup, active, recover;
    float damage, posture;
    bool perilous = false;
    Rect hit;          // relative: x forward from feet, y up is negative
    float lunge = 0;
    float range = 120;
    float weight = 1;
    int chain = -1;    // next attack index in a string
    bool projectile = false;
    bool phase2 = false;
    bool grab = false;
    int poseSet = 0;   // which key poses to use (0 horizontal, 1 overhead, 2 thrust, 3 sweep, 4 bow, 5 kick, 6 wave, 7 aim)
    // special behaviour
    int proj = 0;      // projectile kind: 1 arrow, 2 flame wave, 3 tide wave, 4 kunai fan, 5 musket ball, 6 thread snare, 7 thread wave
    int projCount = 1;
    float hop = 0;     // leaps into the air as the blow starts
    float dive = 0;    // drives straight down (aerial follow-up)
    bool blink = false;// steps through the Veil to appear behind the target as the wind-up starts
    bool summon = false;
    bool ambush = false;
    int minPhase = 1;
};

struct EDef {
    EType type;
    std::string name, kanji;
    float hp, posture, walk, run, sight, scale;
    float guardChance;
    int deathblows = 1;
    int essence = 20, mon = 10;
    std::vector<EAttack> attacks;
    // bosses and specials
    bool boss = false;
    bool spared = false;               // yields instead of dying on the final deathblow
    std::string flag, achievement, reward;
    bool canGuard = true;
    bool sniffs = false;               // hounds find you in grass and shadow
    bool ranged = false;               // keeps distance; attack 0 is the shot, attack 1 the close-range answer
    float bodyW = 0, bodyH = 0;        // hitbox override (hounds)
    Color riseColor{1.0f, 0.48f, 0.16f}; // colour of the rise between deathblows
};
const EDef& enemyDef(EType t);

struct Enemy {
    int id = 0;
    EType type = EType::Swordsman;
    const EDef* def = nullptr;
    std::string spawnId;
    Vec2 pos, prevPos, vel, home;
    float facing = -1;
    EState st = EState::Patrol;
    float stT = 0;
    float hp = 60, posture = 0, postureDelay = 0;
    int deathblows = 1;
    float awareness = 0;
    Vec2 lastKnown;
    float lostT = 0;
    int atk = -1;
    bool hitDone = false;
    bool hasToken = false;
    float cooldown = 0, thinkT = 0, strafeDir = 1;
    float patrolL = 0, patrolR = 0, patrolWait = 0;
    int blocksInRow = 0, hitsTaken = 0;
    bool phase2 = false;
    bool boss = false;
    float flash = 0, alertIcon = 0, perilousIcon = 0, glint = 0;
    float dissolve = 0;
    float burn = 0;
    float hpScale = 1, dmgScale = 1;
    bool countedDead = false;
    bool sleeping = false;
    bool lurking = false;              // submerged, waiting to ambush
    bool summoned = false;
    float chill = 0;                   // Stillwater: slowed and unable to recover posture
    int phase = 1;
    bool yielded = false;
    float summonCd = 0;                // seconds until the next summon is allowed
    Pose pose, fromPose;
    float blend = 1;
    float walkPhase = 0;
    Cloth sash;
    std::deque<TrailSample> trail;
    bool alive() const { return st != EState::Dead && st != EState::BeingKilled; }
    float w() const { return def->bodyW > 0 ? def->bodyW : 30 * def->scale; }
    float h() const { return def->bodyH > 0 ? def->bodyH : 84 * def->scale; }
    Rect box() const { return {pos.x - w() / 2, pos.y - h(), w(), h()}; }
};

// ------------------------------------------------------------------ projectiles, pickups, mechanisms

enum class ProjType { Shuriken, Arrow, Ember, FlameWave, Gale, TideWave, Kunai, Ball, Snare, ThreadWave };
struct Projectile {
    ProjType type;
    Vec2 pos, prevPos, vel;
    float life = 3;
    bool fromPlayer = true;
    float damage = 0, posture = 0;
    bool reflected = false, dead = false, stuck = false;
    std::vector<int> hits;
    float rot = 0;
};

struct Pickup {
    std::string kind, item, spawnId;
    Vec2 pos;
    int amount = 1;
    float bob = 0;
    bool taken = false;
};

enum class MechKind { Gate, Bell, Rope, Bridge, Chest, Scroll, Shrine, Examine, Npc, Trial, Sluice, Drain, Beacon };
struct Mechanism {
    MechKind kind;
    std::string id, link, item, text, spawnId, name;
    Vec2 pos;
    float w = 1, h = 1;          // tiles
    float t = 0;                 // animation progress
    bool active = false;         // opened/rung/cut/dropped/collected
    bool solid = false;
    float swing = 0, swingVel = 0;
    float timer = 0, range = 6, speed = 1;   // drains, beacons
    Rect area;                               // drain region (world px)
    Rect solidRect() const;
};

struct Npc {
    std::string id, kind;
    Vec2 pos, prevPos;
    float facing = 1;
    std::string pose = "idle";
    float walkTo = 0, walkSpeed = 0;
    bool walking = false, visible = true;
    float t = 0, alpha = 1;
    Cloth cloth;
};

struct WorldEvent {
    enum Type { Toast, Achievement, Hint, Cutscene, ShrineMenu, Dialogue, Death, BossStart, BossDefeated, ChapterEnd, Lore, Checkpoint } type;
    std::string a, b;
    float num = 0;
};

// ------------------------------------------------------------------ world

struct World {
    Level level;
    SaveData* save = nullptr;
    const Settings* settings = nullptr;
    Audio* audio = nullptr;
    Input* input = nullptr;
    Stats stats;

    Player player;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<Pickup> pickups;
    std::vector<Mechanism> mechs;
    std::vector<Npc> npcs;
    std::vector<Vec2> anchors;
    std::vector<Particle> particles;
    std::vector<WorldEvent> events;
    std::vector<bool> triggerFired;
    Camera cam;

    float time = 0;
    float hitstop = 0;
    float slowmo = 0, slowmoScale = 1;
    float flash = 0, chroma = 0, desat = 0, ripple = 0; Vec2 rippleAt;
    Color flashColor{1, 1, 1};
    bool controlLocked = false;      // cutscenes
    bool bossActive = false;
    int bossIdx = -1;
    float bossIntroT = 0;
    int curRoom = 0;
    float roomBlend = 0;
    Palette palette;
    float combatIntensity = 0;
    bool groveSpotted = false;
    float interactCooldown = 0;
    std::string prompt;              // contextual prompt text
    Vec2 promptPos;
    float playerDamageTakenBoss = 0;
    int tokensInUse = 0;
    MusicMode musicOverride = MusicMode::Silent; bool hasMusicOverride = false;

    // Kage Rank: a style meter that rewards varied, flawless play
    enum StyleAct { SHit, SDeflect, SExecute, SStealth, SDodge, SProjectile, SKill, SGrapple };
    float style = 0, styleIdle = 0, stylePop = 0;
    int styleRank = 0;
    std::deque<int> styleRecent;
    void addStyle(StyleAct kind, float amount);
    void loseStyle(float amount);
    static const char* rankLetter(int r);
    static const char* rankName(int r);
    static float rankThreshold(int r);

    // Wind-beacon trials (timed combat challenges that awaken Stances)
    bool trialActive = false;
    float trialT = 0, trialLimit = 0;
    std::vector<int> trialEnemies;
    std::string trialMech;
    void startTrial(Mechanism& m);
    void updateTrial(float dt);
    int trialRemaining() const;
    bool hasGale() const { return save && save->has("gale"); }
    bool hasStance(const char* f) const { return save && save->has(f); }
    void spawnGaleBlade(float dmgScale);

    // stances & arts earned in later chapters
    void thunderStrike(Vec2 at);
    struct Bolt { Vec2 from, to; float life; int seed; };
    std::vector<Bolt> bolts;
    Vec2 decoyPos; float decoyT = 0;        // Mirror Step afterimage that draws attacks

    // Mirror Province layers
    float layerBlend = 0;
    int layerFlips = 0;
    bool tryFlipLayer();

    // environment
    std::vector<std::pair<int, float>> crumbling;   // tile index, seconds until it re-forms
    void updateEnvironment(float dt);
    void resetEnvironment();
    void floodRect(const Rect& area, int topRow);
    struct FloodJob { Rect area; int row; float t; bool fill; };
    std::vector<FloodJob> floods;
    float spottedT = 0;                      // a beacon's light found you
    bool beaconSpotted = false;

    // the unravelling (Chapter VI chase)
    bool chaseActive = false;
    float chaseX = 0, chaseSpeed = 0;
    bool chaseHit = false;

    // bosses
    std::vector<Enemy> pendingSpawns;
    Enemy makeEnemy(EType t, Vec2 pos, float facing);
    std::string chapterFlagPrefix;           // e.g. "c2" — scopes scroll achievements

    bool init(const std::string& levelPath, SaveData* save, const Settings* settings, Audio* audio, Input* input, std::string& err);
    void spawnFromLevel();
    void respawnAtCheckpoint();
    void update(float dt);
    void render(Renderer& r, float alpha);
    void renderWorldUI(Renderer& r, float alpha);

    // physics
    bool solidTile(int tx, int ty) const;
    bool rectSolid(const Rect& r, bool includeOneWay = false, float prevBottom = 1e9f) const;
    struct MoveResult { bool hitX = false, hitY = false, ground = false, ceiling = false; int wall = 0; };
    MoveResult moveBody(Vec2& pos, Vec2& vel, float w, float h, float dt, bool dropThrough = false);
    bool lineOfSight(Vec2 a, Vec2 b) const;
    bool onGroundAt(Vec2 pos, float w) const;

    // services
    void sfx(Sfx s, Vec2 at, float vol = 1, float pitch = 1);
    void shake(float amount);
    void hitStop(float secs);
    void emit(PType t, Vec2 pos, int n, float speed, Color col, float size = 4, float life = 0.6f, float spread = TAU, float dir = 0);
    void inkBurst(Vec2 pos, Vec2 dir, int n);
    void achieve(const std::string& id);
    void event(WorldEvent::Type t, const std::string& a = "", const std::string& b = "", float num = 0);
    void gainKi(float base);
    void gainEssence(int n);

    // combat
    HitResult hitPlayer(const AttackInfo& a);
    HitResult hitEnemy(int idx, const AttackInfo& a);
    void killEnemy(int idx, bool execution, bool stealth);
    void playerAttackBoxes();
    void activateLink(const std::string& link);
    void startBoss();

    // subsystems
    void updatePlayer(float dt);
    void updateEnemies(float dt);
    void updateEnemy(Enemy& e, int idx, float dt);
    void updateProjectiles(float dt);
    void updateMechs(float dt);
    void updateParticles(float dt);
    void updateCamera(float dt);
    void updateTriggers();
    void updateAmbient(float dt);
    void playerAnim(float dt);
    void enemyAnim(Enemy& e, float dt);
    int findExecutable() const;
    int findStealthTarget() const;
    int findAnchor() const;
    Npc* npc(const std::string& id);
    Mechanism* mech(const std::string& id);
    int roomIndexAt(float x) const { return level.roomAt(x); }
    Rect playerBox() const { return {player.pos.x - player.w / 2, player.pos.y - player.h, player.w, player.h}; }

    // rendering helpers (render_world.cpp)
    void drawBackground(Renderer& r, Vec2 camPos, float zoom);
    void drawTiles(Renderer& r, const Rect& view);
    void drawProps(Renderer& r, const Rect& view, bool front);
    void drawMechs(Renderer& r);
    void drawPlayer(Renderer& r, float alpha);
    void drawEnemy(Renderer& r, const Enemy& e, float alpha);
    void drawNpc(Renderer& r, const Npc& n, float alpha);
    void drawParticles(Renderer& r, bool fg);
    void drawProjectiles(Renderer& r, float alpha);
    void drawForeground(Renderer& r, Vec2 camPos, float zoom);
    void drawGrass(Renderer& r, const Rect& view, bool front);
    void drawWater(Renderer& r, const Rect& view);
    void drawUpdrafts(Renderer& r, const Rect& view);
    void drawHound(Renderer& r, const Enemy& e, float alpha);
    void drawEffects(Renderer& r);
    Color rimAt(Vec2 p) const;
    void buildBackground();

    struct BgVert { Vec2 p; float shade, alpha; };
    struct BgLayer { float parallax; int colorKey; float fogMix; std::vector<BgVert> tris; std::vector<std::pair<Vec2, float>> windows; float yOffset; };
    std::vector<BgLayer> bgLayers;
    struct Light { Vec2 pos; Color col; float radius; float flicker; };
    std::vector<Light> lights;
};

Pose npcPose(const std::string& kind, const std::string& name, float t);

} // namespace sv
