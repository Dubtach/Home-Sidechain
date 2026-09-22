# Home-Sidechain v52 — Bypass overlay, output meter, Trigger resized, header cleanup

Both `source/Receiver/*` and `source/Trigger/*` changed. Six specific fixes.

## Bypass overlay, matching Home-Disto exactly

Pulled straight from your Disto source: when bypassed, the whole face plate
dims to 70% black, "BYPASSED" appears centered in 48pt bold white, and the
bypass button itself is excluded from the dimming so it's always reachable
to turn back off. Same in both plugins.

## Receiver meter: output instead of input, and much quieter

Two changes:
- The processor now tracks the **processed (post-mix) signal** instead of
  the dry input -- `inputLevelForUI` is `outputLevelForUI` now, computed
  from the sample actually written to the buffer. This shows what the
  shape did to the sound, not just what came in.
- Visually toned way down: the cyan-to-green gradient fill is gone,
  replaced with a single muted white fill at low opacity. It reads as a
  background utility meter now instead of competing with the curve for
  attention.

## Trigger: back down to a sensible size

I agree it was too tall -- 312px for Input/Sensitivity was more room than
that content needs. Back to 220px (a modest 18px more than the original
202px, just enough for the new lamp/legend/level-bar row inside Input
without cramming). Threshold and Cool Down are sized to fit properly again
rather than being stretched to fill leftover space. Window height dropped
from 416px to 320px to match -- the plugin is meaningfully more compact now,
not just the cards inside it.

## Trigger: Test button removed

It's gone from both the layout and the code -- not just hidden. Power kept
the same position (660, 20, 30x30) that Receiver's power button uses, so
the two plugins line up with each other if you have both open.

## Header: more breathing room, consistent text weight

The Receiver/Trigger subtitle block (the small "RECEIVER" / "TRIGGER" +
BPM/note line) moved from x=202 to x=235, giving it real clearance from the
title instead of sitting close enough to nearly touch. I also found and
fixed the actual cause of the "looks broken" read: a leftover dead
`setColour` call, and both lines defaulting to **bold** at that small size
sitting right under a bold 22pt title -- which is what made it look
mismatched rather than like a clean two-line subtitle. Both lines are
regular weight now, consistently sized, left-aligned to each other.

## Verified by rendering again, including the overlay

Same offscreen JUCE pipeline as every round this week, now also checking a
bypassed state for both plugins (confirmed the dimming, the excluded power
button, and the "BYPASSED" text all render correctly) alongside the
existing default/Filters/Sync-off/Host-mode/Advanced states. Trigger's
render came back at exactly 720x320, confirming the resize took.

## Compile-checked

All four files clean at `-std=c++17 -Wall -Wextra` (including
`-Wunused-variable`), and again with `near`/`far` predefined empty to
simulate real Windows conditions.
