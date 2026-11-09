#ifdef SV_STEAM
// Steamworks backend. Built only with -DSV_STEAM=ON; needs the Steamworks SDK (not redistributed
// here) and steam_appid.txt next to the executable during development.
#include "platform/platform.h"
#include "core/util.h"
#include <steam/steam_api.h>

namespace sv::platform {

static bool g_ok = false;

bool init() {
    // If the game was started outside Steam, Steam relaunches it and we quit.
    if (SteamAPI_RestartAppIfNecessary(SV_STEAM_APPID)) return false;
    g_ok = SteamAPI_Init();
    if (!g_ok) logWarn("Steam: SteamAPI_Init failed; achievements stay local");
    else logInfo("Steam: signed in as %s", SteamFriends() ? SteamFriends()->GetPersonaName() : "?");
    return true;
}

void shutdown() { if (g_ok) SteamAPI_Shutdown(); g_ok = false; }
void tick() { if (g_ok) SteamAPI_RunCallbacks(); }
const char* name() { return "steam"; }

void unlockAchievement(const std::string& id) {
    if (!g_ok || !SteamUserStats()) return;
    SteamUserStats()->SetAchievement(id.c_str());
    SteamUserStats()->StoreStats();
}

void syncAchievements(const std::set<std::string>& ids) {
    if (!g_ok || !SteamUserStats()) return;
    for (auto& id : ids) SteamUserStats()->SetAchievement(id.c_str());
    SteamUserStats()->StoreStats();
}

}  // namespace sv::platform
#endif
