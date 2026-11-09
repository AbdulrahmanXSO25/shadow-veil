"""Chapter II — The Drowned Terrace.

Down the far side of the pass, Lord Hakuro's river captain has closed the sluices and drowned the
rice terraces of Minase to flush out anyone who might resist. Rei swims, drains and fights her way
to the Great Sluice, finds Hana among the refugees, and watches her carried upriver to the castle.
Run:  python3 tools/levels/ch2.py
"""
import os
import sys
sys.path.insert(0, os.path.dirname(__file__))
from levelkit import Room, write_level

H = 30
ROOT = os.path.join(os.path.dirname(__file__), "..", "..")


def landing():
    r = Room("landing", "Rain Landing", 58, H, palette="rain", style="terrace", zoom=1.0, music="title", wind=0.5, rain=1, water=0.3)
    r.ground(0, 57, 24)
    # a sunken pool: the first swim
    r.clear(20, 24, 31, 27)
    r.water(20, 31, 24, 27)
    r.ground(36, 57, 22)
    r.at(4, 23, "P")
    r.at(10, 23, "S", "camp")
    r.at(26, 27, "o", "bead")
    r.spawn("examine", 15, 23, text="The terraces below are under water. Five days of rain, and the sluices on the river are all shut.")
    r.spawn("material", 50, 21, item="spirit_silk")
    r.prop("pine", 1, 23, h=6)
    r.prop("pine", 44, 21, h=5)
    r.prop("rocks", 33, 23, w=2)
    r.trigger(2, 10, 4, 14, "cutscene intro")
    r.trigger(17, 14, 2, 10, "hint swim")
    r.mark("exit", 56, 21)
    return r


def flooded():
    r = Room("flooded", "The Flooded Steps", 86, H, palette="flood", style="terrace", zoom=1.0, wind=0.4, rain=1, water=0.5)
    r.ground(0, 85, 26)
    r.fill(0, 22, 13, 25)                       # entry terrace
    r.water(14, 27, 22, 25)                     # paddy one
    r.fill(28, 21, 30, 25)                      # dike
    r.water(31, 46, 22, 25)                     # paddy two (a chest on the bottom)
    r.fill(47, 21, 49, 25)                      # dike
    r.fill(50, 22, 63, 25)                      # walkway with the keeper's hut
    r.water(64, 75, 22, 25)                     # paddy three, boardwalk above
    r.beam(66, 73, 19, "=")
    r.fill(76, 22, 85, 25)
    r.at(20, 25, "q")
    r.at(41, 25, "q")
    r.at(70, 25, "q")
    r.spawn("chest", 37, 25, "garb", item="tide_garb")
    r.spawn("reaver", 56, 21, patrol=3, face="left")
    r.spawn("reaver", 80, 21, patrol=2, face="left")
    r.spawn("archer", 72, 18, face="left")
    r.spawn("scroll", 61, 21, "scroll", lore="scroll_sluice")
    r.spawn("examine", 29, 20, text="Rice stalks under a hand of water. The whole harvest is drowning.")
    r.at(9, 21, "$")
    r.at(83, 21, "m")
    r.prop("paddy", 15, 22, w=12)
    r.prop("paddy", 32, 22, w=14)
    r.prop("paddy", 65, 22, w=10)
    r.prop("house", 57, 21, w=6, h=3)
    r.prop("boat", 33, 22, w=4)
    r.trigger(11, 12, 2, 10, "hint lurker")
    r.mark("exit", 84, 21)
    return r


def sluice():
    r = Room("sluice", "The Upper Sluice", 66, H, palette="flood", style="terrace", zoom=0.95, wind=0.4, rain=1, water=0.8)
    r.ground(0, 65, 26)
    r.fill(0, 22, 18, 25)                       # left bank
    r.beam(9, 13, 18)
    r.beam(13, 17, 15)
    r.fill(12, 12, 18, 13)                      # the wheel ledge
    r.fill(19, 12, 19, 25)                      # basin wall (left)
    r.fill(20, 0, 65, 8)                        # rock ceiling over basin and spillway
    r.fill(44, 9, 44, 25)                       # floodgate wall
    r.clear(44, 20, 44, 25)                     # the gate fills this gap
    r.water(20, 43, 14, 25)                     # the reservoir
    r.fill(54, 22, 65, 25)                      # steps up toward Minase
    r.fill(60, 9, 65, 17)
    r.spawn("sluice", 16, 11, "wheel", link="ch2.sluice_gate")
    r.spawn("gate", 44, 25, h=6, id="ch2.sluice_gate")
    r.spawn("drain", 20, 25, w=24, h=12, id="ch2.sluice_gate", mode="drain")
    r.at(31, 25, "o", "basin")
    r.spawn("reaver", 6, 21, patrol=3)
    r.spawn("archer", 15, 11, face="left")
    r.spawn("reaver", 50, 25, patrol=3, face="left")
    r.spawn("swordsman", 58, 21, face="left", patrol=2)
    r.spawn("examine", 41, 13, text="A floodgate, chained shut from above. The wheel that lifts it must be up on the ledge.")
    r.trigger(4, 12, 2, 10, "cutscene sluice_view")
    r.trigger(15, 6, 3, 6, "hint sluice")
    r.mark("exit", 64, 21)
    return r


