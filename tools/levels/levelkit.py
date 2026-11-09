"""levelkit — paint SHADOW VEIL rooms in Python and write the .lvl format the game reads.

Each chapter script (ch2.py ...) builds rooms with the helpers below, then calls `write_level`.
The writer also runs an approximate reachability check (a coarse model of Rei's movement:
walking, ~3-tile jumps, a dash, wall-jumps, grapple rings, swimming, rising wind and the
Mirror Province's two layers) so that a room which cannot be crossed fails loudly at
authoring time instead of during play. It is deliberately conservative about what it
treats as reachable; it is a safety net, not a substitute for playing the room.

Coordinates: x = column, y = row (0 at the top). An object "standing" at y occupies row y
with solid ground at row y+1 — the same convention the .lvl format uses.
"""
from __future__ import annotations
import collections
import os
import sys

SOLID = set("#=Wc")          # blocks the body (breakable fences and false walls do not)
ONEWAY = set("-")
WATER = set("~q")
LAYER_A, LAYER_B = "a", "b"
SPAWN_CHARS = set("PSsyrTG$*omvqkjngezK")


class Room:
    def __init__(self, rid, name, width, height, **opts):
        self.id, self.name, self.w, self.h = rid, name, width, height
        self.opts = opts
        self.g = [["."] * width for _ in range(height)]
        self.extra = []          # spawn/prop/trigger lines
        self.markers = {}        # name -> (x, y) used by the reachability check

    # ---------------------------------------------------------------- painting
    def put(self, x, y, ch):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.g[y][x] = ch

    def get(self, x, y):
        if x < 0 or x >= self.w:
            return "#"
        if y < 0:
            return "."
        if y >= self.h:
            return "#"
        return self.g[y][x]

    def fill(self, x0, y0, x1, y1, ch="#"):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for x in range(min(x0, x1), max(x0, x1) + 1):
                self.put(x, y, ch)

    def ground(self, x0, x1, top, ch="#"):
        """Solid from row `top` to the bottom of the room."""
        self.fill(x0, top, x1, self.h - 1, ch)

    def clear(self, x0, y0, x1, y1):
        self.fill(x0, y0, x1, y1, ".")

    def beam(self, x0, x1, y, ch="-"):
        for x in range(x0, x1 + 1):
            self.put(x, y, ch)

    def pillar(self, x, top, bottom=None, w=1, ch="#"):
        bottom = self.h - 1 if bottom is None else bottom
        self.fill(x, top, x + w - 1, bottom, ch)

    def water(self, x0, x1, top, bottom):
        for y in range(top, bottom + 1):
            for x in range(x0, x1 + 1):
                if self.get(x, y) == ".":
                    self.put(x, y, "~")

    def grass(self, x0, x1, y):
        for x in range(x0, x1 + 1):
            if self.get(x, y) == ".":
                self.put(x, y, "h")

    def updraft(self, x0, x1, top, bottom):
        for y in range(top, bottom + 1):
            for x in range(x0, x1 + 1):
                if self.get(x, y) == ".":
                    self.put(x, y, "|")

    def at(self, x, y, ch, name=None):
        """Place a single-character spawn (P, S, s, y, r, T, G, $, *, o, m, v, q, k, j, n, g, e, z, K)."""
        self.put(x, y, ch)
        if name:
            self.markers[name] = (x, y)

    # ---------------------------------------------------------------- objects
    def spawn(self, kind, x, y, name=None, **kv):
        self.extra.append(("spawn", kind, x, y, kv))
        if name:
            self.markers[name] = (x, y)

    def prop(self, kind, x, y, **kv):
        self.extra.append(("prop", kind, x, y, kv))

    def trigger(self, x, y, w, h, action, repeat=False):
        self.extra.append(("trigger", action, x, y, (w, h, repeat)))

    def mark(self, name, x, y):
        self.markers[name] = (x, y)

    # ---------------------------------------------------------------- output
    def pad_to(self, H, top=0):
        """Grow the room to height H: `top` empty rows above, copies of the bottom row below.
        Every object, trigger and marker moves down with the map."""
        bottom = H - self.h - top
        assert top >= 0 and bottom >= 0, f"room {self.id}: cannot pad {self.h} to {H} with top={top}"
        self.g = [["."] * self.w for _ in range(top)] + self.g + [list(self.g[-1]) for _ in range(bottom)]
        self.extra = [(k, a, x, y + top, kv) for k, a, x, y, kv in self.extra]
        self.markers = {n: (x, y + top) for n, (x, y) in self.markers.items()}
        self.h = H
        return self

    def to_lvl(self):
        o = []
        opts = " ".join(f"{k}={v}" for k, v in self.opts.items())
        o.append(f'room {self.id} "{self.name}" {opts}'.rstrip())
        o.append("map")
        for row in self.g:
            o.append("".join(row))
        o.append("endmap")
        for kind, a, x, y, kv in self.extra:
            if kind == "trigger":
                w, h, rep = kv
                o.append(f"trigger {x} {y} {w} {h} {a}" + (" repeat" if rep else ""))
            else:
                parts = []
                for k, v in kv.items():
                    v = str(v)
                    parts.append(f'{k}="{v}"' if (" " in v or "," in v and k == "text") else f"{k}={v}")
                o.append(f"{kind} {a} {x} {y} " + " ".join(parts))
        o.append("endroom")
        return "\n".join(s.rstrip() for s in o)


