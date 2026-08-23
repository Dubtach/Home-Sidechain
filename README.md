# Home-Sidechain Trigger

Version 26.2.0

Home-series compact Trigger UI checkpoint.

## v26.2 changes
- Header label updated to Home-Sidechain Trigger with DUBTACH DSP subtitle.
- Removed ENGINE ACTIVE/OFF text.
- Graph display uses a tighter -48 dB to 0 dB range.
- Expanded waveform plot inside the same graph card.
- Threshold badge retained and threshold mapping remains sample-accurate.
- Cool Down labels are aligned inside the slider control.
- GUI refresh reduced to 24 Hz to lower idle editor overhead.
- Audio-thread trigger processing avoids per-sample atomic stores and per-sample dB conversion.
- Home-Link sender link selection only updates when the selected link changes.

## Behavior retained
- Automatic audio + MIDI trigger detection.
- No MIDI velocity control.
- Red waveform highlight for the latest audio-triggered event.
- Home-Link + MIDI output.
- Receiver TEST and existing low-latency transport.
