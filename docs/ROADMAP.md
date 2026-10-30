# SHADOW VEIL — Roadmap

This document separates what is **built and tested** from what is **designed but not yet built**.

## 1. Built (vertical slice v0.1)
Chapter I, *The Ashen Grove*, is playable end to end. A first-time playthrough takes about 25–40 minutes. Full
completion (all scrolls and secrets, the wind trial, S grade, SS Kage Rank, NG+) takes several hours.

| Area | Status |
|---|---|
| Engine | SDL3/GL 3.3 renderer, HDR + bloom + grading, native Wayland, fixed-step 120 Hz sim with interpolation, autoplay test harness |
| Controls | Controller-first (PlayStation/Xbox detection, drawn glyphs, light bar, haptics, analog stealth, right-stick aim & peek); keyboard fallback |
| Combat | Posture, deflect, perilous, execution, stealth kill, shuriken, Ember Art, Gale wind blades, Kage Rank |
| Enemies | Swordsman, Spearman, Archer; Tetsu (2 deathblows, flame phase) |
| Content | 10 rooms, 4 camps, 2 puzzles, 3 secret areas, 1 wind trial, 9 cinematics, 5 lore scrolls |
| Progression | Equipment & gems, 12-skill / 4-branch tree, forge, resin beads, 21 achievements, NG+, personal-best records |
| Tests | 360 headless checks (`sv_tests`) plus screenshot autoplay scripts |

## 2. The full campaign (designed, not built)
The target is a 20–30 hour first playthrough, and **60+ hours** for full mastery (all S+ grades, every trial,
NG+3, all endings). That is the "days of play for a dedicated player" goal. It requires the four remaining
chapters below. Each chapter is about the size of Chapter I ×2–3, plus a hub.

| Chapter | New mechanics | New foes | Boss | Stance / Art |
|---|---|---|---|---|
| II · The Drowned Terrace | Water volumes, rising tide puzzles, swimming stealth | Marsh reavers, tide lurkers | The Tide Warden (grabs, ink-water phase) | Frost Stance (deflects freeze posture) |
| III · Crimson Maple Pass | Updrafts, crow-carried grapple points, vertical chase | Ridge duelists, crow swarms | The Crow King (aerial duel) | Thunder Art (chain strike) |
| IV · Castle Hakuro | Full infiltration: disguises, alarm bells, lights to cut | Samurai elite, hounds, gunners | Lord Hakuro (multi-stage duel) | Shadow Stance (brief invisibility) |
| V · The Mirror Province | Inverted world, Veil-walking between layers | The Hollow Company | Odaka Sen (3 phases) + the Weaver | Choice-dependent final art |

Also planned for the full game:
- **Three endings:** Reweave, Unravel, or Restitch. Restitch requires all 25 scrolls. Each ending branches dialogue in Chapter V.
- **Relationships:** Fuki, Hana (rescued in II), and the Crow King. Side quests change late-game allies and the ending epilogue.
- **Hub:** a rebuilt Kurosaki between chapters, with blacksmith, tea house (lore), training dojo (combo trials with ranks), and a trial board.
- **Mastery content:**
  - Gauntlet trials at each wind beacon, with S/A/B/C times
  - Boss rush, an "Iron Path" one-life mode, NG+1…+3 with remixed placements and new enemy combos
- **Skill ladder expansion:** five branches × 5 tiers (25 arts), plus stance evolutions (Gale → Tempest) earned through mastery trials.
- **Competitive:** an optional online leaderboard for chapter scores and trial times, and daily seeded trial challenges. Today, scores are local personal bests only.

## 3. Production notes and known limits
- **No online features yet.** Records are stored locally (`profile.dat`).
- **Controller testing.** Controller support is implemented against SDL3's gamepad API and unit-tested via injected input. It has not yet been hands-on tested with a physical PS4 pad on this machine.
- **Button remapping.** Players cannot rebind buttons yet; the bindings table lives in `src/core/input.cpp`.
- **Assets.** All art and audio are procedural by design. A future pass could add hand-painted backdrop plates and recorded foley; the renderer and audio mixer already support textured quads and sample playback.
- **Pacing.** Chapter I was tuned through automated play and screenshot review. Human playtests are the next step for difficulty and pacing.

## 4. Pipeline for new chapters
1. Add a palette (`src/game/level.cpp`) and background style for the world.
2. Author rooms in `data/levels/<chapter>.lvl` (see `data/levels/README.md`); validate with `sv_tests`.
3. Write the chapter's cinematics in `data/scripts/<chapter>.cut`.
4. Add enemy definitions (`enemyDef` in `src/game/enemy.cpp`) and gear drawing in `drawEnemy`.
5. Add any new kanji to the game. Run `tools/subset_fonts.sh`, which collects used glyphs automatically.
6. Add autoplay screenshot scripts and headless tests for each new mechanic.
