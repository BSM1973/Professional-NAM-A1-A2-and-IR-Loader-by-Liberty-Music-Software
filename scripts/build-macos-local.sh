#!/usr/bin/env bash
# Liberty Neural Matrix: compilation locale Apple Silicon sans minutes GitHub Actions.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$root/build-local-macos"
deps_dir="$root/.deps-local-macos"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "Erreur : script réservé à macOS." >&2
  exit 2
fi
if [[ "$(uname -m)" != "arm64" ]]; then
  echo "Erreur : un Mac Apple Silicon est requis pour cette configuration." >&2
  exit 2
fi
if ! command -v cmake >/dev/null; then
  echo "Erreur : CMake est absent. Installer CMake, puis relancer." >&2
  exit 2
fi
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Erreur : outils Xcode manquants. Exécuter : xcode-select --install" >&2
  exit 2
fi

mkdir -p "$deps_dir"
echo '=== Contrôles des sources et de la géométrie ==='
python3 "$root/scripts/preflight.py"

echo '=== Configuration CMake ARM64 ==='
cmake -S "$root" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  "-DFETCHCONTENT_BASE_DIR=$deps_dir"

echo '=== Compilation AU / VST3 / Standalone ==='
cmake --build "$build_dir" --config Release --parallel 3

echo '=== Tests NAM et reconnaissance visuelle ==='
ctest --test-dir "$build_dir" -C Release --output-on-failure

release="$build_dir/LibertyNeuralMatrix_artefacts/Release"
exe="$release/Standalone/Liberty Neural Matrix.app/Contents/MacOS/Liberty Neural Matrix"
test -x "$exe" || { echo 'Erreur : application non produite.' >&2; exit 1; }
test -d "$release/AU/Liberty Neural Matrix.component" || exit 1
test -d "$release/VST3/Liberty Neural Matrix.vst3" || exit 1

package="$build_dir/Liberty-Neural-Matrix-macOS-arm64"
rm -rf "$package"
mkdir -p "$package"
for format in Standalone AU VST3; do
  /usr/bin/ditto "$release/$format" "$package/$format"
done
archive="$build_dir/Liberty-Neural-Matrix-macOS-arm64.zip"
rm -f "$archive"
/usr/bin/ditto -c -k --sequesterRsrc --keepParent "$package" "$archive"
/usr/bin/unzip -tq "$archive"

echo '=== Compilation et tests réussis ==='
echo "Application : $release/Standalone/Liberty Neural Matrix.app"
echo "Archive macOS : $archive"
echo "Note : prototype non signé ni notarisé."
