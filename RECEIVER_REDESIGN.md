# Home-Sidechain Receiver – v41 Shaper UI

The Receiver is now a curve-first volume shaper inspired by the workflow of modern ducking/shaper plugins, while keeping the Home-Sidechain cyan/violet/green theme.

## UI
- Trigger-style Home header with A/B/C routing and bypass.
- Large editable shaper graph.
- Main circular MIX control.
- Dual-handle LOW CUT / HIGH CUT filter strip beneath MIX.
- SYNC + 1/8, 1/4, 1/2, 1/1 rate controls below the graph.
- 12 preset curve thumbnails below the rate row.
- TEST and RESET controls.

## Graph editing
- Main nodes are draggable.
- Double-click empty space creates a new node, up to eight total.
- Small handle points between nodes bend the connecting curves.
- Double-clicking empty space near a handle/node leaves it untouched, avoiding accidental edits.

## Processing
- Audio and MIDI/Home-Link triggers are accepted together.
- Sync mode derives cycle length from host BPM.
- Free mode uses the hidden LENGTH parameter for compatibility/automation.
- DEPTH controls maximum ducking.
- MIX blends processed and dry audio.
- LOW_CUT / HIGH_CUT form a post-filter band around the ducked signal.
- BAND / CROSSOVER remain available for compatibility and frequency-focused ducking.

## Compatibility
Legacy SHAPE_1..5 parameters are retained and migrated into the new point model when an older state is loaded.