def minase():
    r = Room("minase", "Minase Rooftops", 80, H, palette="rain", style="terrace", zoom=0.95, wind=0.4, rain=1, water=0.5)
    r.ground(0, 79, 28)
    r.fill(0, 22, 4, 27)                        # the dock
    r.water(5, 79, 22, 27)
    houses = [(9, 15, 18), (20, 28, 16), (33, 46, 17), (51, 58, 18), (63, 71, 16)]
    for x0, x1, top in houses:
        r.fill(x0, top + 1, x1, 27, "W")
        r.beam(x0 - 1, x1 + 1, top, "=")
    r.fill(75, 22, 79, 27)                      # far bank
    r.at(38, 16, "S", "camp")
    r.spawn("npc", 42, 16, who="hana", id="hana", talk="hana_talk", face="left")
    r.spawn("npc", 35, 16, who="refugee", id="ref1", talk="refugee_talk", face="right")
    r.spawn("npc", 24, 15, who="child", id="ref2", face="right")
    r.spawn("npc", 66, 15, who="refugee", id="ref3", face="left")
    r.spawn("scroll", 55, 17, "scroll", lore="scroll_minase")
    r.at(17, 26, "q")
    r.at(49, 26, "q")
    r.at(61, 26, "q")
    r.spawn("reaver", 25, 15, patrol=2, face="left")
    r.spawn("reaver", 68, 15, patrol=2, face="left")
    r.at(13, 17, "*")
    r.at(60, 27, "o", "bead")
    r.prop("boat", 29, 22, w=3)
    r.prop("boat", 72, 22, w=3, pole=1)
    r.trigger(30, 8, 3, 9, "cutscene hana_meet")
    r.mark("exit", 78, 21)
    return r


def reeds():
    r = Room("reeds", "The Reed Channel", 90, H, palette="tide", style="terrace", zoom=1.0, wind=0.5, rain=0.6, water=0.7, stealth=1)
    r.ground(0, 89, 24)
    r.fill(0, 22, 6, 23)                        # off the far bank of Minase
    r.clear(14, 23, 52, 23)
    r.water(14, 52, 23, 23)                     # wading channel
    r.clear(24, 24, 32, 26)
    r.water(24, 32, 24, 26)                     # a deep hole under the boat
    r.grass(7, 13, 23)
    r.grass(53, 64, 23)
    r.fill(65, 21, 70, 23)                      # bank rise
    r.fill(78, 22, 89, 23)
    r.fill(72, 18, 77, 23)                      # an old stone weir-house with a hidden niche
    r.clear(73, 22, 76, 23)
    r.fill(72, 22, 72, 23, "%")
    r.grass(80, 86, 21)
    r.spawn("chest", 75, 23, "pearl", item="rain_pearl")
    r.at(29, 26, "q")
    r.spawn("reaver", 18, 22, patrol=4)
    r.spawn("reaver", 44, 22, patrol=3, sleep=1, face="left")
    r.spawn("archer", 67, 20, face="left")
    r.spawn("swordsman", 58, 23, patrol=3, face="left")
    r.spawn("scroll", 28, 22, "scroll", lore="scroll_ferry")
    r.spawn("trial", 84, 21, "trial", reward="stillwater", time=80, foes="reaver:-6,reaver:6,swordsman:-9,archer:9,reaver:-10")
    r.prop("boat", 26, 23, w=5)
    r.prop("paddy", 33, 23, w=10)
    r.at(60, 23, "m")
    r.trigger(8, 12, 2, 12, "hint stealth")
    r.trigger(80, 12, 2, 10, "hint trial")
    r.mark("exit", 88, 21)
    return r


def weir():
    r = Room("weir", "Weir Camp", 50, H, palette="tide", style="terrace", zoom=1.1, music="camp", wind=0.3, rain=0.4, water=1)
    r.ground(0, 49, 24)
    r.fill(0, 22, 33, 23)                       # the bank
    r.water(34, 49, 22, 23)                     # the weir pool
    r.beam(34, 49, 21, "=")                     # plank walkway along the weir
    r.at(12, 21, "S", "camp")
    r.spawn("scroll", 22, 21, "scroll", lore="scroll_hana")
    r.spawn("examine", 27, 21, text="Fresh scrape marks on the bank, and a dropped hair ribbon. Pink. Hana's.")
    r.prop("pine", 2, 21, h=5)
    r.prop("stilts", 36, 21, w=6, h=1)
    r.trigger(24, 10, 2, 12, "cutscene boat_passes")
    r.mark("exit", 49, 20)
    return r


def greatsluice():
    r = Room("greatsluice", "The Great Sluice", 62, H, palette="tide", style="terrace", zoom=0.95, wind=0.6, rain=0.8, water=1, boss=1)
    r.ground(0, 61, 24)
    r.fill(58, 6, 61, 23)                       # the sluice-house wall
    r.spawn("gate", 4, 23, h=6, id="boss_door", open=1)
    r.spawn("boss", 42, 23, who="warden", face="left")
    r.spawn("drain", 6, 23, w=51, h=2, id="ch2.arena_flood", mode="fill")
    r.prop("gate_arch", 40, 23, w=18, h=12)
    r.trigger(10, 10, 2, 14, "cutscene boss_intro")
    r.mark("boss", 42, 23)
    return r


def main():
    rooms = [landing(), flooded(), sluice(), minase(), reeds(), weir(), greatsluice()]
    header = """SHADOW VEIL — Chapter II: The Drowned Terrace
Generated by tools/levels/ch2.py (levelkit). Edit the script, not this file.
New tiles: ~ water   c crumbling ledge   | rising wind   a/b Bright/Hollow layer stone
New foes: v reaver   q lurker (waits under water)   k duelist   j crow scout   n hound   g gunner   e hollow blade   z snare   K sentinel"""
    ok = write_level(os.path.join(ROOT, "data/levels/ch2_drowned_terrace.lvl"), "ch2", "The Drowned Terrace", "水田", rooms, header,
                     preview_path=os.path.join(os.environ.get("SV_PREVIEW_DIR", "/tmp"), "ch2_preview.png"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
