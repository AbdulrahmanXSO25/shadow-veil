"""Chapter V — The Mirror Province.

Where the Veil runs thinnest the world doubles: a Bright side and a Hollow side, each with stone
the other lacks. A thread from the Weaver lets Rei step between them with a dash. Kagami, who
guards the crossing for the exiles, tests her; and for the first time, Sayo speaks.
Tiles: a = stone only on the Bright side, b = stone only on the Hollow side.
Run:  python3 tools/levels/ch5.py
"""
import os
import sys
sys.path.insert(0, os.path.dirname(__file__))
from levelkit import Room, write_level

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
H = 40
LAY = dict(layers=1, palette2="hollow")


def shore():
    r = Room("shore", "The Glass Shore", 66, H, palette="mirror", style="mirror", zoom=1.0, music="title", wind=0.3, **LAY)
    r.ground(0, 65, 32)
    r.at(4, 31, "P")
    r.at(12, 31, "S", "camp")
    r.spawn("examine", 18, 31, text="A lake so still it holds a second sky. In the reflection, the shore has houses on it. On this side, it does not.")
    r.fill(28, 24, 29, 31, "a")                 # a wall that exists only on the Bright side
    r.fill(40, 29, 46, 29, "b")                 # a jetty that exists only on the Hollow side
    r.clear(38, 32, 48, 34)
    r.water(38, 48, 32, 34)
    r.fill(54, 22, 55, 31, "a")
    r.fill(56, 28, 65, 31)
    r.spawn("hollow", 60, 27, face="left", patrol=2)
    r.at(43, 28, "o", "jetty_bead")
    r.prop("thread", 22, 31, h=14)
    r.prop("thread", 50, 31, h=18)
    r.trigger(2, 18, 4, 14, "cutscene intro")
    r.trigger(24, 18, 2, 14, "hint layers")
    r.mark("exit", 64, 27)
    return r


def orchard():
    """Silver trees over a dry lakebed: every crossing is half on one side, half on the other."""
    r = Room("orchard", "The Silver Orchard", 88, H, palette="mirror", style="mirror", zoom=0.92, wind=0.3, **LAY)
    r.ground(0, 87, 36)
    r.fill(0, 28, 10, 35)
    r.fill(11, 28, 22, 28, "b")                 # Hollow bridge
    r.fill(23, 26, 30, 35)
    r.fill(31, 25, 40, 25, "a")                 # Bright bridge
    r.fill(41, 26, 48, 35)
    r.fill(49, 23, 52, 23, "b")
    r.fill(53, 20, 57, 20, "a")
    r.fill(58, 17, 62, 17, "b")
    r.fill(63, 17, 70, 35)                      # the orchard keeper's terrace
    r.fill(71, 28, 87, 35)
    r.fill(76, 22, 77, 27, "b")                 # a hollow-side fence across the path
    r.spawn("scroll", 66, 16, "scroll", lore="scroll_mirror")
    r.spawn("hollow", 26, 25, face="left", patrol=2)
    r.spawn("snare", 45, 25, face="left")
    r.spawn("hollow", 82, 27, face="left", patrol=3)
    r.spawn("snare", 68, 16, face="left")
    r.at(15, 35, "m")
    r.at(55, 19, "o", "high_bead")
    r.trigger(42, 14, 2, 12, "hint snare")
    r.mark("exit", 86, 27)
    return r


def village():
    """A village that is half here and half there. Exiles on the Hollow side, empty houses on this one."""
    r = Room("village", "Kagami's Crossing", 72, H, palette="mirror", style="mirror", zoom=1.05, music="camp", wind=0.2, **LAY)
    r.ground(0, 71, 28)
    r.at(20, 27, "S", "camp")
    r.spawn("npc", 30, 27, who="refugee", id="m_exile", talk="mirror_exile", face="left")
    r.spawn("npc", 34, 27, who="child", id="m_child", face="left")
    r.spawn("npc", 46, 27, who="ferryman", id="ferryman", talk="ferryman_talk", face="left")
    r.prop("house", 38, 27, w=6, h=4)
    r.prop("house", 54, 27, w=5, h=3)
    r.fill(57, 25, 59, 27)                      # a stack of crates
    r.fill(60, 22, 66, 22, "b")                 # a hollow-side loft: the scroll
    r.spawn("scroll", 63, 21, "scroll", lore="scroll_sayo")
    r.prop("thread", 12, 27, h=16)
    r.trigger(24, 14, 2, 14, "cutscene crossing")
    r.mark("exit", 71, 27)
    return r


