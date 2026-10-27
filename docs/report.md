# MiniSwitch Characterization Report

- Frames processed: 200000
- Wall time: 304196 us
- Throughput: 0.657 Mpps (657471 frames/s)
- Traffic mix: 10% broadcast, rest unicast, 5 hosts, 2 VLANs, 1 trunk

## Counters

    stats port=1 rx=40168 tx=51185 drops=0
    stats port=2 rx=40002 tx=51401 drops=0
    stats port=3 rx=40009 tx=32749 drops=0
    stats port=4 rx=79821 tx=84077 drops=0
    stats switch floods=106464 forwarded=57481 filtered=36055 mac_entries=5

## Observations
- Floods dominate: broadcasts (10%) always flood, and a destination in a different VLAN is never learned in the sender's VLAN, so it floods too.
- Filtered frames come from the random generator picking the same host as source and destination (about 1 in 5 unicast frames).
- Trunk port carries both VLANs tagged; access ports see untagged frames only.
- Single-threaded, -O2: roughly 0.7 Mpps. Likely hotspots: per-frame vector allocation and std::map lookups (to be confirmed with a profiler).
