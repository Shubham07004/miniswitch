#include "frame.hpp"
#include "lpm.hpp"
#include "switch.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::set<uint16_t> parseVlanList(const std::string& s) {
    std::set<uint16_t> r;
    std::stringstream ss(s);
    std::string t;
    while (std::getline(ss, t, ',')) r.insert(static_cast<uint16_t>(std::stoi(t)));
    return r;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << " <config> <scenario> [--quiet]\n";
        return 1;
    }
    bool quiet = argc > 3 && std::string(argv[3]) == "--quiet";

    Switch sw;
    LpmTable rt;

    std::ifstream cfg(argv[1]);
    if (!cfg) { std::cerr << "cannot open config " << argv[1] << "\n"; return 1; }
    std::string line;
    while (std::getline(cfg, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string cmd;
        ls >> cmd;
        if (cmd == "port") {
            int id; std::string mode, arg;
            ls >> id >> mode >> arg;
            if (mode == "access") sw.addAccessPort(id, static_cast<uint16_t>(std::stoi(arg)));
            else if (mode == "trunk") sw.addTrunkPort(id, parseVlanList(arg));
            else std::cerr << "bad port line: " << line << "\n";
        } else if (cmd == "route") {
            std::string cidr, nh;
            ls >> cidr >> nh;
            if (!rt.insertCidr(cidr, nh)) std::cerr << "bad route line: " << line << "\n";
        } else if (cmd == "aging") {
            uint64_t a; ls >> a; sw.setAging(a);
        }
    }

    std::ifstream sc(argv[2]);
    if (!sc) { std::cerr << "cannot open scenario " << argv[2] << "\n"; return 1; }

    uint64_t now = 0, frames = 0;
    auto t0 = std::chrono::steady_clock::now();
    while (std::getline(sc, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string cmd;
        ls >> cmd;
        if (cmd == "time") {
            ls >> now;
        } else if (cmd == "age") {
            size_t n = sw.ageOut(now);
            if (!quiet) std::cout << "age @" << now << ": removed " << n << " entries\n";
        } else if (cmd == "frame") {
            int in; std::string s, d; unsigned vlan;
            ls >> in >> s >> d >> vlan;
            Mac sm, dm;
            if (!parseMac(s, sm) || !parseMac(d, dm)) { std::cerr << "bad frame line: " << line << "\n"; continue; }
            auto f = buildFrame(dm, sm, static_cast<uint16_t>(vlan), 0x0800, std::vector<uint8_t>(46, 0));
            ++frames;
            auto outs = sw.process(in, f, now);
            if (!quiet) {
                std::cout << "frame in=" << in << " src=" << s << " dst=" << d << " vlan_tag=" << vlan << "\n";
                if (outs.empty()) std::cout << "  -> dropped/filtered\n";
                for (const auto& o : outs) {
                    auto pf = parseFrame(o.frame);
                    std::cout << "  -> port " << o.port
                              << (pf && pf->tagged ? " (tagged vlan " + std::to_string(pf->vlan) + ")" : " (untagged)")
                              << "\n";
                }
            }
        } else if (cmd == "route") {
            std::string ip; ls >> ip;
            uint32_t v;
            if (!parseIpv4(ip, v)) { std::cerr << "bad ip " << ip << "\n"; continue; }
            int len = 0;
            auto nh = rt.lookup(v, &len);
            if (!quiet) {
                std::cout << "route " << ip << " -> " << (nh ? *nh + " (/" + std::to_string(len) + ")" : "no route") << "\n";
            }
        } else if (cmd == "show") {
            std::string what; ls >> what;
            if (what == "mac" && !quiet) sw.printMacTable();
        }
    }
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count();
    if (us == 0) us = 1;

    sw.printStats();
    std::cout << "RESULT frames=" << frames << " time_us=" << us
              << " mpps=" << static_cast<double>(frames) / static_cast<double>(us) << "\n";
    return 0;
}