#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

bool parseIpv4(const std::string& s, uint32_t& ip);
std::string ipToString(uint32_t ip);

// Binary trie for IPv4 longest-prefix match
class LpmTable {
public:
    LpmTable();
    bool insertCidr(const std::string& cidr, const std::string& nextHop);
    std::optional<std::string> lookup(uint32_t ip, int* matchedLen = nullptr) const;

private:
    struct Node {
        std::unique_ptr<Node> child[2];
        bool has = false;
        int len = 0;
        std::string nh;
    };
    std::unique_ptr<Node> root_;
};