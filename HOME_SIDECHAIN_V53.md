# Home-Sidechain v53 — The real bypass bug, header rebuilt, sending fixed

Both `source/Receiver/*` and `source/Trigger/*` changed, plus
`source/Shared/HomeSeriesUI.h`.

## The actual bypass bug

Your screenshot made it obvious something was wrong, and comparing it to
Disto's real code found the exact cause: I'd written the overlay inside
`paint()`. In JUCE, `paint()` runs *before* child components paint --
meaning every knob, pill, and the curve graph painted themselves right back
on top of my dim-and-"BYPASSED" overlay immediately afterward, in whatever
area each one occupied. The overlay wasn't broken, it was just getting
erased everywhere a control existed, leaving it visible only in the gaps
between them.

Disto's own code puts this exact overlay in `paintOverChildren()` -- a
second pass that runs *after* every child has already painted, so it always
ends up on top, cleanly, everywhere. That's where it lives now in both
plugins. Confirmed by rendering a bypassed state again: the dim and
"BYPASSED" now sit uniformly over the curve, every knob, and every button,
exactly like your reference.

## Header rebuilt

- **Subtitle block removed.** The title is one line now: "Home-Sidechain
  (Receiver)" / "Home-Sidechain (Trigger)" -- the plugin name in brackets,
  smaller and dimmer than the main title, right on the same line. No more
  separate row underneath for it.
- **Link section increased and repositioned.** Tiles grew from 22px to 30px
  tall (30-40% bigger) and now sit vertically centered with the gear and
  power icons -- same row, same center line, instead of being offset from
  them. Font size bumped from 9.5 to 11 to match. Positioned with real
  clearance after the title instead of crowding it.
- The Receiver's Home-Link status indicator is a plain dot now, sized and
  placed on the same row as everything else, instead of a separate labelled
  lamp taking up header width on its own.

## Trigger: Sending indicator, actually noticeable now

Bigger dot (9px, was 6px), a real glow bloom when it fires, bold white text
that brightens with activity instead of sitting at one fixed dim shade
regardless of what's happening, and a higher baseline visibility so it's
never fully invisible at rest. It's also now clearly bigger than the
Audio/Midi legend dots next to it, so it reads as the primary indicator
rather than a third equally-weighted dot. The decay is a touch slower too
(0.75 vs 0.68 per tick) so the flash lasts long enough to actually catch.

## Verified by rendering the bypassed state specifically

This was the whole point of this round, so I made sure to check it
directly rather than trust the code read correctly: rendered both plugins
bypassed and inspected the output before shipping. The screenshots are in
`render-verification/` -- `2_receiver_bypassed.png` and
`8_trigger_bypassed.png` are the ones that matter most this round.

## Compile-checked

All four files clean at `-std=c++17 -Wall -Wextra`, and again with
`near`/`far` predefined empty to simulate real Windows conditions.
