#!/usr/bin/env bash
set -euo pipefail

output="${1:?usage: make_setup.sh OUTPUT RZC CMAKE_PACKAGE [NASM NASM_LICENSE] [VSIX EXTENSION_ID]}"
compiler="$2"
package="$3"
nasm="${4:-}"
license="${5:-}"
vsix="${6:-}"
extension_id="${7:-}"
here="$(cd "$(dirname "$0")" && pwd)"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
cp "$compiler" "$stage/rzc"
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
