"""Chapter III — Crimson Maple Pass.

The river road climbs out of Minase into the maple pass that guards Castle Hakuro. The pass is held
by the Crow Band, and its leader Karasu. Rei climbs crumbling switchbacks, rides the wind gaps, and
learns that the "crows" are exiles from the Hollow who slipped out through the thinning Veil.
Run:  python3 tools/levels/ch3.py
"""
import os
import sys
sys.path.insert(0, os.path.dirname(__file__))
from levelkit import Room, write_level

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")


def foot():
    r = Room("foot", "Foot of the Pass", 60, 32, palette="maple", style="maple", zoom=1.0, music="title", wind=0.6)
    r.ground(0, 59, 26)
    r.fill(30, 24, 40, 25)                      # a terrace step
    r.fill(41, 22, 59, 25)
    r.at(4, 25, "P")
    r.at(12, 25, "S", "camp")
    r.spawn("examine", 20, 25, text="Maples all the way up, red as embers. Boot prints on the road, and a boat drawn up on the bank below.")
    r.spawn("swordsman", 47, 21, patrol=3, face="left")
    r.at(55, 21, "m")
    r.prop("maple", 2, 25, h=6)
    r.prop("maple", 26, 25, h=7)
    r.prop("maple", 50, 21, h=6)
    r.prop("rocks", 36, 23, w=2)
    r.trigger(2, 12, 4, 14, "cutscene intro")
    r.mark("exit", 58, 21)
    return r


def switchbacks():
    """A tall climb up crumbling stone: zig-zag ledges, some that drop away after a breath."""
    H = 44
    r = Room("switchbacks", "The Switchbacks", 56, H, palette="maple", style="maple", zoom=0.9, wind=0.7)
    r.ground(0, 55, 40)
    r.fill(0, 36, 4, 39)                        # entry shelf (from the foot of the pass)
    r.fill(53, 0, 55, 39)                       # cliff face on the right, a wall-jump surface
    # ledges, left-right zig-zag; each rise is at most three tiles
    r.fill(10, 37, 18, 37)
    r.fill(22, 34, 30, 34)
    r.fill(34, 31, 42, 31, "c")                 # first crumbling ledge
    r.fill(43, 31, 46, 31)
    r.fill(36, 28, 44, 28)
    r.fill(24, 25, 32, 25)
    r.fill(12, 22, 20, 22, "c")
    r.fill(6, 19, 11, 19)
    r.fill(14, 16, 24, 16)
    r.fill(28, 13, 34, 13, "c")
    r.fill(36, 10, 52, 10)                      # the top lip, exit to the right
    r.clear(53, 6, 55, 9)                       # opening in the cliff face at the top
    r.fill(2, 26, 3, 30)                        # a perch high on the left wall with a secret
    r.at(2, 25, "o", "perch_bead")
    r.spawn("scroll", 8, 18, "scroll", lore="scroll_duelist")
    r.spawn("duelist", 40, 27, face="left", patrol=2)
    r.spawn("crow", 28, 24, face="right")
    r.spawn("crow", 46, 9, face="left")
    r.spawn("archer", 18, 15, face="right")
    r.prop("maple", 24, 33, h=4)
    r.prop("maple", 16, 15, h=5)
    r.trigger(8, 30, 2, 8, "hint crumble")
    r.trigger(30, 30, 2, 8, "hint duelist")
    r.mark("exit", 54, 9)
    return r


