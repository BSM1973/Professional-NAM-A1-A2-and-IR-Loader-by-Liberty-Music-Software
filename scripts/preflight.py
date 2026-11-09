#!/usr/bin/env python3
"""Cheap local checks before native compilation."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
required = [
    "CMakeLists.txt", "README.md", ".github/workflows/build.yml",
    "Source/PluginProcessor.h", "Source/PluginProcessor.cpp",
    "Source/PluginEditor.h", "Source/PluginEditor.cpp",
    "docs/ROADMAP.md",
]
errors = []
for name in required:
    try:
        contents = (root / name).read_text(encoding="utf-8")
        if not contents.strip():
            errors.append(f"Empty: {name}")
    except (OSError, UnicodeError) as e:
        errors.append(f"Missing or invalid UTF-8: {name}: {e}")
if not errors:
    cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    code = (root / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")
    for word in ("NAM_ENABLE_A2_FAST", "GIT_TAG v0.5.4", "GIT_TAG 8.0.10", "AU VST3 Standalone"):
        if word not in cmake:
            errors.append(f"CMake token missing: {word}")
    for word in ("nam::get_dsp", "loadImpulseResponse", "getStateInformation"):
        if word not in code:
            errors.append(f"Audio implementation token missing: {word}")
if errors:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)
print("Preflight OK (no actual compilation or DSP verification performed).")
