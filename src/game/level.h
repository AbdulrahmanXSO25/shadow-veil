#pragma once
#include "core/math.h"
#include "render/renderer.h"
#include <string>
#include <vector>
#include <map>

namespace sv {

constexpr float TILE = 48.f;

enum class Tile : uint8_t { Empty, Solid, Plank, OneWay, Spikes, FalseWall, Breakable, Broken, Wall };

enum class BgStyle { Cliff, Village, Grove, Ravine, Gate };

// Everything that defines a room's look. Rooms blend palettes as the camera crosses boundaries.
struct Palette {
    SkyParams sky;
    Color fog{0.2f, 0.2f, 0.3f};
    Color far{0.12f, 0.13f, 0.22f}, mid{0.07f, 0.08f, 0.14f}, nearC{0.03f, 0.035f, 0.06f};
    Color ground{0.02f, 0.02f, 0.03f};
    Color grass{0.03f, 0.04f, 0.05f};
    Color rim{0.55f, 0.65f, 0.95f};
    Color glow{1.0f, 0.55f, 0.2f};     // ambient light colour from fires/windows
    float glowAmt = 0;                 // how strongly the horizon & windows burn
    PostParams post;
    float embers = 0, ash = 0, fireflies = 0, leaves = 0, petals = 0, mist = 0.3f;
    Vec2 wind{30, 0};
};
Palette lerp(const Palette& a, const Palette& b, float t);
const Palette& paletteByName(const std::string& name);
BgStyle styleByName(const std::string& name);

struct Room {
    std::string id, name;
    int x0 = 0, w = 0;               // tile columns
    std::string palette = "moon";
    BgStyle style = BgStyle::Cliff;
    float zoom = 1.0f;
    std::string music = "explore";
    float wind = 0.3f, fire = 0, crickets = 0;
    bool bossArena = false;
    bool stealthZone = false;
    int seed = 1;
};

struct Spawn {
    std::string kind;                // player, checkpoint, swordsman, spearman, archer, boss, anchor, mon, shuriken...
    Vec2 pos;                        // world px: bottom-centre of the tile for actors
    int room = 0;
    std::map<std::string, std::string> params;
    std::string id;                  // stable id for persistence (e.g. "c1_chest_3")
    std::string get(const std::string& k, const std::string& def = "") const {
        auto it = params.find(k); return it == params.end() ? def : it->second;
    }
    float getf(const std::string& k, float def) const {
        auto it = params.find(k); return it == params.end() ? def : std::stof(it->second);
    }
};

struct Prop {
    std::string kind;
    Vec2 pos;                        // world px (bottom-left of anchor tile)
    float w = 1, h = 1;              // tiles
    int seed = 0;
    int room = 0;
    std::map<std::string, std::string> params;
};

struct Trigger {
    Rect area;
    std::string action;              // e.g. "cutscene intro", "hint parry", "music combat"
    bool once = true;
    std::string id;
    int room = 0;
};

struct Level {
    std::string id, title, kanji;
    int width = 0, height = 0;
    std::vector<Tile> tiles;
    std::vector<uint8_t> grass;      // tall-grass mask
    std::vector<Room> rooms;
    std::vector<Spawn> spawns;
    std::vector<Prop> props;
    std::vector<Trigger> triggers;

    Tile at(int x, int y) const {
        if (x < 0 || x >= width) return Tile::Solid;
        if (y < 0) return Tile::Empty;
        if (y >= height) return Tile::Solid;
        return tiles[y * width + x];
    }
    void set(int x, int y, Tile t) { if (x >= 0 && x < width && y >= 0 && y < height) tiles[y * width + x] = t; }
    bool grassAt(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height && grass[y * width + x]; }
    bool solidAt(int x, int y) const { Tile t = at(x, y); return t == Tile::Solid || t == Tile::Plank || t == Tile::Breakable || t == Tile::Wall; }
    float pixelWidth() const { return width * TILE; }
    float pixelHeight() const { return height * TILE; }
    int roomAt(float worldX) const;
    const Spawn* findSpawn(const std::string& kind) const;
    const Spawn* findById(const std::string& id) const;
};

// Parses the room-based level format (see data/levels/README.md). Returns false with an error message.
bool parseLevel(const std::string& text, Level& out, std::string& error);
bool loadLevel(const std::string& path, Level& out, std::string& error);

} // namespace sv
