#include "switch.hpp"
#include <iostream>

void Switch::addAccessPort(int id, uint16_t vlan) {
    Port p;
    p.id = id;
    p.mode = PortMode::Access;
    p.accessVlan = vlan;
    ports_[id] = p;
}

void Switch::addTrunkPort(int id, const std::set<uint16_t>& vlans) {
    Port p;
    p.id = id;
    p.mode = PortMode::Trunk;
    p.allowed = vlans;
    ports_[id] = p;
}

bool Switch::inVlan(const Port& p, uint16_t vlan) const {
    return p.mode == PortMode::Access ? p.accessVlan == vlan : p.allowed.count(vlan) > 0;
}

bool Switch::lookup(uint16_t vlan, const Mac& m, int& port) const {
    auto it = table_.find({vlan, m});
    if (it == table_.end()) return false;
    port = it->second.port;
    return true;
}

std::vector<Output> Switch::process(int inPort, const std::vector<uint8_t>& frame, uint64_t now) {
    std::vector<Output> out;
    auto pit = ports_.find(inPort);
    if (pit == ports_.end()) return out;
    Port& in = pit->second;
    in.rx++;

    auto pf = parseFrame(frame);
    if (!pf) { in.drops++; return out; }

    // 1. Ingress VLAN classification
    uint16_t vlan;
    if (in.mode == PortMode::Access) {
        if (pf->tagged) { in.drops++; return out; }  // access ports accept untagged only
        vlan = in.accessVlan;
    } else {
        if (!pf->tagged || !in.allowed.count(pf->vlan)) { in.drops++; return out; }
        vlan = pf->vlan;
    }

    // 2. Learn source MAC
    if (!(pf->src[0] & 1)) table_[{vlan, pf->src}] = MacEntry{inPort, now};

    std::vector<uint8_t> payload(frame.begin() + pf->payloadOffset, frame.end());
    auto emit = [&](Port& p) {
        uint16_t tag = (p.mode == PortMode::Trunk) ? vlan : 0;  // trunk = tagged egress
        out.push_back(Output{p.id, buildFrame(pf->dst, pf->src, tag, pf->ethertype, payload)});
        p.tx++;
    };

    // 3. Forward or flood
    bool multicast = pf->dst[0] & 1;
    int outPort = 0;
    if (!multicast && lookup(vlan, pf->dst, outPort)) {
        if (outPort == inPort) { filtered_++; return out; }
        auto it = ports_.find(outPort);
        if (it != ports_.end()) { emit(it->second); forwarded_++; }
    } else {
        floods_++;
        for (auto& kv : ports_) {
            Port& p = kv.second;
            if (p.id == inPort || !inVlan(p, vlan)) continue;
            emit(p);
        }
    }
    return out;
}

size_t Switch::ageOut(uint64_t now) {
    size_t removed = 0;
    for (auto it = table_.begin(); it != table_.end();) {
        if (now - it->second.lastSeen > aging_) { it = table_.erase(it); ++removed; }
        else ++it;
    }
    return removed;
}

void Switch::printMacTable() const {
    std::cout << "MAC table (" << table_.size() << " entries)\n";
    for (const auto& kv : table_) {
        std::cout << "  vlan=" << kv.first.first << " mac=" << macToString(kv.first.second)
                  << " port=" << kv.second.port << " last_seen=" << kv.second.lastSeen << "\n";
    }
}

void Switch::printStats() const {
    for (const auto& kv : ports_) {
        const Port& p = kv.second;
        std::cout << "stats port=" << p.id << " rx=" << p.rx << " tx=" << p.tx
                  << " drops=" << p.drops << "\n";
    }
    std::cout << "stats switch floods=" << floods_ << " forwarded=" << forwarded_
              << " filtered=" << filtered_ << " mac_entries=" << table_.size() << "\n";
}