#include "core/util.h"
#include <SDL3/SDL.h>
#include <cstdarg>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace sv {

// UTF-8 strings <-> filesystem paths. On Windows a plain std::string path is read in the ANSI
// code page, which breaks for profile folders with non-Latin names; going through u8string is
// correct everywhere.
fs::path u8path(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
std::string pathString(const fs::path& p) { auto u = p.u8string(); return std::string(u.begin(), u.end()); }

static std::FILE* logFile() {
    // Release builds on Windows have no console, so everything is also written next to the saves.
    static std::FILE* f = [] {
        std::FILE* out = nullptr;
#ifdef _WIN32
        _wfopen_s(&out, (u8path(userDir()) / "shadow-veil.log").wstring().c_str(), L"w");
#else
        out = std::fopen(pathString(u8path(userDir()) / "shadow-veil.log").c_str(), "w");
#endif
        return out;
    }();
    return f;
}

static void vlog(const char* tag, const char* fmt, va_list ap) {
    va_list ap2; va_copy(ap2, ap);
    std::fprintf(stderr, "[%s] ", tag);
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
    if (std::FILE* f = logFile()) {
        std::fprintf(f, "[%s] ", tag);
        std::vfprintf(f, fmt, ap2);
        std::fputc('\n', f);
        std::fflush(f);
    }
    va_end(ap2);
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
    std::ifstream f(u8path(path), std::ios::binary);
    if (!f) return std::nullopt;
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

bool writeFileAtomic(const std::string& path, const std::string& contents) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream f(u8path(tmp), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << contents;
        if (!f.good()) return false;
    }
    std::error_code ec;
    fs::rename(u8path(tmp), u8path(path), ec);
    if (ec) {
        // some platforms refuse to rename over an existing file
        ec.clear();
        fs::remove(u8path(path), ec);
        ec.clear();
        fs::rename(u8path(tmp), u8path(path), ec);
    }
    return !ec;
}

static std::string findContentRoot() {
    std::vector<fs::path> candidates;
    if (const char* env = std::getenv("SHADOW_VEIL_DATA")) candidates.emplace_back(u8path(env));
    if (const char* base = SDL_GetBasePath()) {
        fs::path b = u8path(base);
        candidates.push_back(b);
        candidates.push_back(b / "..");
        candidates.push_back(b / ".." / "share" / "shadow-veil");
    }
#ifndef SV_RELEASE
    candidates.emplace_back(u8path(SV_SOURCE_DIR));   // developer builds run straight from the checkout
#endif
    candidates.emplace_back(".");
    for (auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c / "data" / "levels", ec) && fs::exists(c / "assets" / "fonts", ec))
            return pathString(fs::weakly_canonical(c, ec));
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
        std::error_code ec; fs::create_directories(u8path(dir), ec);
    }
    if (dir.empty()) {
        fs::path p;
#if defined(_WIN32) || defined(__APPLE__)
        // %APPDATA%/XSO/Shadow Veil on Windows
        if (char* pref = SDL_GetPrefPath("XSO", "Shadow Veil")) { p = u8path(pref); SDL_free(pref); }
        else p = "save";
#else
        if (const char* x = std::getenv("XDG_DATA_HOME"); x && *x) p = u8path(x) / "shadow-veil";
        else if (const char* h = std::getenv("HOME")) p = u8path(h) / ".local" / "share" / "shadow-veil";
        else p = "save";
#endif
        std::error_code ec; fs::create_directories(p, ec);
        dir = pathString(p);
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
