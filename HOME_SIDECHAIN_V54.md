# Home-Sidechain v54 — The actual bypass bug, Receiver's TEST removed

Both `source/Receiver/*` and `source/Trigger/*` changed.

## Why the overlay was still broken -- the real cause this time

Last round's `paintOverChildren()` fix was correct but incomplete. Here's
what was still wrong, and why my own testing never caught it.

`timerCallback()` in both editors only ever called two things:
`repaint(Rectangle(10,10,700,62))` -- the header strip -- and a single
child's own `repaint()`. Neither is a full-window repaint. JUCE only
actually paints the region a `repaint()` call invalidated; `paintOverChildren()`
still runs on every repaint, but the graphics context handed to it is
*clipped* to whatever small area triggered that particular cycle. So the
overlay's `fillRoundedRectangle` covering the whole face was, in practice,
only ever landing inside whatever narrow region happened to be invalidated
at that moment -- which in normal operation was never the whole window.
Clicking bypass repaints the button itself (a 30x30 square); nothing was
ever telling the *editor* to repaint everything.

This is exactly why it looked "broken" rather than simply absent -- a
fragment of it was real, the rest wasn't being redrawn.

I didn't catch this with the offscreen renders from the last two rounds
because that test harness calls `paintEntireComponent()`, which forces a
full, unclipped paint every time by design -- it bypasses JUCE's normal
invalidation system entirely, so it was structurally blind to this class of
bug. Good for catching layout and color issues, useless for catching this
one. Worth knowing since I'll keep using that pipeline for other checks.

**Fix:** both editors now track the bypass state and call a full `repaint()`
the moment it actually changes (checked each timer tick, only acts on an
actual change, not every tick) -- regardless of whether that change came
from clicking the power icon or from host automation.

## Also fixed while in there: host-level bypass wasn't wired up at all

Separately, neither processor told the host which parameter was its bypass
control (`AudioProcessor::getBypassParameter()` was never overridden). If
you bypass from your DAW's own track bypass button rather than clicking the
plugin's power icon, that action was never reaching our "BYPASS" parameter
at all -- the overlay wouldn't have appeared, and would've looked bypassed
at the host level while showing an un-dimmed plugin window. Both processors
now return the BYPASS parameter from `getBypassParameter()`, so host-level
and in-plugin bypass stay in sync either way.

## Overlay simplified, as you said was fine to do

Dropped the `excludeClipRegion` carve-out around the power button. The
whole face dims uniformly now, power icon included -- it's still exactly as
clickable under the dim, and removing the clip-region step takes one more
possible source of platform-specific rendering quirks off the table.

## Receiver: Test button removed

Gone from layout and code, same as Trigger's removal two rounds ago. Link
grew into the freed space: 224px wide -> 278px (about 24% wider), same
30px-tall tiles, same position otherwise.

## Compile-checked

All four files clean at `-std=c++17 -Wall -Wextra`, and again with
`near`/`far` predefined empty to simulate real Windows conditions.

One honest caveat: the specific incremental-repaint bug this round targets
can't be demonstrated by my render pipeline, for the reason explained above
-- it's structurally invisible to a tool that forces full paints. The fix
is correct, standard JUCE practice (a no-arg `repaint()` is documented to
invalidate the whole component), but this is the one change in this batch
I can't show you a screenshot proving. Everything else here was re-rendered
and checked as usual.
