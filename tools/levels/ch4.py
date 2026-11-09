"""Chapter IV — Castle Hakuro.

First snow on the White Wolf's castle. Rei climbs the outer wall under sweeping beacon light,
slips through a court full of hounds, frees the captives from the quarry cells — Hana among them —
and climbs the keep to face Lord Hakuro, who has promised Odaka a road to the edge of the Veil.
Run:  python3 tools/levels/ch4.py
"""
import os
import sys
sys.path.insert(0, os.path.dirname(__file__))
from levelkit import Room, write_level

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
H = 40


def road():
    r = Room("road", "The White Road", 64, H, palette="snow", style="castle", zoom=1.0, music="title", wind=0.7)
    r.ground(0, 63, 34)
    r.fill(26, 32, 33, 33)                      # the new road: dressed stone, still raw
    r.fill(34, 30, 63, 33)
    r.at(4, 33, "P")
    r.at(12, 33, "S", "camp")
    r.spawn("examine", 20, 33, text="A paved road, three carts wide, running straight up into the snow. Every stone has a mason's mark. Minase marks.")
    r.fill(48, 22, 52, 29)                      # a watch post on the road
    r.fill(47, 22, 47, 22, "=")
    r.spawn("gunner", 50, 21, face="left")
    r.spawn("swordsman", 40, 29, patrol=3, face="left")
    r.spawn("hound", 58, 29, face="left", patrol=3)
    r.at(44, 29, "m")
    r.prop("pine", 2, 33, h=6)
    r.prop("pine", 22, 33, h=7)
    r.prop("cart", 36, 29, w=3)
    r.prop("rocks", 30, 31, w=2)
    r.trigger(2, 20, 4, 14, "cutscene intro")
    r.trigger(38, 18, 2, 12, "hint gunner")
    r.trigger(54, 18, 2, 12, "hint hound")
    r.mark("exit", 62, 29)
    return r


def wall():
    """Up the outer wall: a dry moat, a buttress to wall-jump, rings, and a beacon sweeping the parapet."""
    r = Room("wall", "The Outer Wall", 58, H, palette="snow", style="castle", zoom=0.88, wind=0.8)
    r.ground(0, 57, 38)
    r.fill(0, 30, 6, 37)                        # the road's end
    r.fill(7, 35, 22, 37)                       # the dry moat floor
    r.fill(23, 9, 57, 37, "W")                  # the wall itself
    r.clear(23, 9, 57, 12)                      # the parapet walk
    r.fill(23, 13, 57, 13)                      # parapet floor
    r.fill(23, 9, 23, 12, "W")                  # crenel at the lip
    r.clear(23, 11, 23, 12)
    r.fill(8, 29, 12, 29)                       # scaffold left by the masons
    r.fill(14, 26, 18, 26)
    r.fill(19, 23, 22, 23)
    r.fill(15, 20, 18, 20)
    r.at(20, 14, "G")                           # a ring hammered into the stonework
    r.fill(11, 17, 13, 17, "=")
    r.spawn("scroll", 12, 16, "scroll", lore="scroll_mason")
    r.spawn("beacon", 40, 9, range=10, speed=0.6, h=3)
    r.spawn("hound", 12, 34, face="right", patrol=4)
    r.spawn("gunner", 50, 12, face="left")
    r.spawn("swordsman", 34, 12, patrol=3, face="right", sleep=1)
    r.at(55, 12, "o", "parapet_bead")
    r.prop("fence", 8, 29, w=4)
    r.trigger(16, 14, 3, 10, "hint beacon")
    r.trigger(8, 26, 2, 4, "hint walljump")
    r.mark("exit", 57, 12)
    return r


