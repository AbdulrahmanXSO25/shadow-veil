#pragma once
// Store/platform services (achievements today). The default build has no store backend; configure
// with -DSV_STEAM=ON -DSTEAMWORKS_SDK=<path> to route achievements to Steam (see steam/README.md).
#include <set>
#include <string>

namespace sv::platform {

bool init();                                            // safe to call when no store is present
void shutdown();
void tick();                                            // once per frame (pumps store callbacks)
const char* name();                                     // "none" or "steam"
void unlockAchievement(const std::string& id);          // ids match achievementDefs()
void syncAchievements(const std::set<std::string>& ids);   // re-sends everything earned offline

}  // namespace sv::platform
