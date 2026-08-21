# Home-Sidechain

Home-Sidechain is a two-plugin sidechain modulation system for the Dubtach Home plugin series.

## Plugins

- **Home-Sidechain Trigger** — detects audio peaks and publishes Home-Link trigger events. It also accepts incoming MIDI automatically, so users can either feed audio transients or draw MIDI notes in the piano roll without switching modes. It continues to output MIDI for advanced routing.
- **Home-Sidechain Receiver** — listens for Home-Link events and applies the ducking / pumping / gate / shape envelope. A **TEST** button can fire the receiver envelope without needing playback.

## Simple setup — Home-Link

The normal user does **not** need to create a REAPER MIDI send.

1. Put **Home-Sidechain Trigger** on the source/kick track.
2. Choose a **LINK**, for example `A`.
3. Put **Home-Sidechain Receiver** on the destination/bass track.
4. Choose the same **LINK**, for example `A`.
5. Leave Receiver **SOURCE = HOME-LINK**.
6. Press play.

That is all.

The signal path is:

    Kick track
      -> Home-Sidechain Trigger
      -> Home-Link
      -> Home-Sidechain Receiver
      -> Bass track

The Receiver status should show:

- `WAITING` — no matching Trigger heartbeat is present.
- `LINKED` — a Trigger is active on the selected Link.
- `HOME-LINK SIGNAL` — a trigger event has arrived.

## Advanced MIDI fallback

Receiver `SOURCE` also has:

- `MIDI` — use normal DAW MIDI routing only.
- `BOTH` — accept Home-Link and normal host MIDI.

Trigger continues to expose its MIDI output, so users who want conventional MIDI routing can still use it.

## Links

Eight independent links are available:

`A B C D E F G H`

A Trigger and Receiver must use the same Link.

Each Link maps to a MIDI note for advanced routing:

- A = C2 / MIDI note 36
- B = C#2 / MIDI note 37
- C = D2 / MIDI note 38
- D = D#2 / MIDI note 39
- E = E2 / MIDI note 40
- F = F2 / MIDI note 41
- G = F#2 / MIDI note 42
- H = G2 / MIDI note 43

## Home-Link implementation

Home-Link uses localhost UDP packets with one shared transport port derived from the current DAW process ID; the Link ID is carried inside each packet. This removes the previous eight-socket polling path and reduces avoidable inter-plugin transport delay.

Socket I/O is performed on high-priority worker threads. The sender wakes immediately when a trigger is queued, while the Receiver drains the localhost socket in bursts so multiple triggers do not wait behind separate polling passes. Audio processing uses fixed-size queues/rings and does not perform network I/O directly.

The system intentionally adds no lookahead or fixed audio delay. A remaining one-host-block worst case can still occur when a DAW schedules the Receiver before the Trigger in the same processing cycle; that is a host scheduling limitation rather than an internal Home-Link delay.

The current implementation is intended for the VST3/Standalone workflow where the Trigger and Receiver run inside the same DAW process. Hosts that isolate plugin instances in separate processes may require a different transport in a future version.

## Pamplejuce / GitHub Actions

This repository keeps the normal Pamplejuce layout and JUCE git submodule. GitHub Actions builds both plugin targets, runs tests, and pluginval-validates both VST3 products.


## Windows CI compiler note

The GitHub Actions Windows job uses the MSVC compiler (`cl.exe`) with Ninja. The workflow previously forced `clang-cl`, which can fail during JUCE's generated VST3 manifest-helper step with newer CMake/Ninja combinations.


## v7 build fixes
- Updated JUCE parameter construction to the current Attributes API.
- Removed a Trigger Home-Link sequence counter data race.
- Windows CI explicitly uses MSVC without treating warnings as errors.


## Trigger UI

The Trigger editor is built around one large live transient graph. Drag the threshold line directly on the graph. Historical trigger markers are intentionally not drawn; each trigger is shown as a clean live pulse/status flash and the trigger count remains visible in the header. Audio and MIDI trigger sources are automatic; there is no mode selector. MIDI output strength is intentionally not exposed as a control: audio-triggered notes use full velocity for predictable hits, while incoming MIDI notes preserve their incoming velocity. Threshold and Retrigger are the only user-facing detection controls, with Link and Bypass kept in the header.

## v15 Trigger redesign
- Compact 640x360 editor with all controls inside the visible bounds.
- Threshold is the primary interactive control and is edited directly on the graph.
- Graph history is reduced to keep transients visually readable.
- Only six recent trigger markers are shown; the newest trigger is highlighted separately.
- Header communicates READY/TRIGGERING, Audio+MIDI smart input, Link, and trigger count without clutter.
- Trigger output behavior remains compatible with the v12 processing checkpoint.


## v17.2.0

Build-stability cleanup for the custom Trigger Cool Down slider: uses Slider minimum/maximum directly with explicit typed clamping, avoiding Range API/template deduction edge cases on MSVC.