def court():
    """The lower court: lantern light everywhere. The test of moving unseen."""
    r = Room("court", "The Lower Court", 92, H, palette="court", style="castle", zoom=0.95, wind=0.4, stealth=1)
    r.ground(0, 91, 26)
    r.fill(0, 13, 4, 25)                        # down from the parapet
    r.fill(5, 17, 8, 25)
    r.fill(9, 21, 12, 25)
    r.grass(14, 22, 25)
    r.fill(24, 21, 34, 25, "W")                 # a storehouse; a false back wall hides the mantle
    r.clear(25, 22, 33, 25)
    r.fill(34, 22, 34, 25, "%")
    r.fill(24, 21, 34, 21)
    r.spawn("chest", 28, 25, "mantle", item="wolf_mantle")
    r.fill(44, 20, 60, 20, "=")                 # a covered gallery to sneak along
    r.fill(44, 21, 44, 22)                      # gallery posts
    r.fill(60, 21, 60, 22)
    r.grass(62, 72, 25)
    r.fill(74, 22, 91, 25)
    r.spawn("beacon", 38, 10, range=12, speed=0.5, h=3)
    r.spawn("beacon", 68, 10, range=12, speed=0.7, h=3)
    r.spawn("hound", 50, 25, patrol=5, face="left")
    r.spawn("swordsman", 20, 25, patrol=3, face="left")
    r.spawn("spearman", 52, 19, patrol=4, face="right")
    r.spawn("gunner", 80, 21, face="left")
    r.spawn("hound", 66, 25, patrol=3, face="right", sleep=1)
    r.prop("pine", 16, 25, h=5)
    r.prop("lantern", 40, 25)
    r.prop("lantern", 70, 25)
    r.prop("banner", 86, 21, h=4)
    r.at(57, 25, "o", "gallery_bead")
    r.trigger(6, 4, 2, 13, "unseen_enter")
    r.trigger(7, 4, 2, 13, "hint shadow")
    r.trigger(84, 8, 2, 14, "unseen_exit castle_unseen")
    r.mark("exit", 90, 21)
    return r


def kennels():
    r = Room("kennels", "The Kennels", 62, H, palette="court", style="castle", zoom=1.0, wind=0.3)
    r.ground(0, 61, 26)
    r.fill(0, 22, 12, 25)
    r.fill(13, 25, 48, 25, "#")
    r.fill(14, 21, 16, 24)                      # feed crates
    r.fill(18, 18, 26, 18, "=")                 # a loft over the runs
    r.fill(32, 15, 40, 15, "=")
    r.fill(49, 22, 61, 25)
    r.fill(46, 18, 48, 18, "=")
    r.at(36, 14, "o", "loft_bead")
    r.spawn("scroll", 24, 17, "scroll", lore="scroll_kennel")
    r.spawn("hound", 22, 24, patrol=3)
    r.spawn("hound", 34, 24, patrol=3, face="left")
    r.spawn("hound", 42, 24, patrol=2, face="left")
    r.spawn("trial", 55, 21, "trial", reward="shadow", time=85,
            foes="hound:-6,hound:6,swordsman:-8,gunner:9,hound:-10")
    r.prop("fence", 14, 25, w=6)
    r.prop("fence", 28, 25, w=6)
    r.trigger(50, 10, 2, 12, "hint trial")
    r.mark("exit", 60, 21)
    return r


def cells():
    """The quarry cells: the Minase captives, and a winch for the cell gate."""
    r = Room("cells", "The Quarry Cells", 70, H, palette="wolf", style="castle", zoom=1.05, music="camp", wind=0.2)
    r.ground(0, 69, 26)
    r.fill(0, 22, 69, 25)
    r.fill(0, 6, 69, 9)                         # rock ceiling: this is under the keep
    r.fill(30, 18, 46, 18)                      # the cell block's roof, a walkway
    r.fill(46, 19, 46, 21)
    r.spawn("gate", 30, 21, h=3, id="ch4.cells")    # the cell gate, lifted from above
    r.fill(26, 20, 28, 20)                      # a step up to the roof
    r.fill(57, 19, 59, 19)
    r.fill(53, 16, 56, 16)
    r.spawn("sluice", 55, 15, "winch", link="ch4.cells")
    r.at(12, 21, "S", "camp")
    r.spawn("npc", 34, 21, who="hana", id="hana", face="left")
    r.spawn("npc", 40, 21, who="refugee", id="cap1", face="left")
    r.spawn("npc", 43, 21, who="child", id="cap2", face="left")
    r.spawn("npc", 42, 21, who="mason", id="cap3", face="right")
    r.spawn("swordsman", 24, 21, patrol=3, face="right")
    r.spawn("spearman", 60, 21, patrol=3, face="left")
    r.spawn("examine", 20, 21, text="Picks, chisels, and a wall of tally marks: days worked, scratched by many hands.")
    r.prop("lantern", 16, 21)
    r.prop("lantern", 50, 21)
    r.trigger(26, 10, 2, 12, "cutscene cells_see")
    r.mark("exit", 69, 21)
    return r


