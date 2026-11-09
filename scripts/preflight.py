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
}
for name, ok in checks.items():
    print(('OK' if ok else 'FAIL') + ' - ' + name)
    if not ok: errors.append(name)
# Guard against MSVC C2026: all image string literals must be short.
# Verify reconstructed Base64 matches complete JPEGs before paying for CI.
art = (root / 'Source/ArtAssets.h').read_text(encoding='utf-8')
asset_pattern = re.compile(r'inline constexpr std::array<const char\*, (\d+)> (ampJpegChunks|cabJpegChunks) = \{(.*?)\};', re.S)
assets = {match.group(2): match for match in asset_pattern.finditer(art)
}
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

# Check common mojibake and replacement glyphs, without scanning Base64 image resources.
for name in ['Source/PluginEditor.cpp', 'Source/PluginEditor.h',
             'Source/PluginProcessor.cpp', 'Source/PluginProcessor.h']:
    content = (root / name).read_text(encoding='utf-8')
    for pattern in ['\ufffd', 'Ã©', 'Ã¨', 'Ãª', 'Ã ', 'â€™', 'â€œ', 'â€', '\u25a1']:
        if pattern in content:
            errors.append(f'Mojibake in {name}: {pattern!r}')
if errors:
    print('\nPRE-FLIGHT FAILED: ' + ', '.join(errors), file=sys.stderr)
    sys.exit(1)
# Independent mathematical layout audit, no JUCE binaries required.
import subprocess
subprocess.run([sys.executable, str(root / 'scripts/check_geometry.py')], check=True)
print('\nPRE-FLIGHT PASSED. NOTE: this does not compile or run DSP.')
