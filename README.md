# SHADOW VEIL — 影ノ帳

[![CI](https://github.com/AbdulrahmanXSO25/shadow-veil/actions/workflows/ci.yml/badge.svg)](https://github.com/AbdulrahmanXSO25/shadow-veil/actions/workflows/ci.yml)

A cinematic 2D shinobi action game for **Windows and Linux**, written in C++20 with SDL3 and OpenGL 3.3. All art,
animation, voice-and-wind score and sound are generated at runtime from code. Nothing is sampled or sprite-based.

**Download:** [abdulrahmanxso25.github.io/shadow-veil](https://abdulrahmanxso25.github.io/shadow-veil/) ·
[GitHub Releases](https://github.com/AbdulrahmanXSO25/shadow-veil/releases/latest)

**Status (v0.9.0, release candidate):** the full campaign is playable: six chapters, six champions (five of whom
can be spared), five stances, 25 ink scrolls and three endings. See [`CHANGELOG.md`](CHANGELOG.md) and the honest
list of what is still untested in [`docs/ROADMAP.md`](docs/ROADMAP.md).

Design bible: [`docs/DESIGN_BIBLE.md`](docs/DESIGN_BIBLE.md) · Level format: [`data/levels/README.md`](data/levels/README.md) ·
Steam: [`steam/README.md`](steam/README.md)

---

## Build & run

### Linux (Wayland or X11)
```sh
sudo pacman -S --needed sdl3 freetype2 zlib mesa cmake ninja gcc     # Arch; other distros: the -dev equivalents
cmake -S . -B build -G Ninja
cmake --build build
./build/shadow_veil
```
* The game runs **natively on Wayland**. XWayland is the automatic fallback, and `--x11` forces it.
* Saves, settings, records and `shadow-veil.log` live in `$XDG_DATA_HOME/shadow-veil/` (usually `~/.local/share/shadow-veil/`).

### Windows
* **MSVC** (Visual Studio 2022, "x64 Native Tools" prompt): `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
  SDL3, FreeType and zlib are fetched at pinned versions and linked statically.
* **Cross-compile from Linux** with MinGW-w64: `tools/package/build-windows.sh` (add `SV_WINE_TESTS=1` to run the
  test suite under Wine).
* Saves and the log live in `%APPDATA%\XSO\Shadow Veil\`.

### Release packages
| Script | Output |
|---|---|
| `tools/package/build-windows.sh` | `dist/ShadowVeil-<v>-windows-x64.zip`: one static `ShadowVeil.exe` with `data/` and `assets/` |
| `tools/package/build-linux.sh` | `dist/ShadowVeil-<v>-linux-x64.tar.gz`: SDL3/FreeType/libstdc++ linked statically; needs only glibc and libGL |
| `tools/package/publish-site.sh` | Publishes `site/` to the `gh-pages` branch (the website) |

Pushing a `v*` tag runs `.github/workflows/release.yml`, which builds and tests both packages in CI.

### Command line
| Flag | Purpose |
|---|---|
| `--fullscreen`, `--size WxH` | Window mode. **F11** toggles fullscreen in-game; **F12** saves a screenshot. |
| `--render-size WxH` | Render at a fixed internal resolution, scaled to the window. |
| `--play`, `--fresh`, `--chapter <1-6>`, `--start <room\|spawn-id>` | Skip the title / ignore the save / start in a chapter or room (development). |
| `--autoplay <script>` | Drive input from a script and take screenshots (automated play-testing). |
| `--user-dir <dir>` | Use a separate save directory. |
| `--export-achievements <file.csv>` | Write the achievement list for store back-ends and exit. |
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

## The campaign
| Chapter | Place | New ground | Champion | Stance |
|---|---|---|---|---|
| I | The Ashen Grove | grapple, wall-jumps, stealth in grass | Tetsu | Gale |
| II | The Drowned Terrace | swimming, lurkers, sluice puzzles, a flooding arena | Funaki, the Tide Warden | Stillwater |
| III | Crimson Maple Pass | rising winds, crumbling ledges, duelists, crow scouts | Karasu, the Crow King | Thunder |
| IV | Castle Hakuro | beacon light, hounds, gunners, an unseen crossing, a rescue | Lord Hakuro | Shadow |
| V | The Mirror Province | two layers of the world, stepped between with a dash | Kagami of the Hollow | Mirror |
| VI | The Heart of the Veil | the loom climb, a chase through the unravelling | Odaka Sen (three phases) | — |

* **Combat:** posture, deflects, perilous attacks, executions, stealth kills, shuriken, Ember Art and five stances;
  12 foe types plus six champions, with sight cones, alert propagation and attack tokens.
* **Progression:** 29 items (blades, garbs, masks, tokens, gems, materials) across six chapters; a 16-skill tree in four tiers; a forge;
  25 ink scrolls; 46 achievements; New Game+; chapter select with per-chapter records.
* **Story:** fully scripted in-engine cinematics, a mercy theme (spare or finish), and three endings (Reweave,
  Unravel, Restitch — the last needs all 25 scrolls).
* **Presentation:** a colour script per room, procedural parallax worlds per chapter, weather (rain, snow, threads),
  HDR bloom and grading, and a score of voice, breath and wind (no instruments).
* **Accessibility:** shake scale, flash reduction, deflect-window assist, game speed 60–100 %, hold/toggle guard,
  subtitle size, high-contrast foes, ink/petal effects, hint toggle.

## Testing
```sh
cmake --build build && ./build/sv_tests      # or: ctest --test-dir build
```
The suite drives the real `World` simulation headless and has 1,620 checks, including:
* every chapter's level and script: loads, links (scenes, NPCs, labels), scroll counts, boss and phase scenes
* save, settings and profile round-trips; chapter replay resets
* movement arcs, wall-jump climb, combat, stealth, puzzles, trials, the multi-deathblow bosses
* finite audio synthesis

Level generators in `tools/levels/chN.py` also run a reachability search over every room (`levelkit.py`) and
refuse to write a level with an unreachable exit, chest, scroll or bead.

Visual verification uses `--autoplay` scripts with `shot` commands (see `src/main.cpp`).

Measured on an AMD Renoir iGPU at 1920×1080 with 4× MSAA: about **5 ms GPU** and **1.5 ms CPU** per frame.

## Code map
| Path | Contents |
|---|---|
| `src/core/` | math, RNG/noise, UTF-8 file paths (XDG / %APPDATA%), log, controller-first input |
| `src/render/` | GL 3.3 loader, batched renderer, sky shader, bloom/grade post chain, FreeType text, PNG capture |
| `src/audio/` | synthesized SFX bank, generative score of formant-synthesized voice, breath and wind, rain/water ambience, reverb |
| `src/game/` | skeleton/animation, level loader, player, enemies, world, style & trials, cutscenes, progression, game flow & UI |
| `src/platform/` | store services (achievements); a no-op default and an optional Steamworks backend |
| `src/ui/` | device-aware button glyphs and rich text |
| `data/` | `levels/*.lvl` (room-based ASCII levels, generated by `tools/levels/chN.py`), `scripts/*.cut` (cutscene language) |
| `site/`, `steam/`, `packaging/` | website, Steam build scripts and store copy, desktop entry / Windows resources |
| `assets/fonts/` | Noto Serif + a subset of Noto Serif CJK (OFL). Regenerate with `tools/subset_fonts.sh`. |

## Contributing
Bug reports, small fixes and level or documentation contributions are welcome. Read
[`CONTRIBUTING.md`](CONTRIBUTING.md) first; the short version is to build it, run `ctest`, and describe how the
change was tested. Security issues should go through [`SECURITY.md`](SECURITY.md), not the public tracker.

## License
The game and its source are released under the MIT license; see [`LICENSE`](LICENSE). The bundled Noto Serif fonts
are under the SIL Open Font License, in [`assets/fonts/OFL.txt`](assets/fonts/OFL.txt).
