# Receiver v40 redesign

This revision shifts the Receiver from a conventional dynamics panel to a curve-based volume shaper workflow.

- Main interaction: large editable envelope graph.
- 16 curve presets shown as miniature curve thumbnails.
- Large Mix control.
- Optional tempo sync with 1/8, 1/4, 1/2 and 1/1 rates.
- Full-band, low-band and high-band ducking with adjustable crossover.
- Home-Link and MIDI are both accepted without a source switch.
- Home-series Trigger visual language: dark chassis, cyan waveform/curve, violet secondary accents and compact utility controls.

The core envelope remains sample-based and the graph is only a UI representation; no audio-thread allocations or blocking UI operations were introduced by the editor.
