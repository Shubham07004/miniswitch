# MiniSwitch
User-space C++17 model of a carrier-style L2 switch with 802.1Q VLANs
(access/trunk), MAC learning with aging, flooding, per-port counters,
and an IPv4 longest-prefix-match routing table (binary trie).

Build: `./build.sh`   Test: `./build/tests`
Demo:  `./build/miniswitch config/switch.cfg examples/demo.txt`
Bench: `python3 scripts/bench.py 200000` (writes docs/report.md)

## Results
- 200,000 frames processed at ~0.7 Mpps (single thread, -O2)
- Valgrind memcheck: 0 errors, no leaks
- Unit tests cover VLAN isolation, trunk tagging, MAC learning/aging, and LPM

## Debugging & validation
- Valgrind memcheck clean on the test binary
- Used gdb breakpoints on Switch::process to inspect parsed frames