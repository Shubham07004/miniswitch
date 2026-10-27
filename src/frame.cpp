#include "frame.hpp"
#include <cstdio>

std::string macToString(const Mac& m) {
    char buf[18];
    std::snprintf(buf, sizeof buf, "%02x:%02x:%02x:%02x:%02x:%02x",
                  m[0], m[1], m[2], m[3], m[4], m[5]);
    return buf;
}

bool parseMac(const std::string& s, Mac& m) {
    unsigned v[6];
    char extra;
    if (std::sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x%c",
                    &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &extra) != 6)
        return false;
    for (int i = 0; i < 6; ++i) {
        if (v[i] > 255) return false;
        m[i] = static_cast<uint8_t>(v[i]);
    }
    return true;
}

std::optional<ParsedFrame> parseFrame(const std::vector<uint8_t>& f) {
    if (f.size() < 14) return std::nullopt;
    ParsedFrame p;
    for (int i = 0; i < 6; ++i) {
        p.dst[i] = f[i];
        p.src[i] = f[6 + i];
    }
    uint16_t et = static_cast<uint16_t>((f[12] << 8) | f[13]);
    if (et == 0x8100) {
        if (f.size() < 18) return std::nullopt;
        uint16_t tci = static_cast<uint16_t>((f[14] << 8) | f[15]);
        p.tagged = true;
        p.vlan = tci & 0x0FFF;
        p.ethertype = static_cast<uint16_t>((f[16] << 8) | f[17]);
        p.payloadOffset = 18;
    } else {
        p.ethertype = et;
        p.payloadOffset = 14;
    }
    return p;
}

std::vector<uint8_t> buildFrame(const Mac& dst, const Mac& src, uint16_t vlan,
                                uint16_t ethertype, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> f;
    f.reserve(64 + payload.size());
    f.insert(f.end(), dst.begin(), dst.end());
    f.insert(f.end(), src.begin(), src.end());
    if (vlan != 0) {
        f.push_back(0x81);
        f.push_back(0x00);
        f.push_back(static_cast<uint8_t>((vlan >> 8) & 0x0F));
        f.push_back(static_cast<uint8_t>(vlan & 0xFF));
    }
    f.push_back(static_cast<uint8_t>(ethertype >> 8));
    f.push_back(static_cast<uint8_t>(ethertype & 0xFF));
    f.insert(f.end(), payload.begin(), payload.end());
    while (f.size() < 60) f.push_back(0);  // minimum Ethernet frame
    return f;
}