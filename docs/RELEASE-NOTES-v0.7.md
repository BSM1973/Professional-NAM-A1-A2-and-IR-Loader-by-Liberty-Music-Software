# Liberty Neural Matrix 0.7 — Automatic visual amp catalog

## Added
- Automatic text-based amp visual recognition (18 families + neutral).
- Reads descriptive NAM metadata fields from bounded file segments (max 320 KiB),
  not model weights, and uses file/folder names as hints.
- Independent amp portraits for slots A/B, with structurally distinct combo/head
  and chassis/speaker/faceplate renders for different families.
- Per-file manual family override + optional local JPG/PNG photo association.
- Global user JSON mapping and project-state persistence with backwards compatibility.
- Confidence/source tooltips and no invented recognition when ambiguous.
- 22 standalone detector checks and 565 minimum/default/max geometry bounds checks.

## Unchanged
- NeuralAmpModelerCore, DSP flow, fixed sample rates, cab convolution, AU/VST3/Standalone.
- No third-party amplifier logos, images, or unauthorised online scraping.

## Validation
- The detector tests and Python preflight have passed locally.
- Full JUCE builds and visual UI checks must still pass GitHub Actions and user testing.
