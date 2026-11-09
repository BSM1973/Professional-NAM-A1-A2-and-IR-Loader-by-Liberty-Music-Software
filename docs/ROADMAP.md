# Liberty Neural Matrix — Roadmap
## B0 — First reproducible build
Build AU/VST3/standalone on macOS ARM64 and VST3/standalone on Windows x64.
Load and audition real NAM A1 and A2 Full/Lite files and mono/stereo WAV IRs.
Verify audio at native model rate, DAW recall, silence and odd-sized host buffers.

## B1 — Audio reliability
Move model and IR preparation off the UI thread, remove real-time locks, provide click-free switches.
Handle model/native sample-rate mismatch, channel formats, allocation tracking, latency and gain matching.

## B2 — Premium IR engine
Eight channels, polarity, phase alignment, micro-delay, per-IR filtering, spectral previews and export.

## B3 — Tone design
Routing matrix, presets, snapshots, MIDI remote, gate, EQ, drive, compressor, ambience and accurate gain staging.

## B4 — Browser and TONE3000
Local indexing, metadata, search, tags, favorites, duplicates, missing-file recovery.
Integrate TONE3000 OAuth only after receiving commercial approval and implement offline cache.

## B5 — QA and commercial release
Objective CPU/latency comparison on specified hardware, host validation and installers.
Verify rights for all model libraries; JUCE commercial licensing and third-party MIT notices.

## Nonnegotiable interface rules
1. No overlapping labels, buttons, menus, faders or other interactive elements.
2. Correct French Unicode glyphs and accented letters, no substitute symbols.
3. Batch changes and run one CI build per validated milestone.
