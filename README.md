# Home-Sidechain Trigger

Version 26.3.0

Home-series compact Trigger UI checkpoint.

## v26.3 changes
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


## v26.3 UI checkpoint
- Removed the separate Link + Output utility panel.
- Moved A/B/C link selector into the header beside Bypass.
- Removed MIDI + Home Link output text and Smart Input text from the UI.
- Replaced the graph title with the live READY/TRIGGERING/BYPASSED indicator.
- Reduced editor size to 580 x 310.
- Graph and Cool Down now use the full plugin width.

## v27.0 visual redesign
The Trigger editor now follows the approved Home-Sidechain mockup direction: large compact analysis scope, header Link selector + bypass, READY/TRIGGERING state, threshold badge, cyan analysis accents, red trigger highlight, and full-width Cool Down control. Processing behavior is unchanged.


## UI checkpoint 27.1
Rebuilt Trigger editor to closely match the approved Home-Sidechain Trigger mockup: wide dark chassis, Home-style header, segmented A/B/C link selector, compact bypass switch, large analysis scope with threshold badge and red trigger highlight, and full-width Cool Down panel. Audio processing and Home-Link behavior are unchanged.
