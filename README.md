# Home-Sidechain

Home-Sidechain is a two-plugin sidechain system for the Dubtach Home plugin series.

- **Home-Sidechain Trigger** — detects audio peaks and outputs MIDI trigger notes.
- **Home-Sidechain Receiver** — receives those MIDI notes and applies a tempo/free-time volume-shaping envelope.

## REAPER routing

1. Put **Home-Sidechain Trigger** on the source/kick track.
2. Set Trigger **LINK = A**.
3. Put **Home-Sidechain Receiver** on the destination/bass track.
4. Set Receiver **LINK = A**.
5. Create a send from Trigger to Receiver.
6. Set the send to **Audio: None** and **MIDI: All -> All**.
7. On playback, Trigger should show its meter moving and Receiver should change from **WAITING** to **MIDI IN** / **TRIGGER IN** and show the last MIDI note/channel.

Each link maps to its own MIDI note, allowing multiple Trigger/Receiver pairs in one project.

## Pamplejuce / GitHub Actions

This repository is intended to be used with the normal Pamplejuce layout and JUCE git submodule. GitHub Actions builds both plugin targets and pluginval validates both VST3 products.

## Important implementation details

- Trigger exposes a VST3 MIDI output via `NEEDS_MIDI_OUTPUT TRUE`.
- Receiver exposes a VST3 MIDI input via `NEEDS_MIDI_INPUT TRUE`.
- Receiver accepts the matching MIDI note on any MIDI channel.
- Receiver MIDI events are handled at their actual sample offsets.
- Both processors avoid dynamic MIDI-event storage on the real-time audio path.
