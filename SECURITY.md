# Security Policy

## Supported versions

SHADOW VEIL is a single-player desktop game. Only the latest release (currently 0.1.x) receives security
fixes.

## Reporting a vulnerability

Please report vulnerabilities privately through GitHub's
[private vulnerability reporting](https://github.com/AbdulrahmanXSO25/shadow-veil/security/advisories/new).
Do not open a public issue for an exploitable problem.

Include the version or commit, your distribution and session type, and the steps needed to reproduce the
issue. A proof-of-concept level, script or save file is welcome if the problem is in one of the parsers.

## Scope

The game reads level files (`data/levels/*.lvl`), cutscene scripts (`data/scripts/*.cut`), settings and
save data from the user's data directory, and loads content through SDL3, FreeType, zlib and OpenGL.
Memory-safety bugs in the parsers, path traversal, and writes outside the save directory are in scope.

Crashes caused by hand-edited local files are treated as ordinary bugs unless the content can reach the
game without the player authoring it. Gameplay bugs belong in the issue tracker.