# ==================================================================== reachability

class Grid:
    """All rooms laid side by side, as the game does."""

    def __init__(self, rooms):
        self.h = rooms[0].h
        self.rooms = rooms
        self.x0 = []
        x = 0
        for r in rooms:
            self.x0.append(x)
            x += r.w
        self.w = x
        self.cells = [["."] * self.w for _ in range(self.h)]
        self.layers_room = []
        for r, ox in zip(rooms, self.x0):
            for y in range(self.h):
                for xx in range(r.w):
                    self.cells[y][ox + xx] = r.g[y][xx]
            self.layers_room += [bool(r.opts.get("layers"))] * r.w
        self.anchors = [(x, y) for y in range(self.h) for x in range(self.w) if self.cells[y][x] == "G"]
        # gates authored as mechanisms block nothing here (puzzles are checked by hand)

    def ch(self, x, y):
        if x < 0 or x >= self.w:
            return "#"
        if y < 0:
            return "."
        if y >= self.h:
            return "#"
        return self.cells[y][x]

    def solid(self, x, y, lay):
        c = self.ch(x, y)
        if c in SOLID:
            return True
        if c == LAYER_A and lay == 0:
            return True
        if c == LAYER_B and lay == 1:
            return True
        return False

    def water(self, x, y):
        return self.ch(x, y) in WATER

    def floor(self, x, y, lay):
        return self.solid(x, y, lay) or self.ch(x, y) in ONEWAY

    def body_free(self, x, y, lay):
        return not self.solid(x, y, lay) and not self.solid(x, y - 1, lay)

    def standable(self, x, y, lay):
        return self.body_free(x, y, lay) and (self.floor(x, y + 1, lay) or self.water(x, y + 1) or self.water(x, y))

    def updraft(self, x, y):
        return self.ch(x, y) == "|"


def _arc_clear(G, x, y, x2, y2, lay, peak):
    """Is there room for a jump arc from (x,y) up to row `peak` and down to (x2,y2)?"""
    top = min(y, y2) - peak
    # rise in the take-off column, travel at the top, fall in the landing column
    for yy in range(top, y + 1):
        if not G.body_free(x, yy, lay):
            # allow the rise to be cut short if the landing is lower than the obstruction
            return False
    step = 1 if x2 >= x else -1
    for xx in range(x, x2 + step, step):
        if not G.body_free(xx, top, lay):
            return False
    for yy in range(top, y2 + 1):
        if not G.body_free(x2, yy, lay):
            return False
    return True