def stair():
    """The reflected stair: a tall climb where the steps alternate sides."""
    r = Room("stair", "The Reflected Stair", 50, H, palette="hollow", style="mirror", zoom=0.88, wind=0.4, **LAY)
    r.ground(0, 49, 38)
    r.fill(0, 28, 6, 37)
    r.fill(7, 33, 16, 37)
    r.fill(46, 3, 49, 37)                       # a sheer face on the right
    steps = [(18, 30, "a"), (24, 27, "b"), (30, 24, "a"), (24, 21, "b"), (16, 18, "a"),
             (10, 15, "b"), (18, 12, "a"), (26, 9, "#")]
    for x, y, ch in steps:
        r.fill(x, y, x + 5, y, ch)
    r.fill(32, 9, 49, 9)                        # top gallery
    r.clear(46, 4, 49, 8)
    r.fill(2, 12, 9, 12)                        # a high alcove reached from the b-step at row 15
    r.fill(2, 8, 2, 11)
    r.fill(2, 7, 6, 7)
    r.spawn("chest", 4, 11, "thread", item="hollow_thread")
    r.spawn("scroll", 40, 8, "scroll", lore="scroll_weaver")
    r.spawn("hollow", 12, 32, face="right", patrol=2)
    r.spawn("snare", 38, 8, face="left")
    r.spawn("hollow", 44, 8, face="left")
    r.at(33, 23, "o", "mid_bead")
    r.prop("thread", 40, 37, h=30)
    r.mark("exit", 49, 8)
    return r


def gallery():
    r = Room("gallery", "The Hall of Reflections", 70, H, palette="hollow", style="mirror", zoom=0.95, wind=0.2, **LAY)
    r.ground(0, 69, 10)
    r.clear(0, 0, 69, 8)
    r.fill(0, 9, 12, 9)
    r.clear(13, 9, 69, 9)
    r.fill(13, 0, 69, 2, "W")
    r.ground(0, 69, 30)
    r.clear(0, 10, 69, 29)
    r.fill(0, 10, 3, 29, "W")
    r.fill(10, 12, 14, 12, "=")
    r.fill(6, 16, 9, 16, "=")
    r.fill(10, 20, 14, 20, "=")
    r.fill(6, 24, 9, 24, "=")
    r.fill(30, 27, 36, 27, "a")
    r.fill(40, 27, 45, 27, "b")
    r.spawn("scroll", 43, 26, "scroll", lore="scroll_kagami")
    r.spawn("trial", 60, 29, "trial", reward="mirror", time=90,
            foes="hollow:-6,snare:7,hollow:9,hollow:-9,snare:-5")
    r.spawn("hollow", 24, 29, face="right", patrol=3)
    r.at(33, 26, "o", "a_bead")
    r.prop("thread", 20, 29, h=18)
    r.prop("thread", 54, 29, h=18)
    r.trigger(56, 18, 2, 12, "hint trial")
    r.mark("exit", 69, 29)
    return r


def mere():
    r = Room("mere", "The Still Mere", 64, H, palette="hollow", style="mirror", zoom=0.92, wind=0.3, boss=1)
    r.ground(0, 63, 32)
    r.fill(60, 6, 63, 31)
    r.fill(14, 26, 19, 26, "=")
    r.fill(44, 26, 49, 26, "=")
    r.spawn("gate", 4, 31, h=6, id="boss_door", open=1)
    r.spawn("boss", 42, 31, who="kagami", face="left")
    r.prop("thread", 10, 31, h=24)
    r.prop("thread", 54, 31, h=24)
    r.trigger(10, 18, 2, 14, "cutscene boss_intro")
    r.mark("boss", 42, 31)
    return r


def main():
    rooms = [shore(), orchard(), village(), stair(), gallery(), mere()]
    header = """SHADOW VEIL — Chapter V: The Mirror Province
Generated by tools/levels/ch5.py (levelkit). Edit the script, not this file.
a = stone on the Bright side only   b = stone on the Hollow side only (dash to step between them)"""
    ok = write_level(os.path.join(ROOT, "data/levels/ch5_mirror_province.lvl"), "ch5", "The Mirror Province", "鏡国", rooms, header,
                     preview_path=os.path.join(os.environ.get("SV_PREVIEW_DIR", "/tmp"), "ch5_preview.png"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
