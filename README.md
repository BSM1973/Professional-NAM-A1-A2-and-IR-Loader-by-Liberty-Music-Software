# LIBERTY NEURAL MATRIX — v0.2 (prototype)

**Liberty Music Software** — standalone project independent of Liberty DAW and Liberty Rompler.

## Current prototype
- Two **real NAM** A/B loaders using official `NeuralAmpModelerCore` v0.5.4 with A2 fast-path enabled. A1/A2-Full/A2-Lite playback needs real-world validation after compilation.
- Two user-selectable WAV IR slots, JUCE convolution, A/B IR mix.
- Input/output gain, presets restoring referenced file paths, French dark-blue editor.
- Native macOS Apple Silicon: **AU, VST3, standalone**; Windows x64: **VST3, standalone**.
- Reproducible CMake dependencies (JUCE 8.0.10 + NAM core v0.5.4).

## Limitations (do not advertise as complete yet)
- **Not compiled in the preparation environment.** CMake needs remote dependencies.
- NAM is mono-in/mono-out in this first prototype. Stereo input is downmixed to mono before NAM.
- NAM models with a mismatched training sample rate cannot be loaded until proper resampling is added; changing host sample rates requires further validation.
- Loading can briefly block the editor; switching loaded models uses try-lock bypass during a contention window.
- Two IRs only, no automatic phase alignment, advanced IR analysis, true morphing, TONE3000 OAuth or online browsing yet.
- Saving presets stores source file paths; it does not embed or redistribute model files.
- Beta binaries are **unsigned and not notarized**; distribution packaging comes later.

## Build on GitHub — without modifying Liberty DAW

1. This repository hosts the standalone Liberty Neural Matrix sources.\n2. In GitHub open **Actions → Liberty Neural Matrix - AU VST3 standalone** and select **Run workflow**.\n3. After successful builds, download artifacts `neural-matrix-macos-arm64` and `neural-matrix-windows-x64`.\n4. Test VST3/AU in the DAW at **48 kHz** first, then extend the matrix, presets and A2 model tests.

The workflow is **manual only**, avoiding unnecessary builds. It uses `macos-15` (Apple Silicon arm64) and `windows-2022` (x64).

## Local build, after dependencies are available

```bash
python3 scripts/preflight.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
```

## Safety / licensing
- `NeuralAmpModelerCore`: MIT, include upstream notices when redistributing.
- JUCE 8: dual AGPL/commercial; verify Liberty's commercial licence before shipping closed-source binaries.
- TONE3000 API integration requires permission for commercial products. No integration or partnership is currently claimed.
- Never redistribute user-downloaded NAM models and IRs without their owners' rights/permissions.

**Status:** Experimental source. No successful build, notarization or TONE3000 integration is claimed.