#!/usr/bin/env bash
set -u

root="$(cd "$(dirname "$0")/.." && pwd)"
if ! command -v node > /dev/null; then
    echo "skip editor lookups: node is not installed"
    exit 0
fi
cd "$root" && NODE_PATH="$root/tests/editor/stubs" node tests/editor/test_extension.js tests/programs/const_enum.rz
