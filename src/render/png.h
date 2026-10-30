#pragma once
#include <string>
#include <cstdint>

namespace sv {
// Writes 8-bit RGBA pixels (top row first) as a PNG.
bool writePNG(const std::string& path, int w, int h, const uint8_t* rgba);
}
