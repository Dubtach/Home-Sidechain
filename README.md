# Home-Sidechain

Home-Sidechain is a two-plugin creative sidechain system for the Dubtach Home series.

## Plugins

### Home-Sidechain Trigger

The Trigger can create trigger events from:

- Audio level/transient detection
- Incoming MIDI notes
- Both sources
- Smart mode, which prefers a matching MIDI note over audio detection within the same block to avoid duplicate hits

It sends its trigger to the paired Receiver through **Home-Link** automatically and can also expose MIDI output for advanced DAW routing.

### Home-Sidechain Receiver

The Receiver listens for the selected Home-Link and applies a zero-lookahead creative ducking envelope. It also supports incoming MIDI as an optional advanced fallback.

Modes:

- Duck
- Pump
- Gate
- Shape

Timing:

- Free timing
- Tempo sync
- 1/4, 1/2, 1, 2, and 4 bar cycles
- Offset

The shaper supports draggable points plus Reset, Flip, Smooth, and Snap operations.

## Normal workflow

No DAW MIDI send is required:

```text
Kick track
  -> Home-Sidechain Trigger (Link A)

Bass track
  -> Home-Sidechain Receiver (Link A, Source = Home-Link)
```

## Home-Link transport

Trigger and Receiver are separate plugin binaries, so they cannot share a C++ static object directly. Home-Link therefore uses a localhost UDP transport keyed to the current DAW process ID. Socket I/O is kept off the audio thread.

The Receiver maintains a small lock-free event ring per Link. Receiver-local sequence numbers avoid collisions when multiple Trigger instances share the same Link.

Home-Link does not add an intentional audio delay or report plugin latency. Actual cross-track trigger timing is still subject to host processing order and thread scheduling.
