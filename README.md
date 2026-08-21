# Home-Sidechain

Home-Sidechain is a two-plugin sidechain modulation system for the Dubtach Home plugin series.

## Plugins

- **Home-Sidechain Trigger** — detects audio peaks and publishes Home-Link trigger events. It also outputs MIDI for advanced routing.
- **Home-Sidechain Receiver** — listens for Home-Link events and applies the ducking / pumping / gate / shape envelope.

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

Home-Link uses localhost UDP packets with a port derived from the current DAW process ID and selected Link. This keeps separate DAW processes isolated without requiring the user to create host MIDI sends.

Socket I/O is performed on worker threads. Audio processing uses fixed-size queues/rings and does not perform network I/O directly.

The current implementation is intended for the VST3/Standalone workflow where the Trigger and Receiver run inside the same DAW process. Hosts that isolate plugin instances in separate processes may require a different transport in a future version.

## Pamplejuce / GitHub Actions

This repository keeps the normal Pamplejuce layout and JUCE git submodule. GitHub Actions builds both plugin targets, runs tests, and pluginval-validates both VST3 products.
