# Home-Sidechain v50 — New button styles, Filters tab, verified end to end

`source/Receiver/*`, `source/Trigger/PluginEditor.{h,cpp}` (just the link
selector), and `source/Shared/HomeSeriesUI.h` changed. Trigger's processor is
untouched.

## Three new button styles

**Link, and Trigger/Host, are now a single connected track**, not N separate
buttons sitting next to each other. One dark pill-shaped bar, divided into
segments, with the active one shown as a white pill that sits inside the
track rather than being its own separate button. This is a new shared
component (`homeUI::SegmentedSwitch`) used in three places: Link (8
segments, header, both plugins), Trigger/Host (2 segments, Timing card), and
the new Shapes/Filters tab (see below).

**The rate stepper is now real arrows.** The "<" / ">" text buttons are gone;
in their place are two small tiles with an actual triangle icon
(`homeUI::ChevronButton`), the same visual language as the reset icon on the
Shapes card. They read as navigation controls now, not text buttons that
happen to contain punctuation.

## Smooth, Low Cut, High Cut are out of Advanced

They're in the Shapes card now, behind a **Filters** tab. The tab switch
sits where the card's title used to be; flip it and the shape thumbnails are
replaced by three compact knobs in the same footprint, flip it back and
you're looking at shapes again. The reset icon only makes sense for shapes,
so it hides itself on the Filters page.

Advanced dropped from 500x270 to 500x180 now that it's down to just the
trigger-source picker and Always Snap -- it was sized for a knob row that
doesn't live there anymore.

## Verified by actually rendering every state, not just the default one

Same offscreen JUCE render pipeline as last round, now checking more ground:
default view, Filters tab open, Sync off (Length knob swap), Host mode
(Sync/Trigger/Host correctly greyed together), the Advanced panel at its new
size, and the Trigger plugin's new link selector. Six real screenshots,
included in `render-verification/` in this drop so you can see exactly what
I checked rather than taking my word for it.

Everything came back clean on inspection -- no overlaps, no clipped text, no
leftover references to the old per-button pills anywhere in either plugin.

## Compile-checked

All four files clean at `-std=c++17 -Wall -Wextra`, and again with
`near`/`far` predefined empty to simulate real Windows conditions.
