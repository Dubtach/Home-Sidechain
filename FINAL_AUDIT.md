# Final Trigger Audit – v36.0

Checked statically against the v35.9 source before packaging:

- Audio detection remains sample-by-sample with linear threshold comparison; no per-sample dB conversion.
- Audio Cool Down still gates only audio-triggered events.
- Incoming MIDI note-ons bypass Cool Down and trigger immediately.
- MIDI visualization state remains independent of the audio Cool Down.
- Home-Link audio-thread path remains queue-based; socket I/O stays on the sender thread.
- A/B/C routing remains direct-click and maps to links 0–2.
- Bypass resets edge state to avoid a stale above-threshold re-trigger.
- MIDI note-off scheduling remains sample-accurate within the current host block.
- APVTS state serialization remains intact for Threshold, Cool Down, Link, and Bypass.
- No new locks, allocations, logging, or blocking waits were introduced on the audio thread.

No intentional processing behavior was changed in this final UI pass.
