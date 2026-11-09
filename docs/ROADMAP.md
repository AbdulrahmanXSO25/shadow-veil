# SHADOW VEIL — Roadmap

This document separates what is **built and tested** from what is **not yet done or not yet verified**.

## 1. Built (v0.9.0, release candidate)

| Area | Status |
|---|---|
| Campaign | Six chapters, 42 rooms, six champions, three endings, credits, chapter select |
| Engine | SDL3 / GL 3.3 renderer, HDR + bloom + grading, native Wayland and X11, Windows, fixed-step 120 Hz sim, autoplay harness |
| Controls | Controller-first (PlayStation / Xbox glyphs, light bar, haptics, analog stealth, right-stick aim); keyboard and mouse |
| Content | 12 foe types, 6 champions, 5 stances, 29 items, 16 skills, 25 scrolls, 46 achievements, NG+ |
| Platforms | Linux x64 (portable tarball), Windows x64 (zip); website on GitHub Pages; Steam scripts and store copy |
| Tests | 1,620 headless checks; level reachability search; every boss fought to its outro by an autoplay bot |

## 2. How it was verified

- `sv_tests` passes on Linux (GCC), under Wine (MinGW build), and on a Windows 11 VM.
- Each chapter was toured room by room with screenshots, and each champion was fought by the bot
  (invulnerable) through every phase to the spare scene and the chapter-end card. The final chapter reaches the
  ending choice and records the ending.
- The Windows build starts on the Windows 11 VM, creates its window and loads OpenGL. The VM has no 3D GPU
  (GDI Generic, OpenGL 1.1), so the game shows its "needs OpenGL 3.3" dialog there; **it has not yet been seen
  rendering on a real Windows PC.**

## 3. Not yet done or not yet verified

- **Human playtests.** Difficulty and pacing of Chapters II–VI were checked by bot and screenshots only. Expect
  tuning after real players (boss health, trial timers, chase speed).
- **Windows on real hardware** (see above), and the **MSVC** CI job, which is written but has not run yet.
- **Steam:** the Steamworks backend (`src/platform/platform_steam.cpp`) has not been compiled against the SDK;
  App ID, depots and store art need a partner account. See `steam/README.md`.
- **Physical controller** testing on this machine (SDL3 gamepad API, unit-tested via injected input).
- **Button remapping** in-game.
- **Interiors:** castle and loom interiors reuse outdoor backgrounds (sky and weather are visible indoors).
- **Localization:** English only. The font pipeline already handles CJK and could take Arabic with a shaping pass.

## 4. Pipeline for new content

1. Add a palette (`src/game/level.cpp`) and background style.
2. Write a generator in `tools/levels/chN.py` with `levelkit` and run it; it refuses unreachable rooms.
3. Write the cinematics in `data/scripts/chN.cut` and register the chapter in `chapterDefs()`.
4. Add foes (`enemyDef` in `src/game/enemy.cpp`) and their look in `render_world.cpp`.
5. Run `tools/subset_fonts.sh` for new kanji, then `ctest`, then an autoplay screenshot tour.
