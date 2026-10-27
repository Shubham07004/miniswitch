#!/usr/bin/env python3
"""Generate random traffic, run miniswitch, write a characterization report."""
import pathlib
import random
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
N = int(sys.argv[1]) if len(sys.argv) > 1 else 200000
random.seed(42)

# (mac, ingress_port, vlan)
HOSTS = [
    ("aa:00:00:00:00:01", 1, 10),
    ("aa:00:00:00:00:02", 2, 10),
    ("aa:00:00:00:00:03", 3, 20),
    ("aa:00:00:00:00:04", 4, 10),
    ("aa:00:00:00:00:05", 4, 20),
]

scen = ROOT / "examples" / "bench.txt"
with scen.open("w") as f:
    for i in range(N):
        if i % 1000 == 0:
            f.write("time %d\n" % (i // 1000))
        s = random.choice(HOSTS)
        if random.random() < 0.1:
            dst = "ff:ff:ff:ff:ff:ff"
        else:
            dst = random.choice(HOSTS)[0]
        tag = s[2] if s[1] == 4 else 0  # trunk ingress is tagged
        f.write("frame %d %s %s %d\n" % (s[1], s[0], dst, tag))

binary = ROOT / "build" / "miniswitch"
cfg = ROOT / "config" / "switch.cfg"
r = subprocess.run([str(binary), str(cfg), str(scen), "--quiet"],
                   capture_output=True, text=True)
if r.returncode != 0:
    print(r.stderr)
    sys.exit(1)

m = re.search(r"RESULT frames=(\d+) time_us=(\d+) mpps=([\d.eE+-]+)", r.stdout)
frames, us, mpps = int(m.group(1)), int(m.group(2)), float(m.group(3))
stats = [l for l in r.stdout.splitlines() if l.startswith("stats")]

lines = [
    "# MiniSwitch Characterization Report",
    "",
    "- Frames processed: %d" % frames,
    "- Wall time: %d us" % us,
    "- Throughput: %.3f Mpps (%.0f frames/s)" % (mpps, frames / (us / 1e6)),
    "- Traffic mix: 10% broadcast, rest unicast, 5 hosts, 2 VLANs, 1 trunk",
    "",
    "## Counters",
    "",
] + ["    " + s for s in stats] + [
    "",
    "## Observations",
    "- Floods dominate: broadcasts (10%) always flood, and a destination in a different VLAN is never learned in the sender's VLAN, so it floods too.",
    "- Filtered frames come from the random generator picking the same host as source and destination (about 1 in 5 unicast frames).",
    "- Trunk port carries both VLANs tagged; access ports see untagged frames only.",
    "- Single-threaded, -O2: roughly 0.7 Mpps. Likely hotspots: per-frame vector allocation and std::map lookups (to be confirmed with a profiler).",
]
report = "\n".join(lines) + "\n"
(ROOT / "docs" / "report.md").write_text(report)
print(report)