# Home-Sidechain v46 — Receiver layout pass + Trigger MIDI visibility

## Receiver

**Shape strip, shorter.** The complaint was right: at the old height, each
preset thumbnail was a tall narrow rectangle, so an ordinary curve read as
stretched vertically. The strip itself is shorter now; the shape card shrank
to match (122px → 100px) rather than leaving dead space.

**Reset is now an icon**, not a text button — a circular arrow glyph, same
idea as the reset icon on Home-Disto's EQ card. It sits in the shape card's
top-right corner instead of taking up width in the strip's row.

**Timing moved up, Output moved down.** Literally a swap: Timing now sits in
the taller top-right slot Output used to occupy, and Output sits in the
shorter bottom-right slot Timing used to occupy. Depth and Mix are side by
side there now, which is what that slot's proportions call for.

**Snap moved into Advanced, and now defaults off.** The front-panel SNAP pill
is gone. In its place, under ADV: "Always snap to grid," off by default —
dragging a node is free-hand out of the box, Shift still gives fine control
if you turn it on. This was a real behaviour change, not just a relocation:
v45 defaulted to snap-on.

**Rate list: 6 → 22.** Straight rates kept their original 6 (and their
original index order, so nothing about existing sessions changes). Added:

- **Triplet** — 1/16T, 1/8T, 1/4T, 1/2T, 1/1T (straight duration × 2/3)
- **Dotted** — 1/16., 1/8., 1/4., 1/2., 1/1. (straight duration × 1.5)
- **Poly-groove** — 1/3, 1/5, 1/6, 1/7, 1/9, 1/12 (the bar split into that
  many equal pulses — what you asked for by name)

22 rates doesn't fit as a row of pills without turning the timing card into a
wall of tiny buttons, so it's a different control now: a large readout
showing the current rate and its category, with prev/next steppers on either
side, and clicking the readout opens the full list as a menu grouped into
Straight / Triplet / Dotted / Groove. `HomeSidechainReceiverAudioProcessor`
exposes the category boundaries as constants (`numStraightRates` etc.) so the
UI and the processor can't drift apart on what's in which group.

One thing worth knowing: some poly-groove and triplet durations are
mathematically identical (a bar split into 3 *is* the same length as a
half-note triplet). That's correct, not a bug — the two names describe the
same duration from different musical angles, and having both in the menu
under their own labels is exactly the point.

## Trigger

**MIDI notes now show up on the scope.** They already caused the plugin to
fire — the bug was purely visual. A MIDI note carries no audio amplitude, so
the waveform bar at that instant was close to a single pixel tall, and only
audio transients got the full-height highlight that makes a trigger easy to
spot. MIDI triggers now get their own full-height purple tint plus a small
flag marker at the top of the graph, so they're visible regardless of what
the audio was doing at that moment. A small AUDIO/MIDI legend in the top-right
of the scope explains the two colours.

## Compile-checked

Same as last time, plus a step further: I pulled JUCE master, syntax-checked
all four translation units at `-std=c++17 -Wall -Wextra`, then again with
`near`/`far` predefined empty to reproduce real `windef.h` conditions (the
cause of the last build failure). All four clean both times. Still GCC on
Linux rather than MSVC on Windows, so a purely MSVC-flavoured complaint could
still surface — send me the log if one does.
