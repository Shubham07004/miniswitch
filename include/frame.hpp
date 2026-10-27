#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using Mac = std::array<uint8_t, 6>;

struct ParsedFrame {
    Mac dst{}, src{};
    bool tagged = false;
    uint16_t vlan = 0;       // valid only if tagged
    uint16_t ethertype = 0;
    size_t payloadOffset = 0;
};

std::string macToString(const Mac& m);
bool parseMac(const std::string& s, Mac& m);
std::optional<ParsedFrame> parseFrame(const std::vector<uint8_t>& f);
// vlan == 0 builds an untagged frame, otherwise an 802.1Q tagged frame
std::vector<uint8_t> buildFrame(const Mac& dst, const Mac& src, uint16_t vlan,
                                uint16_t ethertype, const std::vector<uint8_t>& payload);