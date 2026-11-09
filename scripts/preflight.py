#!/usr/bin/env python3
"""No-network, no-compilation checks before starting a paid CI build."""
from pathlib import Path
import sys
import base64
import re
root = Path(__file__).resolve().parents[1]
paths = [
    'CMakeLists.txt',
    'Source/PluginProcessor.h', 'Source/PluginProcessor.cpp',
    'Source/PluginEditor.h', 'Source/PluginEditor.cpp', 'Source/ArtAssets.h',
    'Source/TextUtf8.h', 'Source/AmpVisualDetector.h', 'Source/AmpArtwork.h',
    'tests/nam_registry_smoke.cpp', 'tests/amp_visual_detect_test.cpp',
    '.github/workflows/build.yml', 'README.md', 'docs/ROADMAP.md',
]
errors=[]
for name in paths:
    p=root/name
    if not p.is_file():
        errors.append(f'Missing: {name}')
        continue
    try:
        p.read_text(encoding='utf-8')
    except UnicodeError:
        errors.append(f'Not UTF-8: {name}')
cmake = (root/'CMakeLists.txt').read_text(encoding='utf-8')
processor = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
workflow = (root/'.github/workflows/build.yml').read_text(encoding='utf-8')
checks = {
    'NAM A2 core pinned': 'GIT_TAG v0.5.4' in cmake and 'NAM_ENABLE_A2_FAST' in cmake,
    'JUCE pinned': 'GIT_TAG 8.0.10' in cmake,
    'AU and VST3 targets': 'AU VST3 Standalone' in cmake,
    'Manual CI and one-time bootstrap': 'workflow_dispatch' in workflow and 'push:' in workflow and '.github/workflows/build.yml' in workflow,
    'Apple Silicon runner': 'macos-15' in workflow,
    'Windows x64 runner': 'windows-2022' in workflow,
    'Two neural slots': 'std::array<std::unique_ptr<nam::DSP>, 2>' in (root/'Source/PluginProcessor.h').read_text(encoding='utf-8'),
    'Model loading': 'nam::get_dsp(nativePath)' in processor,
    'WAV convolution': 'loadImpulseResponse' in processor,
    'Source-state persistence': 'getStateInformation' in processor and 'setStateInformation' in processor,
    'Mic profile DSP': 'micAType' in processor and 'colourMic' in processor and 'micBOffset' in processor,
    'Real IR preview': 'irWaveform' in processor and 'AudioFormatReader' in processor,
    'Functional mic drag UI': 'MicPositionCanvas' in (root/'Source/PluginEditor.cpp').read_text(encoding='utf-8'),
    'Automatic amp recognition': 'libertyAmp::detect' in processor and 'extractModelMetadata' in processor,
    'Visual override saved': 'storeSavedVisual' in processor and 'visualFamily' in processor,
    'Photograph override': 'selectAmpPhoto' in (root/'Source/PluginEditor.cpp').read_text(encoding='utf-8'),
    'Automated detector tests': 'Amp_Visual_Detection' in cmake,
    'Visual family artwork': 'libertyArtwork::paint' in (root/'Source/PluginEditor.cpp').read_text(encoding='utf-8'),
}
for name, ok in checks.items():
    print(('OK' if ok else 'FAIL') + ' - ' + name)
    if not ok: errors.append(name)
# MSVC rejects single overly long C++ string literals (error C2026).
# Check each independently stored segment and verify round-trip JPEG integrity.
art = (root / 'Source/ArtAssets.h').read_text(encoding='utf-8')
asset_pattern = re.compile(r'inline constexpr std::array<const char\*, (\d+)> (ampJpegChunks|cabJpegChunks) = \{(.*?)\};', re.S)
assets = {match.group(2): match for match in asset_pattern.finditer(art)}
for asset_name in ('ampJpegChunks', 'cabJpegChunks'):
    match = assets.get(asset_name)
    if match is None:
        errors.append(f'Missing segmented artwork: {asset_name}')
        continue
    chunks = re.findall(r'"([A-Za-z0-9+/=]+)"', match.group(3))
    if len(chunks) != int(match.group(1)) or not chunks or max(map(len, chunks)) > 3072:
        errors.append(f'MSVC C2026 risk or inconsistent segment count: {asset_name}')
        continue
    try:
        image_bytes = base64.b64decode(''.join(chunks), validate=True)
        if not (image_bytes.startswith(b'\xff\xd8') and image_bytes.endswith(b'\xff\xd9')):
            errors.append(f'Invalid JPEG data: {asset_name}')
    except (ValueError, base64.binascii.Error):
        errors.append(f'Invalid Base64: {asset_name}')

# Explicit JUCE colour construction: juce::Colour(uint32) is explicit in JUCE 8.
# Reject list-initialisation of Colour arrays from integer literals (fails Clang/MSVC).
artwork = (root / 'Source/AmpArtwork.h').read_text(encoding='utf-8')
for palette in ('chassis', 'faces'):
    palette_match = re.search(r'const std::array<juce::Colour, 19> ' + palette + r'\s*\{\{(.*?)\}\};', artwork, re.S)
    if palette_match is None or len(re.findall(r'juce::Colour\(0x[0-9a-fA-F]{8}\)', palette_match.group(1))) != 19:
        errors.append('JUCE explicit colour constructors missing: ' + palette)

# Check common mojibake and replacement glyphs, without scanning Base64 image resources.
for name in ['Source/PluginEditor.cpp', 'Source/PluginEditor.h',
             'Source/PluginProcessor.cpp', 'Source/PluginProcessor.h']:
    content = (root / name).read_text(encoding='utf-8')
    for pattern in ['\ufffd', 'Ã©', 'Ã¨', 'Ãª', 'Ã ', 'â€™', 'â€œ', 'â€', '\u25a1']:
        if pattern in content:
            errors.append(f'Mojibake in {name}: {pattern!r}')
# New French-text invariant: UTF-8 literal bytes must be converted with
# libertyText::u8 before JUCE receives them. This catches future regressions.
for path in sorted((root / 'Source').rglob('*.cpp')) + sorted((root / 'Source').rglob('*.h')):
    name = str(path.relative_to(root))
    if name == 'Source/ArtAssets.h':
        continue
    content = path.read_text(encoding='utf-8')
    for match in re.finditer(r'"(?:\\.|[^"\\])*"', content):
        token = match.group(0)
        if any(ord(c) > 127 for c in token):
            if not re.search(r'libertyText::u8\s*\(\s*$', content[max(0, match.start()-65):match.start()]):
                errors.append(f'Raw non-ASCII GUI literal in {name}: {token[:80]}')
if 'libertyText::u8' not in (root / 'Source/PluginEditor.cpp').read_text(encoding='utf-8'):
    errors.append('UTF-8 wrapper missing in editor')
if '$<LINK_LIBRARY:WHOLE_ARCHIVE,liberty_nam_core>' not in cmake:
    errors.append('NAM architectures will not register: WHOLE_ARCHIVE missing')
if 'NAM_Registries' not in cmake:
    errors.append('NAM registry smoke test missing')

if errors:
    print('\nPRE-FLIGHT FAILED: ' + ', '.join(errors), file=sys.stderr)
    sys.exit(1)
# Independent mathematical layout audit, no JUCE binaries required.
import subprocess
subprocess.run([sys.executable, str(root / 'scripts/check_geometry.py')], check=True)
print('\nPRE-FLIGHT PASSED. NOTE: this does not compile or run DSP.')
