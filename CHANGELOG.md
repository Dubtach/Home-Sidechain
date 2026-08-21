# Home-Sidechain Changelog

## 1.3.3
- Fixed Trigger sender sequence data race between audio and worker threads.
- Receiver now assigns its own per-link sequence numbers so multiple Trigger instances can share one Link without sequence collisions.
- Reduced single-port Receiver polling interval to 1 ms.
- Prevented Home-Link + MIDI duplicate hits from double-triggering when Source = Both.

## 1.3.2
- Fixed Receiver `juce::jlimit` float/double template mismatch.
- Restored a real cross-plugin Home-Link transport using localhost UDP, which works across the separate Trigger and Receiver plugin binaries.
- Reduced Receiver socket polling from eight ports to one process-local port to reduce routing overhead and jitter.
- Kept all socket I/O off the real-time audio thread.
- Sorted and de-duplicated Receiver trigger points so Home-Link + MIDI cannot double-trigger the same hit.
- Preserved automatic Home-Link operation with no DAW MIDI routing required.
