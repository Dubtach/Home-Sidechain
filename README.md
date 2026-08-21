# Home-Sidechain

Home-Sidechain is a two-plugin sidechain system for the Dubtach Home series:

- **Home-Sidechain Trigger** — detects audio peaks and sends short trigger events.
- **Home-Sidechain Receiver** — receives those events through **Home-Link** without requiring DAW MIDI routing, then applies the shaper envelope.

## Basic setup

1. Put **Home-Sidechain Trigger** on the source track (for example Kick).
2. Put **Home-Sidechain Receiver** on the track to duck (for example Bass).
3. Set the same Link (A–H) on both plugins.
4. Leave the Receiver source set to **HOME-LINK**.
5. No REAPER MIDI send/routing is required.

### Advanced MIDI mode

The Receiver also supports **MIDI**, or **BOTH**, for users who want host MIDI routing as a fallback/advanced workflow.

## Home-Link behavior

The Trigger and Receiver communicate through a per-DAW-process local link bus. Trigger instances publish heartbeats and trigger events; Receiver instances listen only to the selected Link. The bus is local to the current DAW process and stored in the user's temporary directory.
