# Publishing Shadow Veil on Steam

Everything here is prepared in the repo. The steps marked **(you)** need your Steamworks partner
account, so they cannot be automated from here.

## 1. Account and app (you)

1. Join Steamworks at partner.steamgames.com and pay the Steam Direct fee (US$100 per app, recouped after
   US$1,000 in sales). Complete the tax and bank interviews; identity verification takes a few days.
2. Create the app. Note the **App ID** and create two depots: **Windows** and **Linux** (SteamOS + Linux).
3. Put the IDs into `steam/scripts/app_build.vdf`, `depot_windows.vdf` and `depot_linux.vdf`
   (replace `APPID_HERE`, `WINDOWS_DEPOTID_HERE`, `LINUX_DEPOTID_HERE`).
4. Steamworks → *Installation → General*: launch options
   - Windows: `ShadowVeil.exe`
   - Linux: `shadow_veil`
5. Valve requires **at least 2 weeks** between the store page going public ("Coming Soon") and release,
   and reviews both the store page and the build (allow 3–5 business days each).

## 2. Achievements

The game has 46 achievements. `steam/achievements.csv` is the list (regenerate with
`shadow_veil --export-achievements steam/achievements.csv`). In Steamworks → *Stats & Achievements*, create
each with the **API name exactly as in the CSV** (`api_name`), plus display name, description and hidden flag.
Each needs two 256×256 icons; `python3 steam/make_achievement_icons.py` renders a matching set into
`steam/achievement_icons/` (achieved = red seal, locked = grey).

Build with Steam enabled so unlocks reach Steam (they are also re-sent at every launch, so anything earned
before the Steam build is kept):

```bash
cmake -S . -B build-steam -G Ninja -DCMAKE_BUILD_TYPE=Release -DSV_RELEASE=ON \
      -DSV_STEAM=ON -DSTEAMWORKS_SDK=$HOME/steamworks_sdk/sdk -DSV_STEAM_APPID=<app id>
```

`src/platform/platform_steam.cpp` is written against the Steamworks API (`SteamAPI_Init`, `SetAchievement`,
`StoreStats`) but **has not been compiled or run yet**: the SDK is not in this repo. Build it once with the
SDK and test with the Steam client open and a `steam_appid.txt` containing the App ID next to the executable.

## 3. Uploading a build

```bash
tools/package/build-windows.sh         # → dist/ShadowVeil-<v>-windows-x64.zip
tools/package/build-linux.sh           # → dist/ShadowVeil-<v>-linux-x64.tar.gz
STEAM_USER=<builder account> steam/upload.sh dist/ShadowVeil-*-windows-x64.zip dist/ShadowVeil-*-linux-x64.tar.gz
```

Then set the build live on a branch (`beta` first) in Steamworks → *Builds*.
Steam Deck: the game runs natively on Linux and supports gamepads; request a Deck compatibility review after
release. The default text size is readable at 1280×800.

## 4. Store page

**Name:** Shadow Veil

**Short description (≤ 300 characters):**
> A hand-crafted 2D action platformer. Deflect, dash and slip through the dark across six chapters, learn five
> sword stances, spare the champions who stand in your way, and decide what becomes of the Veil.

**About this game:**
> Six hundred years ago, the Kagenui stitched a veil across the sky and sealed a country of exiles behind it.
> Now the weave is wearing thin.
>
> Rei, the last student of an old swordsman, follows her master down from the mountains — through a burning
> bamboo grove, drowned rice terraces, a crimson maple pass, a castle in the first snow and a province where
> the world doubles in the glass — to the loom at the heart of the Veil.
>
> **Read every strike.** Deflect at the last instant to break an opponent's posture, then end the fight with a
> single Execution. Perilous attacks must be jumped or dashed through.
>
> **Five stances.** Gale, Stillwater, Thunder, Shadow and Mirror, each earned in a trial, each changing how you
> fight, hide or move.
>
> **Move like a shinobi.** Wall-jumps, dashes, grappling rings, swimming, rising winds, and stepping between the
> Bright and Hollow sides of the world.
>
> **Mercy is a choice.** Every champion after the first can be spared. Gather all 25 ink scrolls to unlock the
> third ending.
>
> - Six chapters, each replayable for a better grade
> - Six champion fights and three endings
> - 46 achievements
> - Built for controllers (PlayStation and Xbox prompts), full keyboard and mouse support
> - No instruments in the score: wind, rain, water and voice

**Tags:** Action, Platformer, 2D, Stealth, Swordplay, Ninja, Story Rich, Atmospheric, Singleplayer,
Controller.

**Content survey:** Fantasy violence with swords (no blood or gore option needed: hits are shown as ink
splashes). No sexual content, no gambling or purchasable randomness, no alcohol or drug use, no religious
content.

**System requirements**

| | Minimum | Recommended |
|---|---|---|
| OS | Windows 10 64-bit / Ubuntu 22.04 or equivalent | Windows 11 / any current distro |
| Processor | Dual-core 2 GHz | Quad-core |
| Memory | 2 GB RAM | 4 GB RAM |
| Graphics | OpenGL 3.3 (Intel HD 4000 or better) | Any dedicated GPU from the last 8 years |
| Storage | 50 MB | 50 MB |

## 5. Store art (you)

Steam requires these exact sizes. Screenshots in `site/img/` are 1280×720; capture new ones at 1920×1080
with `shadow_veil --render-size 1920x1080` and an autoplay script (see `tools/trailer/`).

| Asset | Size | Notes |
|---|---|---|
| Header capsule | 920×430 | Logo + key art |
| Small capsule | 462×174 | Logo must be legible |
| Main capsule | 1232×706 | |
| Vertical capsule | 748×896 | |
| Page background | 1438×810 | Optional |
| Library capsule | 600×900 | |
| Library hero | 3840×1240 | No logo |
| Library logo | 1280×720 max, transparent PNG | |
| Screenshots | ≥ 5 at 1920×1080 | In-game only, no UI mock-ups |
| Trailer | 1080p MP4 | `tools/trailer/make_trailer.py` |
| Client icon | 32×32 ICO / 184×184 JPG | `assets/icon/` |

## 6. Release checklist

- [ ] `ctest` green on Linux and Windows CI; `tools/package/build-windows.sh` run with `SV_WINE_TESTS=1`
- [ ] Full playthrough of all six chapters on Windows and Linux (Wayland and X11), controller and keyboard
- [ ] Each ending reached once (ending 3 needs all 25 scrolls)
- [ ] Every achievement unlocks in the Steam overlay (Steam build)
- [ ] Save in `%APPDATA%\XSO\Shadow Veil` survives an update; Steam Cloud set up for `save*.dat`,
      `profile.dat` and `settings.dat` (Steamworks → *Steam Cloud*, root `WinAppDataRoaming` →
      `XSO/Shadow Veil`, Linux root `LinuxXdgDataHome` → `shadow-veil`)
- [ ] Version bumped in `CMakeLists.txt`, `CHANGELOG.md` updated, git tag `v<version>` pushed (builds the
      GitHub Release that the website links to)
- [ ] Store page reviewed and set to Coming Soon at least 2 weeks before launch
- [ ] Build reviewed by Valve, set live on `default`
