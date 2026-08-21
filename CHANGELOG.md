# Home-Sidechain 1.3.0

## Home-Link
- Replaced localhost UDP transport with process-local shared event bus.
- No socket I/O, worker thread, or wait operation in the audio callback.
- Each Receiver has its own read cursor, allowing multiple receivers on the same link.
- Absolute host sample timestamps are carried with trigger events when the host provides them.

## Trigger
- Smart / Audio / MIDI / Both modes.
- External MIDI note input and generated MIDI output.
- MIDI note selector 0-127.
- Manual TEST trigger.
- Audio threshold + sensitivity + retrigger controls.
- Zero reported plugin latency.

## Receiver
- Home-Link / MIDI / Both source modes.
- Duck / Pump / Gate / Shape.
- Draggable 5-point envelope shaper.
- Reset / Flip / Smooth / Snap.
- Factory creative presets.
- Free / Sync timing and bar lengths 1/4 to 4 bars.
- Attack / Hold / Release / Curve / Depth / Mix / Offset.
- Zero reported plugin latency and no lookahead buffer.

## Important timing note
Home-Sidechain intentionally reports 0 plugin latency and does not add lookahead or buffering. When the host processes Trigger before Receiver, Home-Link timestamps allow the Receiver to place the event at the exact sample in the current block. If a host processes the Receiver before the Trigger, the host's own scheduling order can still push the event to the next audio block; a plugin cannot force the DAW to execute tracks in a different order.
