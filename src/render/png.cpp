#include "render/png.h"
#include <zlib.h>
#include <vector>
#include <fstream>

namespace sv {

static void put32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

static void chunk(std::ofstream& f, const char* type, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> buf;
    put32(buf, (uint32_t)data.size());
    buf.insert(buf.end(), type, type + 4);
    buf.insert(buf.end(), data.begin(), data.end());
    uint32_t crc = crc32(0, buf.data() + 4, (uInt)(buf.size() - 4));
    put32(buf, crc);
    f.write((const char*)buf.data(), buf.size());
}

bool writePNG(const std::string& path, int w, int h, const uint8_t* rgba) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    f.write((const char*)sig, 8);
    std::vector<uint8_t> ihdr;
    put32(ihdr, w); put32(ihdr, h);
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0});
    chunk(f, "IHDR", ihdr);
    std::vector<uint8_t> raw((size_t)(w * 4 + 1) * h);
    for (int y = 0; y < h; y++) {
        raw[y * (w * 4 + 1)] = 0;
        std::copy(rgba + (size_t)y * w * 4, rgba + (size_t)(y + 1) * w * 4, raw.begin() + y * (w * 4 + 1) + 1);
    }
    uLongf len = compressBound(raw.size());
    std::vector<uint8_t> z(len);
    if (compress2(z.data(), &len, raw.data(), raw.size(), 6) != Z_OK) return false;
    z.resize(len);
    chunk(f, "IDAT", z);
    chunk(f, "IEND", {});
    return f.good();
}

} // namespace sv
