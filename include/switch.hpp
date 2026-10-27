#pragma once
#include "frame.hpp"
#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>

enum class PortMode { Access, Trunk };

struct Port {
    int id = 0;
    PortMode mode = PortMode::Access;
    uint16_t accessVlan = 1;
    std::set<uint16_t> allowed;  // trunk VLANs
    uint64_t rx = 0, tx = 0, drops = 0;
};

struct Output {
    int port;
    std::vector<uint8_t> frame;
};

struct MacEntry {
    int port = 0;
    uint64_t lastSeen = 0;
};

class Switch {
public:
    void setAging(uint64_t sec) { aging_ = sec; }
    void addAccessPort(int id, uint16_t vlan);
    void addTrunkPort(int id, const std::set<uint16_t>& vlans);

    // Core data-plane function: one frame in, zero or more frames out
    std::vector<Output> process(int inPort, const std::vector<uint8_t>& frame, uint64_t now);

    size_t ageOut(uint64_t now);
    bool lookup(uint16_t vlan, const Mac& m, int& port) const;
    void printMacTable() const;
    void printStats() const;

private:
    bool inVlan(const Port& p, uint16_t vlan) const;

    std::map<int, Port> ports_;
    std::map<std::pair<uint16_t, Mac>, MacEntry> table_;  // key: (vlan, mac)
    uint64_t aging_ = 300;
    uint64_t floods_ = 0, forwarded_ = 0, filtered_ = 0;
};