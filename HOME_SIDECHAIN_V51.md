# Home-Sidechain v51 — Matched to Home-Disto, restructured Trigger, new visualizers

All of `source/Receiver/*`, `source/Trigger/*`, and `source/Shared/HomeSeriesUI.h`
changed. I read your actual Home-Disto source this time rather than working
from the screenshot description, and matched its real drawing code directly.

## Knobs now match Home-Disto exactly

Pulled `drawRotarySlider` straight from your Disto header: dark body, 11 tick
marks around the travel, a black background arc, a neon glow arc (14px,
60% opacity) under a white arc (5px) on top, a glowing pointer needle, and a
faint outer ring. Every knob now takes an accent colour matching its card
(pink for Output/Sensitivity, green for Length, purple for the Filters
knobs) the same way Disto's own glow colour is set per-knob.

## Header matches Home-Disto's icon language

- **Settings/Advanced** is a real gear icon now -- six rectangular spokes
  around a ringed hub, copied from Disto's `SETTINGS` icon path -- on the
  same dark rounded-square tile Disto uses, not a text pill.
- **Bypass** is Disto's actual power-symbol icon (circular arc with a gap,
  plus the vertical tick through the top) on the same square tile, not a
  circular lamp.
- No preset system added, per your note.
- **Link** changed shape and colour: it's a row of individual dark square
  tiles now (matching Disto's own button tiles exactly, including the
  0x161618 fill and 4px corner radius), with the active link filled solid
  cyan -- the same "solid fill = engaged" language Disto's lock icon uses.
  Replaces the connected sliding-track look from last round.

## Receiver: a live input meter in the Shape section

Worth explaining why this isn't a scrolling waveform: the Shape graph's
x-axis is *cycle phase* (0 to 1 within one envelope), not wall-clock time.
A time-based scrolling waveform would never line up with that axis --
it'd be showing two unrelated things on the same ruler. So instead there's
a live input-level meter in a reserved strip on the graph's right edge,
with a decaying peak-hold line. It's driven by the same `inputLevelForUI`
the processor was already tracking, so no processing changes were needed,
just the missing visual.

## Trigger: Activity and Sending cards removed

- The Sending lamp now lives inside the Input graph itself, top-left, lit
  briefly whenever a trigger actually fires.
- The Audio/MIDI legend that used to be there moved to make room and now
  sits top-right of the same row.
- The input level bar (previously its own Activity card) is now a thin
  strip along the bottom of the Input graph, with the threshold shown as a
  marker line on it instead of a separate readout.
- Triggers-sent, dropped, and cool-down numbers are gone, as asked.
- Input and Sensitivity now each span the full content height (312px)
  instead of stacking with two cards below them, and Threshold/Cool Down
  grew to fill the extra room properly rather than leaving it empty.

## Verified by rendering every state again

Same offscreen JUCE pipeline as the last two rounds: default view, Filters
tab, Sync off, Host mode, the Advanced panel, and the Trigger plugin's new
layout -- six real screenshots, all inspected before this went out. The gear
icon, power icon, link tiles, knob glow, sending lamp, and level bar were
all checked at 2x-4x zoom for genuine pixel-level correctness, not just "it
compiles."

## Compile-checked

All four files clean at `-std=c++17 -Wall -Wextra`, and again with
`near`/`far` predefined empty to simulate real Windows conditions.
