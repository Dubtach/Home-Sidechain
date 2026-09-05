# Home-Sidechain Receiver — redesign checkpoint

The Receiver has been rebuilt around one purpose: receive Home-Link or MIDI trigger events and apply a musical ducking envelope to the input.

UI principles:
- compact Home-series visual treatment
- A/B/C direct link selection in the header
- compact bypass icon + TEST action
- large editable duck envelope graph
- six compact controls: Depth, Attack, Hold, Release, Curve, Mix
- live READY/RECEIVING/BYPASS state
- no source/mode/sync dropdown clutter

Processing principles:
- Receiver listens to Home-Link and MIDI in parallel
- MIDI and Home-Link triggers both start the same envelope
- existing APVTS parameter IDs are retained for project/state compatibility
- legacy SOURCE/MODE/SYNC/BARS parameters are intentionally hidden from the new UI
- envelope timing is based directly on Attack + Hold + Release
