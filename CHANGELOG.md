# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.9.0] - 2026-11-09

Release candidate: the full campaign, a Windows version and downloadable packages.

### Added

- Chapters II–VI: *The Drowned Terrace*, *Crimson Maple Pass*, *Castle Hakuro*, *The Mirror Province* and
  *The Heart of the Veil*, each with its own rooms, palette, background world, cinematics, trial and champion.
- New mechanics: swimming, sluices and flooding, rising winds, crumbling ledges, beacon light, two-layer rooms
  stepped between with a dash, and a chase through the unravelling Veil.
- Nine new foe types and five new champions: Funaki, Karasu, Lord Hakuro, Kagami and Odaka Sen (three phases).
- Stillwater, Thunder, Shadow and Mirror stances; four tier-3 skills; twelve new items.
- 20 new ink scrolls (25 in all), 25 new achievements (46 in all), the mercy theme (spare the champions) and
  three endings chosen at the heart of the Veil; Restitch requires all 25 scrolls.
- Chapter select, per-chapter grades, times and records, an end-of-chapter card, and credits.
- Windows support: a runtime OpenGL loader, UTF-8-safe paths, saves under `%APPDATA%\XSO\Shadow Veil`,
  an application icon and version resource, and a console-free release executable.
- Release packaging for Windows (zip) and Linux (portable tarball, optional AppImage), with pinned,
  statically linked SDL3, FreeType and zlib; Windows MSVC CI; a tag-triggered release workflow.
- A log file next to the saves, and clear error dialogs when the graphics driver lacks OpenGL 3.3 or the game
  data is missing.
- The website (`site/`), Steam build scripts, store copy and achievement export (`steam/`), and an optional
  Steamworks achievements backend.
- `tools/levels/`: Python level generators with a reachability check for every room.
- `--chapter` and `--export-achievements` command-line options.
- The test suite grows from 360 to 1,620 checks, validating every chapter's level and script.

### Changed

- The score is voice, breath and wind only; no instruments.
- Content review: removed religious and superstition references (shrines, bells, prayers, zodiac years,
  talismans) from all player-facing text and sounds. Camps replace shrines; the bell puzzle is now a
  counterweight catch.
- The Bright side of the Mirror Province is darkened so terrain reads against the sky.

### Fixed

- Persistent gate and flood flags are chapter-prefixed, so replaying a chapter resets its puzzles.
- The chase hit-tracking is reset when a scripted chase begins.

## [0.1.0] - 2026-10-30

First public release: Chapter I, *The Ashen Grove*, playable from start to finish.

### Added

- Ten handcrafted rooms with authored set pieces, three secret areas, four camps, two puzzles and a
  wind trial.
- Posture-based combat: deflects, perilous attacks, executions, stealth kills, shuriken and Ember Arts.
- Three enemy types with sight cones, alert propagation and attack tokens, plus a two-phase boss.
- Traversal: variable jump with coyote time and input buffering, wall-slide and wall-jump, air-dash and a
  pendulum grapple.
- Progression: two blades, three garbs, two masks, four tokens, three gems, a twelve-skill tree, a forge
  with six recipes, resin beads, five lore scrolls, twenty-one achievements and New Game+.
- Kage Rank scoring, a chapter grade and local personal-best records, including a trial time record.
- Controller-first input with device-aware button glyphs, light bar and haptics; a complete keyboard and
  mouse fallback.
- Procedurally rendered presentation: colour-scripted rooms, parallax worlds, rim-lit silhouettes, verlet
  cloth, HDR bloom and grading, and letterboxed in-engine cinematics.
- A synthesized sound bank and a generative score that reacts to combat.
- Accessibility options: shake scale, flash reduction, deflect-window assist, game speed, hold/toggle
  guard, subtitle size, high-contrast foes and a hint toggle.
- A headless test suite of 360 checks covering levels, saves, progression, movement, combat, puzzles and
  audio synthesis.
- A deterministic autoplay harness for screenshots and automated play-testing.

[Unreleased]: https://github.com/AbdulrahmanXSO25/shadow-veil/compare/v0.9.0...HEAD
[0.9.0]: https://github.com/AbdulrahmanXSO25/shadow-veil/compare/v0.1.0...v0.9.0
[0.1.0]: https://github.com/AbdulrahmanXSO25/shadow-veil/releases/tag/v0.1.0
