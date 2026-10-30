# Contributing to SHADOW VEIL

Thanks for taking an interest. This is a solo hobby project, so the most useful contributions are focused:
a bug report with a reliable reproduction, a small fix, a level, or improved documentation.

## Before you start

- Open an issue before a large change — a new mechanic, a dependency, or a refactor. Agreeing on the
  direction first is cheaper than reworking a pull request.
- Small fixes, typo corrections and documentation changes do not need an issue.
- By contributing you agree that your work is released under the MIT license (see [`LICENSE`](LICENSE)).

## Building

The build instructions and dependencies are in the [README](README.md#build--run-arch-linux-wayland). The
short version, on Arch Linux:

```sh
sudo pacman -S --needed sdl3 freetype2 zlib mesa cmake ninja gcc
cmake -S . -B build -G Ninja
cmake --build build
```

Run the headless test suite before opening a pull request:

```sh
ctest --test-dir build --output-on-failure
```

## Style

- C++20. Match the surrounding code rather than a house style; the codebase is deliberately plain.
- Four spaces, 120 columns. A [`.clang-format`](.clang-format) is included but not enforced, so do not
  reformat files you are not otherwise changing.
- Keep the simulation in `src/game/` free of windowing and rendering calls where practical — the tests
  drive `World` headless and depend on that separation.
- No new third-party dependencies without discussion. The project builds against SDL3, OpenGL, FreeType
  and zlib, and nothing else.
- Gameplay constants belong in the level data or progression tables, not sprinkled through the code.

## Levels and cutscenes

Room geometry lives in [`data/levels/`](data/levels) and dialogue in [`data/scripts/`](data/scripts). The
level format is documented in [`data/levels/README.md`](data/levels/README.md). Level changes are best
submitted with a short description of the intended route and, for anything visual, a screenshot.

## Commits and pull requests

- One logical change per commit. Write the subject in the imperative mood (`Fix wall-jump buffer`, not
  `Fixed` or `Fixes`), and explain *why* in the body if it is not obvious.
- Rebase on `main` rather than merging it into your branch.
- Describe how the change was tested in the pull request. Visual changes need a screenshot or a short
  clip, since the presentation is a large part of the game.
- Keep the tests green. If a change alters behaviour covered by `tests/tests.cpp`, update or extend the
  test in the same pull request.

## Reporting bugs

Use the bug report form and include the version or commit, your distribution and session type
(Wayland or X11), your GPU and driver, and the steps that reproduce the problem. `--stats` prints frame
timings, F12 saves a screenshot, and a save file or room name helps pin down progression bugs.

Please do not report security issues in public — see [`SECURITY.md`](SECURITY.md).
