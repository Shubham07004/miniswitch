#include "frame.hpp"
#include "lpm.hpp"
#include "switch.hpp"
#include <iostream>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << "FAIL line " << __LINE__ << ": " #c "\n"; failures++; } } while (0)

static const Mac A = {0xaa, 0, 0, 0, 0, 1};
static const Mac B = {0xaa, 0, 0, 0, 0, 2};
static const Mac C = {0xaa, 0, 0, 0, 0, 3};
static const Mac D = {0xaa, 0, 0, 0, 0, 4};
static const Mac BCAST = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static std::vector<uint8_t> mk(const Mac& src, const Mac& dst, uint16_t vlan) {
    return buildFrame(dst, src, vlan, 0x0800, std::vector<uint8_t>(46, 0));
}
static bool hasPort(const std::vector<Output>& o, int p) {
    for (const auto& x : o) if (x.port == p) return true;
    return false;
}

int main() {
    Switch sw;
    sw.setAging(300);
    sw.addAccessPort(1, 10);
    sw.addAccessPort(2, 10);
    sw.addAccessPort(3, 20);
    sw.addTrunkPort(4, {10, 20});

    // Unknown unicast floods only inside VLAN 10
    auto o = sw.process(1, mk(A, B, 0), 0);
    CHECK(o.size() == 2);
    CHECK(hasPort(o, 2));
    CHECK(hasPort(o, 4));
    CHECK(!hasPort(o, 3));
    for (const auto& x : o) {
        auto pf = parseFrame(x.frame);
        CHECK(pf.has_value());
        if (x.port == 4) CHECK(pf->tagged && pf->vlan == 10);   // trunk egress tagged
        else CHECK(!pf->tagged);                                // access egress untagged
    }

    // A was learned on port 1, so B -> A goes only to port 1
    o = sw.process(2, mk(B, A, 0), 1);
    CHECK(o.size() == 1 && o[0].port == 1);

    // B was learned too, so A -> B is unicast to port 2
    o = sw.process(1, mk(A, B, 0), 2);
    CHECK(o.size() == 1 && o[0].port == 2);

    // Broadcast in VLAN 20 reaches only the trunk, tagged 20
    o = sw.process(3, mk(C, BCAST, 0), 3);
    CHECK(o.size() == 1 && o[0].port == 4);
    auto pf = parseFrame(o[0].frame);
    CHECK(pf && pf->tagged && pf->vlan == 20);

    // Tagged frame on an access port is dropped
    o = sw.process(1, mk(A, B, 10), 4);
    CHECK(o.empty());

    // VLAN 30 not allowed on the trunk
    o = sw.process(4, mk(D, C, 30), 5);
    CHECK(o.empty());

    // MAC lookup and aging
    int p = 0;
    CHECK(sw.lookup(10, A, p) && p == 1);
    sw.ageOut(1000);
    CHECK(!sw.lookup(10, A, p));

    // LPM: longest prefix wins
    LpmTable rt;
    rt.insertCidr("10.0.0.0/8", "R1");
    rt.insertCidr("10.1.0.0/16", "R2");
    rt.insertCidr("0.0.0.0/0", "GW");
    uint32_t ip;
    int len = 0;
    parseIpv4("10.1.2.3", ip);
    CHECK(rt.lookup(ip, &len).value() == "R2" && len == 16);
    parseIpv4("10.9.9.9", ip);
    CHECK(rt.lookup(ip).value() == "R1");
    parseIpv4("8.8.8.8", ip);
    CHECK(rt.lookup(ip).value() == "GW");

    std::cout << (failures ? "TESTS FAILED" : "ALL TESTS PASSED") << "\n";
    return failures ? 1 : 0;
}