#!/usr/bin/env bash
set -u

root="$(cd "$(dirname "$0")/.." && pwd)"
if ! command -v node > /dev/null; then
    echo "skip website docs: node is not installed"
    exit 0
fi
cd "$root" && node tests/website/test_docs.js && node tests/website/test_benchmarks.js
