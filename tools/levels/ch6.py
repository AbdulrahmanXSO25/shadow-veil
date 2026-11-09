"""Chapter VI — The Heart of the Veil.

Inside the Veil: a loom the size of a mountain, threads running up out of sight, and silent
sentinels woven to guard it. Odaka has begun cutting. Rei climbs the loom, outruns the
unravelling, and reaches the Heart, where Sayo holds the weave — and where Odaka waits.
Run:  python3 tools/levels/ch6.py
"""
import os
import sys
sys.path.insert(0, os.path.dirname(__file__))
from levelkit import Room, write_level

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
H = 40


def threshold():
    r = Room("threshold", "The Threshold", 56, H, palette="loom", style="loom", zoom=1.0, music="title", wind=0.3)
    r.ground(0, 55, 32)
    r.at(4, 31, "P")
    r.at(12, 31, "S", "camp")
    r.spawn("examine", 20, 31, text="Threads as thick as rope, running up out of sight. Each one hums a different note when the wind touches it.")
    r.fill(36, 29, 55, 31)
    r.spawn("sentinel", 46, 28, face="left", patrol=3)
    r.prop("thread", 8, 31, h=24)
    r.prop("thread", 26, 31, h=26)
    r.prop("loom", 30, 31, w=5, h=14)
    r.prop("thread", 50, 28, h=22)
    r.trigger(2, 18, 4, 14, "cutscene intro")
    r.trigger(32, 16, 2, 14, "hint sentinel")
    r.mark("exit", 55, 28)
    return r


def loomhall():
    """The great loom: climb its beams to the upper warp, where the threads run out to the Heart."""
    r = Room("loomhall", "The Great Loom", 60, H, palette="loom", style="loom", zoom=0.86, wind=0.4)
    r.ground(0, 59, 38)
    r.fill(0, 29, 6, 37)                        # from the threshold
    r.fill(7, 35, 59, 37)
    # the loom's beams: a climb in three tiers, with rings between
    for x0, x1, y in [(10, 18, 32), (22, 30, 29), (14, 20, 26), (6, 12, 23), (14, 22, 20),
                      (26, 32, 17), (38, 44, 17), (48, 56, 14), (40, 46, 11), (24, 34, 8)]:
        r.fill(x0, y, x1, y, "=")
    r.at(35, 14, "G")
    r.fill(36, 8, 59, 8)                        # the upper warp
    r.fill(57, 9, 59, 34, "W")
    r.clear(57, 3, 59, 7)
    r.fill(2, 17, 8, 17)                        # a hidden bay off the third tier
    r.fill(2, 13, 2, 16)
    r.fill(2, 12, 8, 12)
    r.fill(8, 13, 8, 16, "%")
    r.spawn("chest", 4, 16, "needle", item="weavers_needle")
    r.spawn("scroll", 16, 25, "scroll", lore="scroll_loom")
    r.spawn("scroll", 52, 13, "scroll", lore="scroll_sentinels")
    r.spawn("sentinel", 30, 34, face="left", patrol=4)
    r.spawn("sentinel", 46, 7, face="left", patrol=3)
    r.spawn("hollow", 28, 16, face="left")
    r.at(42, 34, "o", "floor_bead")
    r.prop("loom", 20, 34, w=6, h=20)
    r.prop("loom", 44, 34, w=6, h=24)
    r.prop("thread", 54, 34, h=26)
    r.mark("exit", 59, 7)
    return r


def unravel():
    """The weave is coming apart behind Rei. A long run with no time to stop."""
    r = Room("unravel", "The Unravelling", 120, H, palette="unravel", style="loom", zoom=0.9, wind=1.2)
    r.ground(0, 119, 12)
    r.clear(0, 0, 119, 7)
    r.fill(0, 8, 8, 11)                         # the upper warp continues
    r.clear(9, 8, 119, 11)
    r.ground(0, 119, 30)
    r.clear(0, 12, 119, 29)
    r.fill(0, 8, 8, 29)
    # the run: a series of drops, hurdles and gaps on the falling warp
    r.fill(9, 14, 20, 14)
    r.fill(21, 18, 32, 18)
    r.fill(33, 22, 44, 29)
    r.fill(48, 24, 52, 29)                      # hurdle
    r.fill(56, 22, 66, 29)
    r.clear(67, 22, 71, 29)                     # a gap to clear
    r.fill(67, 28, 71, 29)
    r.fill(72, 22, 84, 29)
    r.fill(88, 19, 92, 19, "=")
    r.fill(95, 22, 119, 29)
    r.fill(100, 17, 104, 21)
    r.spawn("sentinel", 78, 21, face="left")
    r.prop("thread", 40, 22, h=20)
    r.prop("thread", 62, 22, h=20)
    r.prop("thread", 110, 22, h=20)
    r.trigger(10, 4, 2, 10, "cutscene unravel_start")
    r.trigger(112, 8, 3, 14, "chase_stop")
    r.mark("exit", 119, 21)
    return r


def quiet():
    r = Room("quiet", "The Quiet Room", 54, H, palette="heart", style="loom", zoom=1.1, music="camp", wind=0.1)
    r.ground(0, 53, 22)
    r.at(14, 21, "S", "camp")
    r.fill(30, 19, 34, 19, "=")
    r.fill(38, 16, 42, 16, "=")
    r.fill(44, 13, 50, 13)
    r.spawn("scroll", 22, 21, "scroll", lore="scroll_odaka")
    r.spawn("scroll", 47, 12, "scroll", lore="scroll_restitch")
    r.spawn("examine", 8, 21, text="A child's sandal, very small, hung on a thread at eye height. Someone has kept it for a long time.")
    r.prop("thread", 26, 21, h=14)
    r.trigger(16, 10, 2, 11, "cutscene quiet")
    r.mark("exit", 53, 21)
    return r


def heart():
    r = Room("heart", "The Heart of the Veil", 66, H, palette="heart", style="loom", zoom=0.9, wind=0.2, boss=1)
    r.ground(0, 65, 22)
    r.fill(62, 0, 65, 21)
    r.fill(14, 16, 19, 16, "=")
    r.fill(44, 16, 49, 16, "=")
    r.spawn("gate", 4, 21, h=6, id="boss_door", open=1)
    r.spawn("boss", 40, 21, who="odaka", face="left")
    r.spawn("npc", 56, 21, who="sayo", id="sayo", face="left", hidden=1)
    r.prop("loom", 52, 21, w=8, h=18)
    r.prop("thread", 10, 21, h=20)
    r.prop("thread", 30, 21, h=22)
    r.trigger(10, 8, 2, 14, "cutscene boss_intro")
    r.mark("boss", 40, 21)
    return r


def main():
    rooms = [threshold(), loomhall(), unravel(), quiet(), heart()]
    header = """SHADOW VEIL — Chapter VI: The Heart of the Veil
Generated by tools/levels/ch6.py (levelkit). Edit the script, not this file."""
    ok = write_level(os.path.join(ROOT, "data/levels/ch6_heart_of_veil.lvl"), "ch6", "The Heart of the Veil", "幕心", rooms, header,
                     preview_path=os.path.join(os.environ.get("SV_PREVIEW_DIR", "/tmp"), "ch6_preview.png"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
