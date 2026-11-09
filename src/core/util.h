#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <cstdio>

// printf-style format checking where the compiler supports it (GCC/Clang; MinGW uses C99 stdio).
#if defined(__MINGW32__)
#define SV_PRINTF(f, a) __attribute__((format(gnu_printf, f, a)))
#elif defined(__GNUC__) || defined(__clang__)
#define SV_PRINTF(f, a) __attribute__((format(printf, f, a)))
#else
#define SV_PRINTF(f, a)
#endif

namespace sv {

void logInfo(const char* fmt, ...) SV_PRINTF(1, 2);
void logWarn(const char* fmt, ...) SV_PRINTF(1, 2);
void logError(const char* fmt, ...) SV_PRINTF(1, 2);

std::optional<std::string> readFile(const std::string& path);
std::filesystem::path u8path(const std::string& utf8);       // UTF-8 string -> path, on every platform
std::string pathString(const std::filesystem::path& p);      // path -> UTF-8 string
// Writes atomically (temp file + rename) so a crash never corrupts a save.
bool writeFileAtomic(const std::string& path, const std::string& contents);

// Root of read-only content (contains data/ and assets/).
const std::string& contentRoot();
std::string contentPath(const std::string& rel);
// Writable per-user directory ($XDG_DATA_HOME/shadow-veil), created on first use.
const std::string& userDir();
void setUserDirOverride(const std::string& dir);

std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char delim);
// Splits on whitespace but keeps "quoted strings" together (quotes removed).
std::vector<std::string> tokenize(const std::string& line);
bool startsWith(const std::string& s, const std::string& p);
std::string strfmt(const char* fmt, ...) SV_PRINTF(1, 2);

} // namespace sv