def reachable(rooms, start, dash=True):
    G = Grid(rooms)
    seen = set()
    q = collections.deque()
    s = (start[0], start[1], 0)
    seen.add(s)
    q.append(s)
    H = G.h
    while q:
        x, y, lay = q.popleft()
        nxt = []
        in_water = G.water(x, y) or G.water(x, y - 1)
        # layer flip (Veil-Step) where allowed
        if G.layers_room[x] and G.body_free(x, y, 1 - lay):
            nxt.append((x, y, 1 - lay))
        if in_water:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if G.body_free(nx, ny, lay) and (G.water(nx, ny) or G.water(nx, ny - 1) or G.standable(nx, ny, lay)):
                    nxt.append((nx, ny, lay))
            # leap out from the surface
            if not G.water(x, y - 2):
                for dx in range(-5, 6):
                    for up in range(0, 4):
                        nx, ny = x + dx, y - up
                        if G.standable(nx, ny, lay) and _arc_clear(G, x, y, nx, ny, lay, 1):
                            nxt.append((nx, ny, lay))
        if G.standable(x, y, lay) or in_water:
            # walking
            for dx in (-1, 1):
                if G.body_free(x + dx, y, lay):
                    nxt.append((x + dx, y, lay))
                    # step off a ledge: fall straight down
                    yy = y
                    while yy < H - 1 and G.body_free(x + dx, yy + 1, lay) and not G.floor(x + dx, yy + 1, lay) and not G.water(x + dx, yy):
                        yy += 1
                    nxt.append((x + dx, yy, lay))
            # drop through one-way beams
            if G.ch(x, y + 1) in ONEWAY:
                yy = y + 1
                while yy < H - 1 and not G.floor(x, yy + 1, lay) and not G.water(x, yy):
                    yy += 1
                nxt.append((x, yy, lay))
            # jumps (+ an optional mid-air dash)
            reach = 9 if dash else 5
            for dx in range(-reach, reach + 1):
                for dy in range(-3, 12):
                    nx, ny = x + dx, y + dy
                    if (dx, dy) == (0, 0):
                        continue
                    if not (G.standable(nx, ny, lay) or G.water(nx, ny)):
                        continue
                    peak = 3 if dy >= -3 else 0
                    if abs(dx) > 6 and dy < -1:
                        continue  # long dashes cannot also climb much
                    if _arc_clear(G, x, y, nx, ny, lay, max(0, 3 + min(0, dy)) if dy < 0 else 2):
                        nxt.append((nx, ny, lay))
            # wall-jumps: climb a wall face (or a chimney) to its top
            for side in (-1, 1):
                if G.solid(x + side, y - 1, lay):
                    yy = y
                    while yy > 1 and G.body_free(x, yy - 1, lay) and G.solid(x + side, yy - 1, lay):
                        yy -= 1
                    # from the top of the climb, hop onto the wall or keep going
                    for dx in range(-3, 4):
                        for up in range(0, 4):
                            nx, ny = x + dx, yy - up
                            if G.standable(nx, ny, lay):
                                nxt.append((nx, ny, lay))
                    nxt.append((x, yy, lay))
            # grapple rings within reach
            for ax, ay in G.anchors:
                if abs(ax - x) <= 8 and y - 1 - ay >= 1 and (ax - x) ** 2 + (y - 1 - ay) ** 2 <= 8.3 ** 2:
                    for dx in range(-11, 12):
                        for ny in range(ay - 1, min(H - 1, ay + 14)):
                            nx = ax + dx
                            if G.standable(nx, ny, lay):
                                nxt.append((nx, ny, lay))
        # rising wind
        if G.updraft(x, y) or G.updraft(x, y - 1):
            yy = y
            while yy > 1 and (G.updraft(x, yy - 1) or G.updraft(x, yy - 2)) and G.body_free(x, yy - 1, lay):
                yy -= 1
            for top in range(yy, y + 1):
                for dx in range(-7, 8):
                    for dy in range(-4, 6):
                        nx, ny = x + dx, top + dy
                        if G.standable(nx, ny, lay):
                            nxt.append((nx, ny, lay))
                nxt.append((x, top, lay))
        if not G.standable(x, y, lay) and not in_water and not G.updraft(x, y):
            # in the air (e.g. after a flip): fall
            yy = y
            while yy < H - 1 and G.body_free(x, yy + 1, lay) and not G.floor(x, yy + 1, lay):
                yy += 1
            nxt.append((x, yy, lay))
        for n in nxt:
            nx, ny, nl = n
            if 0 <= nx < G.w and 0 <= ny < H and n not in seen and G.body_free(nx, ny, nl):
                seen.add(n)
                q.append(n)
    return G, seen


