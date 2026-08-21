# Home-Sidechain

Home-Sidechain is a two-plugin MIDI-linked sidechain system for the Dubtach Home plugin series:

- **Home-Sidechain Trigger** detects audio peaks and outputs short MIDI note events.
- **Home-Sidechain Receiver** accepts those MIDI notes and turns them into a sample-processed gain envelope with a draggable shape and bar-synced timing.

## Pamplejuce setup

This repo intentionally expects your normal Pamplejuce layout with `JUCE/` as the git submodule. Keep the Pamplejuce `packaging/`, `Tests/`, `VERSION`, and other template files in your repo and replace the project `CMakeLists.txt` with the provided one.

## Routing

The two instances must be connected by the DAW's MIDI routing:

1. Put **Home-Sidechain Trigger** on the source track (usually the kick).
2. Set its `LINK` to A-H.
3. Route the Trigger's MIDI output to the track containing **Home-Sidechain Receiver**.
4. Set the Receiver to the same `LINK`.
5. Put the Receiver on the audio you want to duck.

Link A-H maps to MIDI notes C2-G2 plus H. The Receiver listens only to MIDI channel 1 and its selected link note.

## Important v1 limitation

Whether an audio/MIDI effect can send MIDI to another effect depends on DAW routing. The current architecture intentionally uses standard host MIDI routing rather than OS-level shared memory, so it remains portable across macOS/Windows and compatible with the Pamplejuce/JUCE model.

## Next additions

- Home-series LicenseManager integration
- Preset system
- More shaper modes
- Trigger MIDI-thru option
- MIDI-note/velocity visualizer
- Optional audio sidechain input on Receiver for hosts that support it


## MIDI routing note

Home-Sidechain Trigger explicitly exposes a VST3 MIDI output and Home-Sidechain Receiver explicitly exposes a VST3 MIDI input. This is required by JUCE's CMake plugin configuration; overriding `producesMidi()`/`acceptsMidi()` in C++ alone does not create the host-facing MIDI bus.
