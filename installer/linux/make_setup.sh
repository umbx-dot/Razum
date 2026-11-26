#!/usr/bin/env bash
set -euo pipefail

output="${1:?usage: make_setup.sh OUTPUT RZC RZ CMAKE_PACKAGE [NASM NASM_LICENSE] [VSIX EXTENSION_ID]}"
compiler="$2"
driver="$3"
package="$4"
nasm="${5:-}"
license="${6:-}"
vsix="${7:-}"
extension_id="${8:-}"
here="$(cd "$(dirname "$0")" && pwd)"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
cp "$compiler" "$stage/rzc"
cp "$driver" "$stage/rz"
cp "$here/../../rz/vendor/tomlc17/LICENSE" "$stage/tomlc17-LICENSE.txt"
cp "$package" "$stage/RazumConfig.cmake"
cp "$here/../../LICENSE" "$stage/LICENSE.txt"
cp "$here/../../docs/linux-gui-notices.md" "$stage/linux-gui-NOTICES.txt"
if [ -n "$nasm" ] && [ -n "$license" ]; then
    cp "$nasm" "$stage/nasm"
    cp "$license" "$stage/nasm-LICENSE"
fi
if [ -n "$vsix" ]; then
    cp "$vsix" "$stage/razum.vsix"
    printf '%s' "$extension_id" > "$stage/extension-id"
fi
cat "$here/setup.sh" > "$output"
tar -czf - -C "$stage" . >> "$output"
chmod 755 "$output"
echo "$output"
