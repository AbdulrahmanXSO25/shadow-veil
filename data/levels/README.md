# Level format (`.lvl`)

Levels are a horizontal sequence of **rooms**. Each room is drawn as an ASCII map of exactly `height` rows,
and rooms are placed left to right. Each room carries its own palette, background style, music and camera
zoom. Palettes blend smoothly across room boundaries, which is how the colour script is authored.

```
level ch1
title "The Ashen Grove"
kanji "灰竹林"
height 30

room grove "The Bamboo Grove" palette=grove style=grove zoom=1.0 wind=0.35 crickets=1 stealth=1
map
....................   (exactly `height` rows; width is free per room)
endmap
spawn swordsman 18 23 patrol=4
prop bamboo 20 23 w=3 n=5
trigger 2 10 1 14 grove_enter
endroom
```

**Room options:**
* `palette` = `moon`, `ember`, `grove`, `ravine` or `gate`
* `style` = `cliff`, `village`, `grove`, `ravine` or `gate`
* `zoom`
* `music` = `explore`, `title`, `camp`, `tension` or `silent`
* ambience levels: `wind`, `fire`, `crickets`
* flags: `boss` (locks the camera) and `stealth` (detection counts against the *Unseen* achievement)

## Map legend
| Char | Meaning | Char | Meaning |
|---|---|---|---|
| `#` | earth / stone | `P` | player start (exactly one per level) |
| `=` | wooden planks (solid) | `S` | camp (checkpoint) |
| `W` | timber house wall (solid) | `s` `y` `r` | swordsman, spearman, archer |
| `-` | one-way beam | `T` | boss |
| `^` | stakes (damage + respawn) | `G` | Veil-Thread ring |
| `%` | false wall (secret) | `$` `*` `o` `m` | mon, shuriken, resin bead, material |
| `B` | breakable bamboo fence | `L` `l` `f` | stone lantern, hanging lantern, fire (also lights) |
| `h` | tall grass (hides when crouched) | `.` | empty |

## Extra lines (coordinates are room-local tiles; `y` is the row the object stands in)
- `spawn <kind> x y key=value…` places a spawn of one of these kinds:
  - `swordsman`, `spearman`, `archer`, `boss`: options `face=left|right`, `patrol=<tiles>`, `sleep=1`
  - `gate` (`h`, `id`, `open=1`), `bell` (`link`), `rope` (`h`, `link`), `bridge` (`len`, `id`)
  - `chest` (`item`), `scroll` (`lore`), `examine` (`text`), `npc` (`who`, `id`, `talk=<scene>`, `hidden=1`)
  - `material` (`item`), `trial` (`reward`, `time`, `foes=kind:dx,kind:dx…`)
- `prop <kind> x y key=value…` places set dressing:
  - `house` (`w`, `h`, `burning`), `bamboo` (`n`), `pine`/`maple` (`h`)
  - `statue`, `banner`, `fence`, `cart`, `rocks`, `toy`, `gate_arch`
  - add `front=1` to draw the prop in front of actors
- `trigger x y w h <action> [repeat]` fires one of these actions:
  - `cutscene <name>`, `hint <id>`, `music <mode|none>`, `boss`, `flag <id>`
  - `grove_enter`, `grove_exit`, `toast <title>`, `chapter_end`

The test suite (`sv_tests`) validates every level:
- row counts and a single player start
- unique ids
- chest, material and lore references
- every referenced cutscene exists
