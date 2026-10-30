#include "game/level.h"
#include "core/util.h"
#include <sstream>

namespace sv {

// ------------------------------------------------------------------ palettes (the colour script)

static Palette makeMoon() {
    Palette p;
    p.sky.top = rgb(0x04060e); p.sky.mid = rgb(0x111a35); p.sky.horizon = rgb(0x3b4170);
    p.sky.horizonY = 0.78f; p.sky.moonPos = {0.70f, 0.22f}; p.sky.moonRadius = 0.085f;
    p.sky.moon = rgb(0xdfe6ff) * 0.9f; p.sky.stars = 1.0f; p.sky.cloud = rgb(0x1a2142, 0.55f);
    p.fog = rgb(0x2c3360); p.far = rgb(0x1c2347); p.mid = rgb(0x111530); p.nearC = rgb(0x080a16);
    p.ground = rgb(0x05060c); p.grass = rgb(0x0b0f1d); p.rim = rgb(0x93a9ff);
    p.glow = rgb(0xff7a33); p.glowAmt = 0.15f;
    p.post.lift = {0.008f, 0.012f, 0.03f}; p.post.gain = {0.96f, 1.0f, 1.08f}; p.post.saturation = 0.92f;
    p.post.bloom = 0.55f; p.post.vignette = 0.42f;
    p.mist = 0.45f; p.leaves = 0.25f; p.embers = 0.06f; p.wind = {40, 0};
    return p;
}
static Palette makeEmber() {
    Palette p;
    p.sky.top = rgb(0x0b0610); p.sky.mid = rgb(0x2c1016); p.sky.horizon = rgb(0xb4431e);
    p.sky.horizonY = 0.8f; p.sky.moonPos = {0.22f, 0.2f}; p.sky.moonRadius = 0.06f;
    p.sky.moon = rgb(0xffb08a) * 0.8f; p.sky.moonVeil = 0.8f; p.sky.stars = 0.15f; p.sky.cloud = rgb(0x1c0d0d, 0.85f);
    p.fog = rgb(0x5c2617); p.far = rgb(0x3b1712); p.mid = rgb(0x210c0a); p.nearC = rgb(0x0d0506);
    p.ground = rgb(0x070303); p.grass = rgb(0x140907); p.rim = rgb(0xff8a3d);
    p.glow = rgb(0xff651c); p.glowAmt = 1.0f;
    p.post.lift = {0.03f, 0.005f, 0.0f}; p.post.gain = {1.08f, 0.98f, 0.9f}; p.post.saturation = 1.05f;
    p.post.bloom = 0.85f; p.post.vignette = 0.45f;
    p.embers = 1.0f; p.ash = 0.6f; p.mist = 0.2f; p.wind = {70, -10};
    return p;
}
static Palette makeGrove() {
    Palette p;
    p.sky.top = rgb(0x02080a); p.sky.mid = rgb(0x0c2426); p.sky.horizon = rgb(0x31615a);
    p.sky.horizonY = 0.8f; p.sky.moonPos = {0.64f, 0.17f}; p.sky.moonRadius = 0.07f;
    p.sky.moon = rgb(0xe4f6ea) * 0.85f; p.sky.stars = 0.6f; p.sky.cloud = rgb(0x0e2426, 0.5f);
    p.fog = rgb(0x1f423f); p.far = rgb(0x143230); p.mid = rgb(0x0b1f1e); p.nearC = rgb(0x051110);
    p.ground = rgb(0x020706); p.grass = rgb(0x071513); p.rim = rgb(0xa3e6cc);
    p.glow = rgb(0xffd27a); p.glowAmt = 0.1f;
    p.post.lift = {0.0f, 0.01f, 0.012f}; p.post.gain = {0.93f, 1.02f, 1.0f}; p.post.saturation = 0.86f;
    p.post.bloom = 0.6f; p.post.vignette = 0.5f;
    p.fireflies = 1.0f; p.leaves = 0.7f; p.mist = 0.75f; p.wind = {25, 0};
    return p;
}
static Palette makeRavine() {
    Palette p;
    p.sky.top = rgb(0x07050f); p.sky.mid = rgb(0x1f1632); p.sky.horizon = rgb(0x5d4668);
    p.sky.horizonY = 0.75f; p.sky.moonPos = {0.8f, 0.28f}; p.sky.moonRadius = 0.075f;
    p.sky.moon = rgb(0xe8dcff) * 0.85f; p.sky.moonVeil = 0.3f; p.sky.stars = 0.5f; p.sky.cloud = rgb(0x221a31, 0.7f);
    p.fog = rgb(0x3e3155); p.far = rgb(0x2b2242); p.mid = rgb(0x181228); p.nearC = rgb(0x0b0814);
    p.ground = rgb(0x05030a); p.grass = rgb(0x0f0b17); p.rim = rgb(0xc8acff);
    p.glow = rgb(0xff8a50); p.glowAmt = 0.25f;
    p.post.lift = {0.012f, 0.005f, 0.025f}; p.post.gain = {1.0f, 0.97f, 1.06f}; p.post.saturation = 0.9f;
    p.post.bloom = 0.6f; p.post.vignette = 0.45f;
    p.mist = 0.95f; p.ash = 0.3f; p.leaves = 0.3f; p.wind = {55, 0};
    return p;
}
static Palette makeGate() {
    Palette p;
    p.sky.top = rgb(0x0b0406); p.sky.mid = rgb(0x2a0b10); p.sky.horizon = rgb(0x6e2024);
    p.sky.horizonY = 0.78f; p.sky.moonPos = {0.5f, 0.24f}; p.sky.moonRadius = 0.13f;
    p.sky.moon = rgb(0xff5a40) * 0.85f; p.sky.moonVeil = 0.3f; p.sky.stars = 0.1f; p.sky.cloud = rgb(0x1b0507, 0.7f);
    p.fog = rgb(0x5a2a2a); p.far = rgb(0x3a1a1c); p.mid = rgb(0x24100f); p.nearC = rgb(0x0c0406);
    p.ground = rgb(0x060203); p.grass = rgb(0x140608); p.rim = rgb(0xffb070);
    p.glow = rgb(0xff3a1a); p.glowAmt = 0.6f;
    p.post.lift = {0.02f, 0.0f, 0.005f}; p.post.gain = {1.06f, 0.95f, 0.93f}; p.post.saturation = 0.92f;
    p.post.bloom = 0.75f; p.post.vignette = 0.52f;
    p.petals = 1.0f; p.embers = 0.5f; p.ash = 0.4f; p.mist = 0.3f; p.wind = {60, 0};
    return p;
}

const Palette& paletteByName(const std::string& name) {
    static const std::map<std::string, Palette> table = {
        {"moon", makeMoon()}, {"ember", makeEmber()}, {"grove", makeGrove()}, {"ravine", makeRavine()}, {"gate", makeGate()}};
    auto it = table.find(name);
    if (it == table.end()) { logWarn("unknown palette '%s'", name.c_str()); return table.at("moon"); }
    return it->second;
}

BgStyle styleByName(const std::string& n) {
    if (n == "village") return BgStyle::Village;
    if (n == "grove") return BgStyle::Grove;
    if (n == "ravine") return BgStyle::Ravine;
    if (n == "gate") return BgStyle::Gate;
    return BgStyle::Cliff;
}

static SkyParams lerp(const SkyParams& a, const SkyParams& b, float t) {
    SkyParams o = a;
    o.top = lerp(a.top, b.top, t); o.mid = lerp(a.mid, b.mid, t); o.horizon = lerp(a.horizon, b.horizon, t);
    o.horizonY = lerp(a.horizonY, b.horizonY, t); o.moonPos = lerp(a.moonPos, b.moonPos, t);
    o.moonRadius = lerp(a.moonRadius, b.moonRadius, t); o.moon = lerp(a.moon, b.moon, t);
    o.moonVeil = lerp(a.moonVeil, b.moonVeil, t); o.stars = lerp(a.stars, b.stars, t); o.cloud = lerp(a.cloud, b.cloud, t);
    return o;
}
static PostParams lerp(const PostParams& a, const PostParams& b, float t) {
    PostParams o = a;
    o.lift = lerp(a.lift, b.lift, t); o.gamma = lerp(a.gamma, b.gamma, t); o.gain = lerp(a.gain, b.gain, t);
    o.saturation = lerp(a.saturation, b.saturation, t); o.bloom = lerp(a.bloom, b.bloom, t);
    o.vignette = lerp(a.vignette, b.vignette, t);
    return o;
}

Palette lerp(const Palette& a, const Palette& b, float t) {
    Palette o;
    o.sky = lerp(a.sky, b.sky, t);
    o.fog = lerp(a.fog, b.fog, t); o.far = lerp(a.far, b.far, t); o.mid = lerp(a.mid, b.mid, t); o.nearC = lerp(a.nearC, b.nearC, t);
    o.ground = lerp(a.ground, b.ground, t); o.grass = lerp(a.grass, b.grass, t); o.rim = lerp(a.rim, b.rim, t);
    o.glow = lerp(a.glow, b.glow, t); o.glowAmt = lerp(a.glowAmt, b.glowAmt, t);
    o.post = lerp(a.post, b.post, t);
    o.embers = lerp(a.embers, b.embers, t); o.ash = lerp(a.ash, b.ash, t); o.fireflies = lerp(a.fireflies, b.fireflies, t);
    o.leaves = lerp(a.leaves, b.leaves, t); o.petals = lerp(a.petals, b.petals, t); o.mist = lerp(a.mist, b.mist, t);
    o.wind = lerp(a.wind, b.wind, t);
    return o;
}

// ------------------------------------------------------------------ level

int Level::roomAt(float worldX) const {
    int col = (int)std::floor(worldX / TILE);
    for (size_t i = 0; i < rooms.size(); i++)
        if (col >= rooms[i].x0 && col < rooms[i].x0 + rooms[i].w) return (int)i;
    if (rooms.empty()) return 0;
    return col < 0 ? 0 : (int)rooms.size() - 1;
}

const Spawn* Level::findSpawn(const std::string& kind) const {
    for (auto& s : spawns) if (s.kind == kind) return &s;
    return nullptr;
}
const Spawn* Level::findById(const std::string& id) const {
    for (auto& s : spawns) if (s.id == id) return &s;
    return nullptr;
}

static std::map<std::string, std::string> parseParams(const std::vector<std::string>& tok, size_t from) {
    std::map<std::string, std::string> m;
    for (size_t i = from; i < tok.size(); i++) {
        auto eq = tok[i].find('=');
        if (eq == std::string::npos) m[tok[i]] = "1";
        else m[tok[i].substr(0, eq)] = tok[i].substr(eq + 1);
    }
    return m;
}

static const char* spawnKindForChar(char c) {
    switch (c) {
    case 'P': return "player";
    case 'S': return "shrine";
    case 's': return "swordsman";
    case 'y': return "spearman";
    case 'r': return "archer";
    case 'T': return "boss";
    case 'G': return "anchor";
    case '$': return "mon";
    case '*': return "shuriken";
    case 'o': return "bead";
    case 'm': return "material";
    default: return nullptr;
    }
}

bool parseLevel(const std::string& text, Level& L, std::string& err) {
    L = Level();
    std::vector<std::string> lines = split(text, '\n');
    struct PendingRoom { Room room; std::vector<std::string> rows; std::vector<std::vector<std::string>> extra; int line; };
    std::vector<PendingRoom> rooms;
    PendingRoom* cur = nullptr;
    bool inMap = false;
    for (size_t li = 0; li < lines.size(); li++) {
        std::string raw = lines[li];
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
        if (inMap) {
            if (trim(raw) == "endmap") { inMap = false; continue; }
            cur->rows.push_back(raw);
            continue;
        }
        std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;
        auto tok = tokenize(line);
        const std::string& cmd = tok[0];
        auto need = [&](size_t n) {
            if (tok.size() < n) { err = strfmt("line %zu: '%s' needs %zu arguments", li + 1, cmd.c_str(), n - 1); return false; }
            return true;
        };
        if (cmd == "level") { if (!need(2)) return false; L.id = tok[1]; }
        else if (cmd == "title") { if (!need(2)) return false; L.title = tok[1]; }
        else if (cmd == "kanji") { if (!need(2)) return false; L.kanji = tok[1]; }
        else if (cmd == "height") { if (!need(2)) return false; L.height = std::stoi(tok[1]); }
        else if (cmd == "room") {
            if (!need(3)) return false;
            rooms.push_back({});
            cur = &rooms.back();
            cur->line = (int)li + 1;
            Room& r = cur->room;
            r.id = tok[1]; r.name = tok[2];
            auto p = parseParams(tok, 3);
            if (p.count("palette")) r.palette = p["palette"];
            if (p.count("style")) r.style = styleByName(p["style"]);
            if (p.count("zoom")) r.zoom = std::stof(p["zoom"]);
            if (p.count("music")) r.music = p["music"];
            if (p.count("wind")) r.wind = std::stof(p["wind"]);
            if (p.count("fire")) r.fire = std::stof(p["fire"]);
            if (p.count("crickets")) r.crickets = std::stof(p["crickets"]);
            if (p.count("boss")) r.bossArena = true;
            if (p.count("stealth")) r.stealthZone = true;
            r.seed = (int)hash32((uint32_t)std::hash<std::string>{}(r.id));
        }
        else if (cmd == "map") { if (!cur) { err = strfmt("line %zu: map outside room", li + 1); return false; } inMap = true; }
        else if (cmd == "endroom") { cur = nullptr; }
        else if (cmd == "spawn" || cmd == "prop" || cmd == "trigger") {
            if (!cur) { err = strfmt("line %zu: '%s' outside room", li + 1, cmd.c_str()); return false; }
            cur->extra.push_back(tok);
        }
        else { err = strfmt("line %zu: unknown command '%s'", li + 1, cmd.c_str()); return false; }
    }
    if (inMap) { err = "unterminated map block"; return false; }
    if (L.height <= 0) { err = "missing 'height'"; return false; }
    if (rooms.empty()) { err = "level has no rooms"; return false; }

    int total = 0;
    for (auto& pr : rooms) {
        if ((int)pr.rows.size() != L.height) {
            err = strfmt("room '%s' (line %d): map has %zu rows, expected %d", pr.room.id.c_str(), pr.line, pr.rows.size(), L.height);
            return false;
        }
        size_t w = 0;
        for (auto& r : pr.rows) w = std::max(w, r.size());
        pr.room.x0 = total; pr.room.w = (int)w;
        total += (int)w;
    }
    L.width = total;
    L.tiles.assign(L.width * L.height, Tile::Empty);
    L.grass.assign(L.width * L.height, 0);
    std::map<std::string, int> counters;
    auto makeId = [&](const std::string& room, const std::string& kind) {
        int n = counters[room + kind]++;
        return strfmt("%s.%s.%s.%d", L.id.c_str(), room.c_str(), kind.c_str(), n);
    };
    for (size_t ri = 0; ri < rooms.size(); ri++) {
        auto& pr = rooms[ri];
        Room& room = pr.room;
        for (int y = 0; y < L.height; y++) {
            const std::string& row = pr.rows[y];
            for (int x = 0; x < room.w; x++) {
                char c = x < (int)row.size() ? row[x] : '.';
                int gx = room.x0 + x;
                Tile t = Tile::Empty;
                switch (c) {
                case '#': t = Tile::Solid; break;
                case '=': t = Tile::Plank; break;
                case '-': t = Tile::OneWay; break;
                case '^': t = Tile::Spikes; break;
                case '%': t = Tile::FalseWall; break;
                case 'B': t = Tile::Breakable; break;
                case 'W': t = Tile::Wall; break;
                case 'h': L.grass[y * L.width + gx] = 1; break;
                case 'L': case 'l': case 'f': {
                    Prop p; p.kind = c == 'L' ? "lantern" : (c == 'l' ? "hanging_lantern" : "fire");
                    p.pos = {gx * TILE, (y + 1) * TILE}; p.room = (int)ri; p.seed = gx * 131 + y;
                    L.props.push_back(p);
                    break;
                }
                default:
                    if (const char* kind = spawnKindForChar(c)) {
                        Spawn s; s.kind = kind; s.room = (int)ri;
                        s.pos = {(gx + 0.5f) * TILE, (y + 1) * TILE};
                        s.id = makeId(room.id, kind);
                        if (s.kind == "anchor") s.pos.y = (y + 0.5f) * TILE;
                        L.spawns.push_back(s);
                    }
                    break;
                }
                L.tiles[y * L.width + gx] = t;
            }
        }
        for (auto& tok : pr.extra) {
            const std::string& cmd = tok[0];
            if (cmd == "spawn") {
                if (tok.size() < 4) { err = strfmt("room %s: spawn needs kind x y", room.id.c_str()); return false; }
                Spawn s; s.kind = tok[1]; s.room = (int)ri;
                float tx = std::stof(tok[2]), ty = std::stof(tok[3]);
                s.pos = {(room.x0 + tx + 0.5f) * TILE, (ty + 1) * TILE};
                s.params = parseParams(tok, 4);
                s.id = s.params.count("id") ? s.params["id"] : makeId(room.id, s.kind);
                L.spawns.push_back(s);
            } else if (cmd == "prop") {
                if (tok.size() < 4) { err = strfmt("room %s: prop needs kind x y", room.id.c_str()); return false; }
                Prop p; p.kind = tok[1]; p.room = (int)ri;
                float tx = std::stof(tok[2]), ty = std::stof(tok[3]);
                p.pos = {(room.x0 + tx) * TILE, (ty + 1) * TILE};
                p.params = parseParams(tok, 4);
                if (p.params.count("w")) p.w = std::stof(p.params["w"]);
                if (p.params.count("h")) p.h = std::stof(p.params["h"]);
                p.seed = (int)(tx * 97 + ty * 13) + (int)ri * 1000;
                L.props.push_back(p);
            } else if (cmd == "trigger") {
                if (tok.size() < 6) { err = strfmt("room %s: trigger needs x y w h action", room.id.c_str()); return false; }
                Trigger t; t.room = (int)ri;
                t.area = {(room.x0 + std::stof(tok[1])) * TILE, std::stof(tok[2]) * TILE, std::stof(tok[3]) * TILE, std::stof(tok[4]) * TILE};
                std::string action;
                for (size_t i = 5; i < tok.size(); i++) {
                    if (tok[i] == "repeat") { t.once = false; continue; }
                    if (!action.empty()) action += ' ';
                    action += tok[i];
                }
                t.action = action;
                t.id = makeId(room.id, "trigger");
                L.triggers.push_back(t);
            }
        }
        L.rooms.push_back(room);
    }
    int players = 0;
    for (auto& s : L.spawns) if (s.kind == "player") players++;
    if (players != 1) { err = strfmt("level must contain exactly one player start 'P' (found %d)", players); return false; }
    return true;
}

bool loadLevel(const std::string& path, Level& out, std::string& err) {
    auto txt = readFile(path);
    if (!txt) { err = "cannot read " + path; return false; }
    return parseLevel(*txt, out, err);
}

} // namespace sv
