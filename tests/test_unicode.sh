#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"

if ! "$rzc" "$root/tests/unicode/console_check.rz" -o "$work/console_check$exe" 2> "$work/console_check.log"; then
    echo "FAIL unicode console: rzc failed: $(cat "$work/console_check.log")"
    exit 1
fi
output="$("$work/console_check$exe" "Grüße 世界 🌍" | tr -d '\r')"
expected="console output ok
console input ok
argument ok
Grüße 世界 🌍"
if [ "$output" == "$expected" ]; then
    echo "ok   unicode console output, console input and arguments"
    exit 0
fi
echo "FAIL unicode console:"
printf '%s\n' "$output"
exit 1