def keep():
    """The keep stair: a tall shaft of stairs and galleries, gunners on every landing."""
    r = Room("keep", "The Keep Stair", 54, H, palette="wolf", style="castle", zoom=0.88, wind=0.3)
    r.ground(0, 53, 38)
    r.fill(0, 22, 4, 37)                        # from the cells
    r.fill(5, 26, 10, 37)
    r.fill(11, 30, 53, 37)
    r.fill(0, 0, 53, 2, "W")                    # the keep roof beams
    r.fill(51, 3, 53, 29, "W")
    r.fill(18, 27, 26, 27, "=")
    r.fill(30, 24, 38, 24)
    r.fill(42, 21, 50, 21)
    r.fill(32, 18, 40, 18, "=")
    r.fill(20, 15, 28, 15)
    r.fill(10, 12, 18, 12, "=")
    r.fill(22, 9, 50, 9)                        # the top gallery to the hall
    r.clear(51, 4, 53, 8)
    # the lord's study: a niche off the top gallery
    r.fill(2, 4, 8, 4)
    r.fill(2, 5, 2, 8)
    r.fill(2, 9, 9, 9)
    r.spawn("scroll", 5, 8, "scroll", lore="scroll_odaka_pact")
    r.spawn("chest", 46, 20, "sapphire", item="frost_sapphire")
    r.spawn("scroll", 34, 23, "scroll", lore="scroll_hakuro")
    r.spawn("gunner", 36, 23, face="left")
    r.spawn("gunner", 26, 14, face="right")
    r.spawn("swordsman", 44, 8, patrol=3, face="left")
    r.spawn("spearman", 36, 17, patrol=2)
    r.prop("banner", 30, 29, h=5)
    r.prop("banner", 12, 25, h=4)
    r.mark("exit", 53, 8)
    return r


def hall():
    r = Room("hall", "The White Hall", 64, H, palette="wolf", style="castle", zoom=0.92, wind=0.5, boss=1)
    r.ground(0, 63, 30)
    r.fill(0, 0, 63, 3, "W")                    # roof beams
    r.fill(0, 9, 10, 9)                         # the landing at the top of the stair
    r.fill(0, 10, 3, 29, "W")
    r.fill(60, 4, 63, 29, "W")
    r.fill(12, 23, 17, 23, "=")
    r.fill(46, 23, 51, 23, "=")
    r.spawn("gate", 8, 29, h=6, id="boss_door", open=1)
    r.spawn("boss", 44, 29, who="hakuro", face="left")
    r.prop("banner", 20, 29, h=7)
    r.prop("banner", 40, 29, h=7)
    r.prop("lantern", 30, 29)
    r.trigger(12, 18, 2, 12, "cutscene boss_intro")
    r.mark("boss", 44, 29)
    return r


def main():
    rooms = [road(), wall(), court(), kennels(), cells(), keep(), hall()]
    header = """SHADOW VEIL — Chapter IV: Castle Hakuro
Generated by tools/levels/ch4.py (levelkit). Edit the script, not this file."""
    ok = write_level(os.path.join(ROOT, "data/levels/ch4_castle_hakuro.lvl"), "ch4", "Castle Hakuro", "白狼城", rooms, header,
                     preview_path=os.path.join(os.environ.get("SV_PREVIEW_DIR", "/tmp"), "ch4_preview.png"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