def windgap():
    """A wide gorge. Rising wind carries Rei across; rings hang where the wind gives out."""
    H = 40
    r = Room("windgap", "The Wind Gap", 84, H, palette="ridge", style="maple", zoom=0.85, wind=1.2)
    r.ground(0, 83, 38)
    r.fill(0, 10, 14, 37)                       # west lip (from the switchbacks)
    r.fill(70, 12, 83, 37)                      # east lip
    r.water(15, 69, 34, 37)                     # a cold torrent at the bottom of the gorge
    r.fill(15, 33, 69, 33, ".")
    r.updraft(18, 21, 12, 33)                   # first wind column
    r.fill(30, 18, 34, 18, "=")                 # a timber stub mid-gorge
    r.updraft(40, 43, 10, 33)
    r.at(50, 9, "G")                            # ring past the second column
    r.fill(56, 14, 60, 14, "=")
    r.updraft(63, 66, 8, 33)
    r.fill(22, 30, 26, 30)                      # low shelf: the climb back after a fall
    r.fill(45, 30, 49, 30)
    r.spawn("scroll", 58, 13, "scroll", lore="scroll_crows")
    r.spawn("crow", 32, 17, face="right")
    r.spawn("crow", 74, 11, face="left", patrol=2)
    r.spawn("examine", 8, 9, text="Wind roaring up out of the gorge, strong enough to lean on. Black feathers caught in every crack.")
    r.at(24, 29, "o", "low_bead")
    r.at(47, 29, "m")
    r.prop("maple", 3, 9, h=6)
    r.prop("maple", 78, 11, h=7)
    r.trigger(10, 0, 3, 10, "hint updraft")
    r.mark("exit", 82, 11)
    return r


def bridge():
    """An old rope-and-plank bridge, half gone, held by a duelist who waits for challengers."""
    H = 36
    r = Room("bridge", "Duelists' Bridge", 70, H, palette="ridge", style="maple", zoom=0.95, wind=0.9)
    r.ground(0, 69, 32)
    r.fill(0, 12, 12, 31)                       # west cliff
    r.fill(58, 12, 69, 31)                      # east cliff
    r.fill(13, 12, 57, 12, "=")                 # the bridge deck
    r.clear(22, 12, 24, 12)                     # planks long gone: a hole to the ledge below
    r.fill(25, 12, 26, 12, "c")                 # rotten planks
    r.fill(40, 12, 42, 12, "c")
    r.water(13, 57, 28, 31)
    # below the bridge: a hidden ledge with the maple blade, reached by dropping through the rot
    r.fill(18, 22, 30, 22)
    r.fill(30, 17, 30, 21, "%")                 # false wall closes the ledge off from the gorge side
    r.fill(31, 22, 35, 22)
    r.updraft(36, 38, 13, 27)                   # wind back up to the deck
    r.spawn("chest", 20, 21, "blade", item="maple_blade")
    r.spawn("duelist", 34, 11, face="left")
    r.spawn("duelist", 50, 11, face="left", patrol=2)
    r.spawn("crow", 63, 11, face="left")
    r.at(9, 11, "S", "camp")
    r.at(25, 21, "o", "under_bead")
    r.prop("maple", 2, 11, h=6)
    r.prop("maple", 64, 11, h=5)
    r.trigger(14, 2, 2, 10, "cutscene bridge_duel")
    r.mark("exit", 68, 11)
    return r


def hamlet():
    """Where the crows live. Not a war camp — a village of people who should not exist."""
    H = 34
    r = Room("hamlet", "The Hidden Hamlet", 66, H, palette="crow", style="village", zoom=1.05, music="camp", wind=0.4)
    r.ground(0, 65, 28)
    r.fill(0, 12, 8, 27)                        # the road ends at a cliff; the hamlet is below it
    r.fill(9, 15, 12, 27)
    r.fill(13, 18, 15, 27)
    r.fill(16, 21, 18, 27)
    r.fill(24, 24, 40, 27)                      # the hamlet's raised yard
    r.fill(52, 22, 65, 27)
    r.at(30, 23, "S", "camp")
    r.spawn("npc", 34, 23, who="refugee", id="exile1", talk="exile_talk", face="left")
    r.spawn("npc", 37, 23, who="child", id="exile2", face="left")
    r.spawn("npc", 21, 27, who="mason", id="exile3", talk="exile_mason", face="right")
    r.spawn("scroll", 58, 21, "scroll", lore="scroll_exile")
    r.spawn("examine", 26, 23, text="Houses of black pine, roofed in maple bark. Built to be invisible from the road. They have been here for years.")
    r.prop("house", 42, 23, w=7, h=4)
    r.prop("house", 46, 27, w=4, h=3)
    r.prop("fire", 32, 23)
    r.prop("maple", 50, 21, h=6)
    r.trigger(19, 14, 2, 13, "cutscene hamlet")
    r.mark("exit", 64, 21)
    return r


