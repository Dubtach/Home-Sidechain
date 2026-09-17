# Home-Sidechain v45

Three things this pass: loop-safe shapes, a compact face, and the Home-Disto look
applied to both plugins.

---

## 1. The shapes are sidechain-shaped now

You were right, and it's the most important fix here.

A drawn sidechain envelope is **cycled like an LFO**. Whatever value the shape
ends on is the value it steps away from at the next trigger. Every v44 preset
ended at 1.0 and the next cycle began at 0.0, so the loop point was a step
discontinuity in gain — a click generator, every beat, forever.

Every factory shape now satisfies two rules:

- **`y(1.0) == y(0.0)`.** The cycle ends at the height it starts from.
- **The last ~5% is a ramp down to that height, never a vertical drop.** At
  128 BPM on a 1/4 cycle that's about a 23 ms fall. This is the same move
  VolumeShaper's trigger pre-smoothing makes with its "tight fade just before
  the transient", and it doubles as the hold-before-the-next-cycle that gives
  four-to-the-floor pumping its shape.

The same rule holds for internal drops too: `Double` and `Triplet` fall over
4–5% of the cycle rather than instantly.

The rule is enforced in the editor, not just in the presets:

- The two outer points are drawn in **violet** and marked with a dashed seam
  line across the graph at their shared height.
- **Dragging either one moves both.** You cannot end the cycle somewhere other
  than where it starts.
- Neither can be deleted.
- Everything in between is yours — 16 points, free drag, bendable segments.

`SMOOTH` also moved from 1.5 ms to 4 ms by default, in the neighbourhood of the
~6 ms anti-click window Kickstart uses for its MIDI mode.

## 2. Compact

**940 × 620 → 720 × 430**, the same footprint as Home-Disto.

That came out of cutting, not shrinking. `SMOOTH`, `LENGTH`, `LOW CUT`,
`HIGH CUT` and the trigger source moved behind **ADV**, the way Home-Disto hides
its settings. The front face is now only what you touch while writing: the
curve, eight shapes, depth, mix, run mode, rate. Nothing was shrunk to 7pt to
make it fit.

Fixed size now rather than scaled-resizable — at this footprint the scaling was
solving a problem that no longer exists. Easy to add back.

## 3. The Home look, shared

`source/Shared/HomeSeriesUI.h` is new: palette, card treatment, brand header,
pill, knob, power switch, lamp. Header-only, everything inline, included by both
plugins. The Trigger and the Receiver are now identical by construction rather
than by me copying colours between two files — change the card gradient once and
both move.

Straight from Home-Disto: `#09090B` chassis, `#111114` face plate, the four card
colours (`#00E5FF` / `#00FF87` / `#B900FF` / `#FF007F`), the drop-shadow +
gradient + sheen + cross-hatch card, dark ink text on the cards, white knob arcs
with tick rings, `Home-` in white and the second half in the accent.

Card assignment is consistent across both plugins, so the colour tells you what
kind of thing you're looking at:

| | Cyan | Pink | Purple | Green |
|---|---|---|---|---|
| **Trigger** | input scope | sensitivity | activity | sending |
| **Receiver** | shape graph | output | shapes | timing |

### The Trigger was rewritten too

Its editor was never in any upload — the Receiver files kept overwriting it — so
this is a fresh editor against the same processor. **The Trigger's
`PluginProcessor.h/.cpp` are untouched**; only `PluginEditor.h/.cpp` change.

You lose the old `HomeSeriesTriggerLookAndFeel`, `HomeSidechainTriggerGapSlider`
and `HomeSidechainTriggerLinkSelector` classes; the shared UI replaces them. If
there was behaviour in the old gap slider you want kept, send me that file and
I'll fold it back in.

---

## Not compiled

No JUCE here. Delimiters balance and every declaration has a definition, but
budget for a line or two on first build. Most likely spot is `FontOptions` if
your JUCE is 7.x rather than 8.

## Worth checking first

- Loop a bar in host-sync mode with `DEPTH` at 100% and listen at the seam. It
  should be silent there. If it isn't, the shape's ends have drifted apart and
  that's a bug in the seam lock, not in your ears.
- Drag an end point up and confirm the other end follows.
- `Gate` and `Stutter`-style shapes at 1/16 — the 5% fall gets short at that
  rate, so raise `SMOOTH` if you hear edges.
