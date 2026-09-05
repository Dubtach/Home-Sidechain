# Home-Sidechain Receiver v39 — Shaper Redesign

The Receiver is designed around the curve-first interaction model used by modern volume-shaping and sidechain-shaping tools. The graph is the primary control surface; points are directly draggable, timing regions are visible, and the live envelope playhead shows the current ducking position.

UI:
- Large central shaper graph with editable envelope points.
- Compact Home-series header with LINK, Bypass, TEST and status.
- Five concise controls: Depth, Attack, Hold, Release, Mix.
- TEST fires the same envelope without an incoming trigger.
- Double-click an envelope point to reset the curve.
- No SOURCE/MODE/SYNC/BARS clutter in the interface.

Processing is unchanged from v38.x: Home-Link and MIDI are accepted in parallel, with the selected A/B/C link determining the Home-Link channel and matching MIDI note. Existing parameter IDs remain for state compatibility.
