# Home-Sidechain v47 — Receiver polish + a real color-consistency bug

Only `source/Receiver/` files changed this round. Trigger is untouched.

## Length knob moved out of Advanced

Length now lives in the timing card itself, not behind ADV. It only means
anything when Sync is off (in Trigger mode -- the rate always drives the
cycle otherwise), so it occupies the exact spot the rate selector uses:
when Sync is off, Length replaces the rate readout there; turn Sync back on
and the rate selector returns. One slot, whichever control is relevant.

## Shift+drag now snaps

This was backwards before. Shift used to force a drag to stay unsnapped;
now it forces a snap, regardless of the "Always snap" setting in Advanced.
So: drag freely by default, hold Shift any time you want that one drag to
land exactly on the grid. If "Always snap" is turned on, every drag already
snaps and Shift makes no difference (nothing to override).

## Shapes: Deep removed, cells wider

Removed "Deep" from the factory shapes (down to 7). The strip divides the
same card width by however many shapes there are, so with one fewer shape
each thumbnail is proportionally wider -- less of the squeezed, elongated
look you were seeing.

## Timing shorter, Output matches Shape

Timing: 202px -> 170px tall. Output now has the exact same height and row
position as Shape (both 100px, both starting at the same y) instead of
being a slightly different size sitting in roughly the same place.

## The color-swap bug

This is the one I want to actually explain, because you were right that
something was inconsistent, and I found it.

Every button in this plugin family is supposed to follow one rule: selected
= bright/white fill with dark text, unselected = dark fill with light text.
That's `homeUI::Pill`, and it's what all the link buttons, TRIG/HOST, SYNC,
and the Advanced panel's toggles do. It's also exactly what your Home-Disto
screenshot shows for the MODE buttons (TUBE selected = white pill, black
text; everything else = dark pill, white text).

One component didn't follow it: the shape-preset cells. Selected there was
a *darker* fill with brighter white text -- the opposite background logic
from every other button in the plugin, while still using white text either
way. So depending which control you were looking at, "the bright one" could
mean selected or not-selected. That's exactly the kind of thing that reads
as "confusing, hard to tell which is which" even when you can't immediately
name why.

Fixed: shape cells now follow the same rule as everything else. Selected =
solid white fill, dark curve and dark label text. Unselected = dark fill,
dimmed white curve and label. One convention, everywhere, no exceptions.

## Compile-checked

Same process as the last two drops: JUCE master, all four files at
`-std=c++17 -Wall -Wextra`, then again with `near`/`far` predefined empty to
reproduce real `windef.h` conditions. Clean both times.
