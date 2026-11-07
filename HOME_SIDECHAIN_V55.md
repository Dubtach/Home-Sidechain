# Home-Sidechain v55 — final

Changed: `source/Receiver/PluginProcessor.{h,cpp}`, `source/Receiver/PluginEditor.{h,cpp}`,
`source/Trigger/PluginProcessor.cpp`, `source/Trigger/PluginEditor.{h,cpp}`.

## Receiver: Classic is a proper sidechain carve now

It's the default, so it should look like what the plugin is for. The old
Classic started rising the instant the trigger landed. The new one is the
standard duck shape:

1. instant drop to the bottom at the trigger,
2. a short hold there (10% of the cycle) -- the room a kick needs to punch,
3. a release that recovers fast and eases into the top (about 43% back by
   0.2, 78% by 0.5, full by 0.8),
4. a hold at full level,
5. the short fall at the very end that keeps the loop seam click-free.

Fresh instances open on it, and Reset goes back to it.

## Receiver: save your own carve into any of the 7 slots

**Right-click a slot** in the bottom strip:

- **Save current shape to this slot** -- whatever is on the graph replaces
  that slot. (A menu rather than a straight save, so a stray right-click
  can't overwrite a shape.)
- **Restore factory shape** -- only shown on a slot you've saved over, so
  overwriting is never a one-way door.

A saved slot is marked with `*` after its name (same convention Home-Disto
uses for an edited preset) and shows its own thumbnail. Clicking it loads
your carve; Reset still gives you factory Classic unless you've saved over
slot 1.

Custom carves are stored in one file shared by every instance, not in the
project, so they behave like permanent additions to your shape bank:

- Windows: `%APPDATA%\Dubtach\Home-Sidechain\CustomCarves.xml`
- macOS: `~/Library/Application Support/Dubtach/Home-Sidechain/CustomCarves.xml`

An instance that's already open won't see a save made in another instance
until it's reopened.

## Trigger: sending indicator

It's a pill now instead of a dot and a label. Idle, it's a quiet dark pill.
When a trigger is sent it fills solid green with dark text, throws a glow,
and washes the whole graph faintly, for about half a second.

There was also a real reason it was easy to miss: it was driven by a meter
that decays *per audio block*. At small buffer sizes that can fall to zero
between two 30 Hz UI ticks, so a trigger could fire and the lamp would
never see it. It now latches on the processor's trigger **counter**
changing, which can't be missed regardless of buffer size.

## Trigger: default Cool Down 80 ms -> 150 ms

New instances only. Existing projects keep whatever value they saved.
Changing it is one number in `Trigger/PluginProcessor.cpp` if 150 isn't
the right feel.

## How this was checked

Compiled clean at `-Wall -Wextra` with zero warnings, and again with
`near`/`far` defined empty to simulate Windows. Then, against the real
processor and editors (not copies of their logic):

- **Classic curve**, as numbers: duck, hold, fast-then-easing monotonic
  release, unity by 0.8, seam y(1)==y(0), and a fresh instance's default
  parameters equal the preset.
- **Custom carves**, against an isolated home directory: save -> file
  written -> a brand-new instance loads it -> clicking the slot applies it
  exactly -> restore -> persists; neighbouring slots untouched; a garbage
  file and out-of-range / too-short slots are ignored without crashing.
- **Sending pill**, with the editor's real 30 Hz timer: dark at idle, solid
  green after a trigger, fades back, and a burst of 7 triggers between two
  UI ticks is still caught.

Writing those checks caught two mistakes of mine before they shipped: the
Classic release curve's tension was on the wrong node (so it came out
almost linear instead of fast-then-easing), and the first pill design had a
muddy half-lit mid-fade with poor text contrast. Both are fixed.

## What I could not check

I can't run a DAW here. The bypass-overlay repaint fix from the last
round is the same standard JUCE call it was then, and nothing about it has
changed, but it's the one thing in this plugin I've never been able to see
working live. If the overlay still misbehaves in your host, tell me which
host and how you're toggling bypass.