def stormridge():
    H = 36
    r = Room("stormridge", "Storm Ridge", 76, H, palette="crow", style="maple", zoom=0.92, wind=1.0)
    r.ground(0, 75, 30)
    r.fill(0, 22, 10, 29)
    r.fill(11, 25, 20, 29)
    r.fill(21, 27, 23, 29)                      # a step to climb back out of the hollow
    r.fill(21, 22, 28, 22, "c")                 # crumbling ledge over the hollow
    r.fill(29, 22, 33, 22)
    r.fill(37, 23, 40, 23)                      # stepping stone
    r.fill(43, 22, 45, 29)
    r.fill(46, 20, 60, 29)                      # the ridge's high saddle
    r.fill(61, 24, 75, 29)
    # a climb back west, high above the road, to a hidden niche
    r.fill(22, 19, 26, 19)
    r.fill(15, 16, 20, 16)
    r.fill(7, 13, 13, 13)
    r.fill(6, 9, 13, 9)
    r.fill(6, 10, 6, 12)
    r.fill(13, 10, 13, 12, "%")
    r.spawn("chest", 9, 12, "agate", item="thunder_agate")
    r.spawn("trial", 68, 23, "trial", reward="thunder", time=85,
            foes="duelist:-6,crow:6,duelist:9,crow:-9,swordsman:-4")
    r.spawn("scroll", 52, 19, "scroll", lore="scroll_maple")
    r.spawn("duelist", 54, 19, face="left", patrol=3)
    r.spawn("crow", 24, 27, face="right")
    r.spawn("archer", 58, 19, face="left")
    r.at(5, 21, "o", "west_bead")
    r.prop("maple", 3, 21, h=5)
    r.prop("maple", 47, 19, h=6)
    r.trigger(62, 12, 2, 12, "hint trial")
    r.mark("exit", 74, 23)
    return r


def roost():
    H = 34
    r = Room("roost", "The Crow's Roost", 64, H, palette="crow", style="maple", zoom=0.92, wind=1.1, boss=1)
    r.ground(0, 63, 28)
    r.fill(60, 6, 63, 27)
    r.fill(14, 22, 20, 22, "=")                 # perches Karasu dives from — and Rei can use
    r.fill(43, 22, 49, 22, "=")
    r.fill(28, 17, 35, 17, "=")
    r.spawn("gate", 4, 27, h=6, id="boss_door", open=1)
    r.spawn("boss", 40, 27, who="crowking", face="left")
    r.prop("maple", 8, 27, h=8)
    r.prop("maple", 54, 27, h=9)
    r.trigger(10, 14, 2, 14, "cutscene boss_intro")
    r.mark("boss", 40, 27)
    return r


def main():
    H = 44
    rooms = [foot().pad_to(H, 12), switchbacks(), windgap().pad_to(H), bridge().pad_to(H), hamlet().pad_to(H),
             stormridge().pad_to(H), roost().pad_to(H)]
    header = """SHADOW VEIL — Chapter III: Crimson Maple Pass
Generated by tools/levels/ch3.py (levelkit). Edit the script, not this file."""
    ok = write_level(os.path.join(ROOT, "data/levels/ch3_maple_pass.lvl"), "ch3", "Crimson Maple Pass", "紅葉峠", rooms, header,
                     preview_path=os.path.join(os.environ.get("SV_PREVIEW_DIR", "/tmp"), "ch3_preview.png"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