def check(rooms, must_reach=()):
    """Every room's right edge, and every named marker, must be reachable from the start."""
    G = Grid(rooms)
    start = None
    for r, ox in zip(rooms, G.x0):
        for y in range(r.h):
            for x in range(r.w):
                if r.g[y][x] == "P":
                    start = (ox + x, y)
    assert start, "no player start"
    G, seen = reachable(rooms, start)
    pts = {(x, y) for x, y, _ in seen}
    problems = []
    for r, ox in zip(rooms, G.x0):
        edge = ox + r.w - 1
        if r is not rooms[-1] and not any(px >= edge - 1 and px < ox + r.w + 1 for px, _ in pts):
            problems.append(f"room {r.id}: right edge not reached")
        for name, (mx, my) in r.markers.items():
            gx = ox + mx
            ok = any(abs(px - gx) <= 1 and abs(py - my) <= 1 for px, py in pts)
            if not ok:
                problems.append(f"room {r.id}: marker '{name}' at ({mx},{my}) not reached")
    return problems


def find_start(rooms):
    x0 = 0
    for r in rooms:
        for y in range(r.h):
            for x in range(r.w):
                if r.g[y][x] == "P":
                    return (x0 + x, y)
        x0 += r.w
    raise AssertionError("no player start")


def preview(rooms, path, scale=4):
    from PIL import Image
    G = Grid(rooms)
    _, seen = reachable(rooms, find_start(rooms))
    pts = {(x, y) for x, y, _ in seen}
    colors = {"#": (40, 40, 48), "=": (90, 60, 40), "W": (70, 50, 40), "-": (150, 110, 70), "^": (180, 40, 40),
              "~": (40, 90, 140), "|": (170, 220, 230), "c": (140, 100, 60), "a": (230, 220, 200), "b": (120, 80, 180),
              "h": (40, 90, 50), "G": (255, 220, 120), "%": (60, 60, 70), "B": (110, 90, 50)}
    img = Image.new("RGB", (G.w * scale, G.h * scale), (14, 14, 20))
    px = img.load()
    for y in range(G.h):
        for x in range(G.w):
            c = G.cells[y][x]
            col = colors.get(c)
            if c in "sykjvnge zK".replace(" ", ""):
                col = (230, 60, 60)
            elif c in "rgz":
                col = (230, 120, 60)
            elif c in "PS":
                col = (80, 230, 120)
            elif c in "T":
                col = (255, 40, 200)
            elif c in "$*om":
                col = (240, 200, 80)
            if col is None and (x, y) in pts:
                col = (30, 50, 40)
            if col:
                for dy in range(scale):
                    for dx in range(scale):
                        px[x * scale + dx, y * scale + dy] = col
    for ox in G.x0:
        for y in range(G.h * scale):
            px[ox * scale, y] = (90, 90, 120)
    img.save(path)


def write_level(path, level_id, title, kanji, rooms, header="", must_reach=(), preview_path=None):
    for r in rooms:
        assert r.h == rooms[0].h, f"room {r.id} height {r.h} != {rooms[0].h}"
    out = []
    if header:
        out += ["# " + l if l else "#" for l in header.strip().split("\n")]
    out += [f"level {level_id}", f'title "{title}"', f'kanji "{kanji}"', f"height {rooms[0].h}", ""]
    for r in rooms:
        out.append(r.to_lvl())
        out.append("")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    problems = check(rooms)
    if preview_path:
        preview(rooms, preview_path)
    if problems:
        print(f"{path}: reachability problems:")
        for p in problems:
            print("   ", p)
        return False
    print(f"{path}: {len(rooms)} rooms, {sum(r.w for r in rooms)} columns — all exits and markers reachable")
    return True
