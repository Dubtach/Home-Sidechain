# Home-Sidechain Receiver v42 — Deep Polish

Receiver is now intentionally a simple sidechain ducking / volume-shaper instrument.

## User model
- Trigger event starts an envelope cycle.
- The graph is the envelope: higher graph values mean deeper ducking.
- DEPTH sets maximum attenuation.
- MIX blends the ducked signal with dry audio.
- SYNC + RATE define the cycle length; LENGTH remains available internally for unsynced compatibility.
- A/B/C routes Home-Link events.
- MIDI and Home-Link can trigger the Receiver.
- LOW CUT / HIGH CUT define the audio band that receives ducking.

## Editor
- Home Trigger theme reused: dark chassis, cyan/violet/green/red accents, Helvetica.
- Header mirrors the finished Trigger plugin.
- Main graph supports draggable nodes, double-click node creation, curve handles, and node reset/delete behavior.
- Trigger state is integrated into the graph header.
- Compact Mix/Depth controls and a filter editor sit underneath.
- Rate controls and visual curve presets are kept compact and secondary to the graph.

## Design decisions
- Removed redundant visible compressor-style controls and kept the graph as the primary interaction model.
- Kept legacy APVTS parameters for state compatibility, but they are not required by the new interface.
- No additional latency is introduced by the UI or receiver transport.
