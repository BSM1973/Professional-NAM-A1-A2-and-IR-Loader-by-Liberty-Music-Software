# Liberty Neural Matrix - Guitar Edition v0.5

Liberty Music Software - AU/VST3/Standalone plugin, developed with JUCE 8 and NeuralAmpModelerCore.

## Key additions

- Two guitar-themed amplifier/cabinet images embedded directly in the plugin (no network required).
- Reworked section geometry at 1370x930, 1560x980 and 1860x1200, with strict no-overlap test of 515 component bounds.
- French UI text validated as UTF-8, with an automated mojibake/replacement-character preflight check.
- Actual WAV IR waveform previews from decoded file samples, not decorative synthesized curves.
- Two independent microphone profile selectors: neutral, dynamic 57-style, dynamic 421-style, condenser 87-style, tube 47-style, ribbon 121-style.
- Two interactive XY microphone positioning canvases. Dragging each control adjusts off-axis percentage and distance in centimeters, with DAW-automatable parameters.
- Position and model type drive real DSP filters **after the loaded IR**. This is a tonal approximation, not a physically accurate re-microphoning or endorsed microphone-model emulation.
- Existing NAM A/B, IR A/B, gain/phase, global HP/LP, post-NAM EQ, gate, boost and delay remain supported.

## What simulated microphone positioning means

A captured IR is a recording of a particular microphone and physical position. Moving a virtual microphone cannot reconstruct data that were never captured. Liberty v0.5 therefore uses frequency-colour filtering and proximity approximation, clearly marked as **simulation post-IR**. Authentic multi-mic positioning will require a measured grid of same-cabinet IRs at different placements, with phase-aligned interpolation. Manufacturer and microphone family names are descriptive inspirations; no official partnership or exact microphone emulation is claimed.

## Technical and commercial status

- NAM A1/A2 playback must still be validated with a test suite of actual models; the pinned NeuralAmpModelerCore library uses its A2 optimized path.
- The generated beta is not signed/notarized. GitHub Actions produces preserve-permissions macOS ZIP bundles.
- No TONE3000 OAuth/API, auto phase alignment, true multi-position IR interpolation, multi-band EQ or preset browser yet.
- NAM and IR loading still happen from the editor; a worker-thread loading system remains a future requirement.
- JUCE commercial/AGPL licensing and NAM Core MIT notices must be respected before distribution.
- Only release a build as tested once both GitHub CI jobs pass and runtime audio is manually validated.

## Reproduce checks and build

```bash
python3 scripts/preflight.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
```

## UI testing

1. Change between AMPLI, BAFFLES / IR, EFFETS and RÉGLAGES at minimum and maximum window size.
2. Verify all accent-bearing labels (É, È, ê, à, ç) are correct with no corrupted symbols.
3. Load a real A1/A2 NAM and two known-good WAV IRs; verify both waveforms reflect the chosen recordings.
4. Change mic A and B families and drag each position. Listen to the actual tonal change; verify preset-state reload.
5. Confirm all knobs, popups and files remain readable and non-overlapping in both AU and VST3 hosts.
