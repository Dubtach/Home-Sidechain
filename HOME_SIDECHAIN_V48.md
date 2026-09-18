# Home-Sidechain v48 — Output restored, Timing disambiguated, Shape look reverted

Only `source/Receiver/` and `source/Shared/HomeSeriesUI.h` changed. Trigger
is untouched.

## Output: back to its real size, moved up instead of down

Last round I made Output match Shape's height to line the two bottom cards
up, which meant shrinking it to 100px and squeezing Depth/Mix down to
64px-tall knobs. That's the squish you were right to flag.

Output is back to 200x122 with proper-sized knobs (Depth and Mix still side
by side, per the arrangement you asked for two rounds ago -- only the
*size* was the problem, not the layout). It now sits directly under Timing
instead of down in Shape's row: Timing (170px) + a small gap + Output
(122px), starting right after the header.

Since Output no longer needs to reach all the way down to Shape's row, nothing
does anymore -- the window shrank from 430px to 416px tall to match.

## Timing: Sync is now a checkbox, not a pill

I think this was the real source of "confusing" in Timing specifically. TRIG
and HOST are two pills that mean "pick one of these two modes." Sync used to
be a third pill sitting right next to them, same shape, same size -- so it
read as if there were three mode options, when Sync is actually an
independent on/off modifier that only matters when TRIG is selected.

Sync (and Advanced's "Always snap to grid") are now checkboxes -- a small
square that fills and checks when on, with its own label next to it,
borrowed directly from the `[x] AUTO` checkbox on your Home-Disto reference.
A thin divider line now separates the TRIG/HOST pair from Sync too, so the
grouping is visible, not just implied by spacing.

The rate selector also lost its four-color category coding (cyan for
Straight, purple for Triplet, and so on). That put colors inside the green
Timing card that didn't belong to it and didn't mean "selected" the way
white does anywhere else -- just noise. It's one consistent green now,
matching the card it lives in. The category grouping is still there in the
popup menu when you click it; it just doesn't borrow unrelated colors to do
it.

## Shape thumbnails: selected look reverted

Back to the darker-chip, bright-white-text-and-border treatment you liked
before -- not the solid-white-fill version I switched to two rounds ago. My
reasoning then was plugin-wide consistency, but you're the one who has to
look at it, and this is a curve preview, not a button, so it doesn't need to
follow the same rule buttons do. Only this one component reverted; buttons
elsewhere still follow the white-selected / dark-unselected rule.

## Compile-checked

Same process as every drop this week: JUCE master, all four files at
`-std=c++17 -Wall -Wextra`, then again with `near`/`far` predefined empty.
Clean both times.
