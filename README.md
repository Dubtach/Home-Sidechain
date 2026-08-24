# Home-Sidechain Trigger v27.3

Trigger UI rebuilt to closely match the approved Home-Sidechain Trigger mockup.

Visuals: compact Home-series header, A/B/C link selector, bypass switch, READY/TRIGGERING graph header, 3-second analysis graph with threshold badge, red trigger region, and full-width Cool Down control.

Core trigger/audio/MIDI/Home-Link processing remains unchanged from the v27.2 checkpoint.


## v27.6 pluginval editor stability
- Replaced the zero-length/null dash array passed to `Graphics::drawDashedLine()` with a valid two-element dash pattern. Some JUCE builds assert on a null dash array during editor painting, which can make pluginval fail at the Editor test.
- Kept the mockup-matching UI and audio/trigger behavior unchanged.
