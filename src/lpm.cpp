#include "lpm.hpp"
#include <cstdio>

bool parseIpv4(const std::string& s, uint32_t& ip) {
    unsigned a, b, c, d;
    char extra;
    if (std::sscanf(s.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) != 4) return false;
    if (a > 255 || b > 255 || c > 255 || d > 255) return false;
    ip = (a << 24) | (b << 16) | (c << 8) | d;
    return true;
}

std::string ipToString(uint32_t ip) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%u.%u.%u.%u",
                  (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
    return buf;
}

LpmTable::LpmTable() : root_(std::make_unique<Node>()) {}

bool LpmTable::insertCidr(const std::string& cidr, const std::string& nh) {
    auto slash = cidr.find('/');
    if (slash == std::string::npos) return false;
    uint32_t ip;
    if (!parseIpv4(cidr.substr(0, slash), ip)) return false;
    int len;
    try { len = std::stoi(cidr.substr(slash + 1)); } catch (...) { return false; }
    if (len < 0 || len > 32) return false;

    Node* n = root_.get();
    for (int i = 0; i < len; ++i) {
        int bit = (ip >> (31 - i)) & 1;
        if (!n->child[bit]) n->child[bit] = std::make_unique<Node>();
        n = n->child[bit].get();
    }
    n->has = true;
    n->len = len;
    n->nh = nh;
    return true;
}

std::optional<std::string> LpmTable::lookup(uint32_t ip, int* matchedLen) const {
    const Node* n = root_.get();
    const Node* best = n->has ? n : nullptr;
    for (int i = 31; i >= 0 && n; --i) {
        int bit = (ip >> i) & 1;
        n = n->child[bit].get();
        if (n && n->has) best = n;
    }
    if (!best) return std::nullopt;
    if (matchedLen) *matchedLen = best->len;
    return best->nh;
}