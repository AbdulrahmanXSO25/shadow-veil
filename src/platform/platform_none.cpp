#ifndef SV_STEAM
#include "platform/platform.h"

namespace sv::platform {
bool init() { return true; }
void shutdown() {}
void tick() {}
const char* name() { return "none"; }
void unlockAchievement(const std::string&) {}
void syncAchievements(const std::set<std::string>&) {}
}  // namespace sv::platform
#endif
