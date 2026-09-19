# Home-Sidechain v49 — Output/Timing repositioned, verified by actually rendering it

Only `source/Receiver/*` changed. Trigger is untouched (I did check it this
round -- see below).

## Output moved, Timing extended

Output's bottom now lines up exactly with Shape's bottom (both at y=388).
Timing grew from 170px to 178px to fill the space above it, with the same
12px gap under Timing that the rest of this layout uses between cards.
Output kept the 200x122 size and knob layout from last round -- only its
position changed.

## This round I actually rendered the UI instead of reasoning about pixels blind

Up to now every layout fix in this thread was me computing coordinates on
paper and hoping. This time I got a real offscreen JUCE render pipeline
working -- compiled JUCE itself plus your plugin source, instantiated the
actual editor, and painted it to PNG. No emulator, no DAW, just your real
code producing real pixels. That let me check claims instead of guessing at
them, and it caught one real bug that paper math had missed:

**Found and fixed:** the gain readout ("0.0dB") in the top-right of the
curve graph was overlapping the top-right curve node whenever a shape had a
point near full value out there -- which is most of them, including
Classic. The readout now sits in its own reserved strip above the plot,
never reachable by any node regardless of shape.

**Checked and confirmed fine, not touched:**
- Timing's Sync checkbox, the TRIG/HOST divider, and the disabled/ghosted
  look when Sync doesn't apply (Host mode) -- all read clearly.
- The Length-knob-replaces-rate-selector swap when Sync is off -- verified
  by actually rendering that state.
- Shape thumbnail selected/unselected contrast -- I'd misread a compressed
  preview as "background went white" two messages ago; pixel-sampled the
  real render and confirmed the revert is correct, it's the bold white
  curve+text dominating the impression, not the fill.
- Output's knobs -- properly sized, not squished, comfortable margins.
- The Advanced panel's spacing -- I'd eyeballed a crop and thought there was
  a bad gap; measured it precisely and it's a normal 26px, not a bug.
- Title vs. "RECEIVER/BPM" label in the header -- tight but not colliding.
- The Trigger plugin -- rendered its default state too since your notes
  said "this plugin" generically. Nothing wrong found: scope, sensitivity
  knobs, activity meter, and sending lamp are all clean, no overlaps.

I'm noting the false leads deliberately rather than pretending I got it
right on the first look -- the render caught a real bug and also stopped me
from "fixing" three things that weren't broken.

## Compile-checked, same as every round

All four files clean at `-std=c++17 -Wall -Wextra`, and again with
`near`/`far` predefined empty to simulate real Windows conditions.
