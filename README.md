# SHADOW VEIL — 影ノ帳

[![CI](https://github.com/AbdulrahmanXSO25/shadow-veil/actions/workflows/ci.yml/badge.svg)](https://github.com/AbdulrahmanXSO25/shadow-veil/actions/workflows/ci.yml)

A cinematic 2D ninja action game for Linux, written in C++20 with SDL3 and OpenGL. All art, animation,
music and sound are generated at runtime from code. Nothing is sampled or sprite-based.

**Status:** this is a vertical slice. *Chapter I: The Ashen Grove* is complete and playable from start to
finish (10 handcrafted rooms, a two-phase boss, cinematics, progression and records). Chapters II–V are designed
but not yet built; see [`docs/ROADMAP.md`](docs/ROADMAP.md).

Design bible: [`docs/DESIGN_BIBLE.md`](docs/DESIGN_BIBLE.md) · Level format: [`data/levels/README.md`](data/levels/README.md)

---

## Build & run (Arch Linux, Wayland)

```sh
sudo pacman -S --needed sdl3 freetype2 zlib mesa cmake ninja gcc   # all present on a typical desktop install
cmake -S . -B build -G Ninja
cmake --build build
./build/shadow_veil
```

* The game runs **natively on Wayland**. XWayland is the automatic fallback, and `--x11` forces it.
* Saves, settings and the records profile live in `$XDG_DATA_HOME/shadow-veil/` (usually `~/.local/share/shadow-veil/`).
* `cmake --install build --prefix ~/.local` installs the binary and its content to `share/shadow-veil`.

### Command line
| Flag | Purpose |
|---|---|
| `--fullscreen`, `--size WxH` | Window mode. **F11** toggles fullscreen in-game; **F12** saves a screenshot. |
| `--render-size WxH` | Render at a fixed internal resolution, scaled to the window. |
| `--play`, `--fresh`, `--start <room\|spawn-id>` | Skip the title / ignore the save / start at a room (development). |
| `--autoplay <script>` | Drive input from a script and take screenshots (automated play-testing). |
| `--user-dir <dir>` | Use a separate save directory. |
| `--stats` | Print FPS, CPU and GPU frame times every 5 s. |

---

## Controls: designed for a controller

SHADOW VEIL is designed for a **PS4/PS5 or Xbox controller**. Keyboard and mouse is a complete fallback
mapping. All prompts show drawn button icons for the device you are using (PlayStation shapes, Xbox
letters, or keycaps). The full list is in-game under **How to Play**.

| Action | PlayStation | Xbox | Keyboard (fallback) |
|---|---|---|---|
| Move (a light tilt walks silently) | Left stick / D-pad | Left stick / D-pad | A / D |
| Jump (hold for height) | ✕ | A | Space |
| Strike (3–4 hit combo) | □ | X | J / Left mouse |
| Guard · tap to **Deflect** | L1 | LB | K / Right mouse |
| Veil-Step (untouchable dash) | ○ | B | Shift / L |
| Veil-Thread (grapple, hold) | R1 | RB | F / I |
| Throw shuriken | R2 | RT | R / U |
| Aim throws & thread · camera peek | Right stick | Right stick | W / S lean the throw |
| Ember Art · **Interact when prompted** | △ | Y | Q (Art) · E / W (Interact) |
| Heal (gourd) | L2 | LT | C / H |
| Crouch · drop through beams | Down · Down + ✕ | Down · Down + A | S · S + Space |
| Equipment | Touchpad | View | Tab |
| Pause · hold to skip a scene | Options | Menu | Esc |

Controller-specific features:

- **Analog stealth:** footsteps are silent below half tilt.
- **Right-stick aiming and camera peek.**
- **Light bar** shows Rei's state: crimson normally, teal with the Gale Stance, an orange pulse when Ember is ready, a red pulse at low vitality, deep red during the boss.
- **Haptics:** a sharp deflect, a heavy execution, a warning pulse when a perilous attack winds up, a heartbeat when near death, and a countdown in the last ten seconds of a trial.

## Core techniques
* **Deflect:** tap Guard as a blow lands. It does no damage, breaks the attacker's posture, and sends arrows back.
* **Posture & Execution:** a broken foe shows a vermilion mark; Strike to execute. Bosses need one execution per pip.
* **Perilous 危:** red diamond telegraphs can't be guarded. Veil-Step through them or jump.
* **Stealth:** a ring above a foe fills as they notice you. Crouch in tall grass to vanish. Assassinate from behind or above.
* **Kage Rank:** a D → SS style meter. Varied play raises it, repetition is worth less, and getting hit drops it. It feeds the chapter score.
* **Stances:** win the Wind Trial at the Ravine's wind beacon to awaken the **Gale Stance**. It gives faster movement and wind blades, and opens the Wind skill branch.

---

## What's in the slice
* **Story:** cold open, 10 rooms with authored set pieces, NPC Fuki, the Tetsu duel, and the Odaka revelation.
* **Combat:**
  * posture, deflects, perilous attacks, executions, stealth kills, ember projectiles, wind blades, shuriken
  * three enemy types with sight cones, alert propagation and attack tokens
  * a two-phase boss with a flame phase
* **Traversal:** variable jump, coyote time and input buffering, wall-slide and wall-jump, air-dash, and a pendulum grapple.
* **Progression:**
  * 2 blades, 3 garbs, 2 masks, 4 tokens, 3 socketable gems
  * a 12-skill tree in 4 branches, a forge with 6 recipes and materials
  * resin beads, 5 lore scrolls, 21 achievements, New Game+
* **Competition:** Kage Rank, chapter score and grade (C → S+), a personal-best score, the trial time record, and a speedrun achievement.
* **Presentation:**
  * colour script blended per room, procedural parallax worlds, rim-lit silhouettes, verlet cloth
  * HDR bloom and grading, letterboxed in-engine cinematics, a generative score that reacts to combat
* **Accessibility:** shake scale, flash reduction, deflect-window assist, game speed 60–100 %, hold/toggle guard, subtitle size, high-contrast foes, ink/petal effects, hint toggle.

## Testing
```sh
cmake --build build && ./build/sv_tests      # or: ctest --test-dir build
```
The suite drives the real `World` simulation headless and has 360 checks:
* level and script validation
* save, settings and profile round-trips
* progression rules
* running and jump arcs, a scripted wall-jump climb
* combo kills, a timed deflect, execution, stealth kill, grass hiding
* the bell and bridge puzzles, the wind trial (win, fail, retry)
* stance gating, Kage Rank, right-stick aim, the two-deathblow boss
* finite audio synthesis

Visual verification uses `--autoplay` scripts with `shot` commands (see `src/main.cpp`).

Measured on an AMD Renoir iGPU at 1920×1080 with 4× MSAA: about **5 ms GPU** and **1.5 ms CPU** per frame.

## Code map
| Path | Contents |
|---|---|
| `src/core/` | math, RNG/noise, file and XDG paths, controller-first input |
| `src/render/` | batched GL renderer, sky shader, bloom/grade post chain, FreeType text, PNG capture |
| `src/audio/` | synthesized SFX bank, generative music (Karplus–Strong koto, shakuhachi, taiko, drone), ambience, reverb |
| `src/game/` | skeleton/animation, level loader, player, enemies, world, style & trials, cutscenes, progression, game flow & UI |
| `src/ui/` | device-aware button glyphs and rich text |
| `data/` | `levels/*.lvl` (room-based ASCII levels), `scripts/*.cut` (cutscene language) |
| `assets/fonts/` | Noto Serif + a subset of Noto Serif CJK (OFL). Regenerate with `tools/subset_fonts.sh`. |

## Contributing
Bug reports, small fixes and level or documentation contributions are welcome. Read
[`CONTRIBUTING.md`](CONTRIBUTING.md) first; the short version is to build it, run `ctest`, and describe how the
change was tested. Security issues should go through [`SECURITY.md`](SECURITY.md), not the public tracker.

## License
The game and its source are released under the MIT license; see [`LICENSE`](LICENSE). The bundled Noto Serif fonts
are under the SIL Open Font License, in [`assets/fonts/OFL.txt`](assets/fonts/OFL.txt).
