#include "core/util.h"
#include <SDL3/SDL.h>
#include <cstdarg>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace sv {

static void vlog(const char* tag, const char* fmt, va_list ap) {
    std::fprintf(stderr, "[%s] ", tag);
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
}
void logInfo(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog("info", fmt, ap); va_end(ap); }
void logWarn(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog("warn", fmt, ap); va_end(ap); }
void logError(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog("ERROR", fmt, ap); va_end(ap); }

std::string strfmt(const char* fmt, ...) {
    char buf[1024];
    va_list ap; va_start(ap, fmt);
    int n = std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < (int)sizeof buf) return std::string(buf, n > 0 ? n : 0);
    std::string out(n + 1, '\0');
    va_start(ap, fmt); std::vsnprintf(out.data(), n + 1, fmt, ap); va_end(ap);
    out.resize(n);
    return out;
}

std::optional<std::string> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

bool writeFileAtomic(const std::string& path, const std::string& contents) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << contents;
        if (!f.good()) return false;
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    return !ec;
}

static std::string findContentRoot() {
    std::vector<fs::path> candidates;
    if (const char* env = std::getenv("SHADOW_VEIL_DATA")) candidates.emplace_back(env);
    if (const char* base = SDL_GetBasePath()) {
        fs::path b(base);
        candidates.push_back(b);
        candidates.push_back(b / "..");
        candidates.push_back(b / ".." / "share" / "shadow-veil");
    }
    candidates.emplace_back(SV_SOURCE_DIR);
    candidates.emplace_back(".");
    for (auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c / "data" / "levels", ec) && fs::exists(c / "assets" / "fonts", ec))
            return fs::weakly_canonical(c, ec).string();
    }
    logError("Could not locate game content (data/ and assets/). Set SHADOW_VEIL_DATA.");
    return ".";
}

const std::string& contentRoot() {
    static std::string root = findContentRoot();
    return root;
}
std::string contentPath(const std::string& rel) { return contentRoot() + "/" + rel; }

static std::string g_userOverride;
void setUserDirOverride(const std::string& dir) { g_userOverride = dir; }

const std::string& userDir() {
    static std::string dir;
    if (!g_userOverride.empty() && dir != g_userOverride) {
        dir = g_userOverride;
        std::error_code ec; fs::create_directories(dir, ec);
    }
    if (dir.empty()) {
        fs::path p;
        if (const char* x = std::getenv("XDG_DATA_HOME"); x && *x) p = fs::path(x) / "shadow-veil";
        else if (const char* h = std::getenv("HOME")) p = fs::path(h) / ".local" / "share" / "shadow-veil";
        else p = "save";
        std::error_code ec; fs::create_directories(p, ec);
        dir = p.string();
    }
    return dir;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out; std::string cur;
    for (char c : s) { if (c == delim) { out.push_back(cur); cur.clear(); } else cur += c; }
    out.push_back(cur);
    return out;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out; std::string cur; bool inQ = false, has = false;
    for (char c : line) {
        if (c == '"') { inQ = !inQ; has = true; continue; }
        if (!inQ && (c == ' ' || c == '\t')) { if (has) { out.push_back(cur); cur.clear(); has = false; } continue; }
        cur += c; has = true;
    }
    if (has) out.push_back(cur);
    return out;
}

bool startsWith(const std::string& s, const std::string& p) { return s.rfind(p, 0) == 0; }

} // namespace sv
