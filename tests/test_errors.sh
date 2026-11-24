#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"

failures=0
for source in "$root"/tests/errors/*.rz; do
    name="$(basename "$source" .rz)"
    if ! fits_host "$source"; then
        echo "skip $name"
        continue
    fi
    expected="$(sed -n 's|^// error: ||p' "$source" | head -n 1)"
    location="${expected%%: *}"
    message="${expected#*: }"
    output="$("$rzc" "$source" -S -o "$work/error_case$exe" 2>&1)"
    status=$?
    if [ "$status" -eq 1 ] && [[ "$output" == *":$location: error: $message"* ]]; then
        echo "ok   $name"
    else
        echo "FAIL $name: exit code $status"
        echo "     expected: $location: error: $message"
        echo "     got:      $output"
        failures=$((failures + 1))
    fi
done
for source in "$root"/tests/errors_multiple/*.rz; do
    name="$(basename "$source" .rz)"
    output="$("$rzc" "$source" --check 2>&1 | tr -d '\r' | sed 's|^.*errors_multiple/||')"
    status=$?
    if [ "$output" == "$(cat "${source%.rz}.expected")" ]; then
        echo "ok   all errors: $name"
    else
        echo "FAIL all errors: $name"
        diff <(cat "${source%.rz}.expected") <(printf '%s
' "$output") | head -20
        failures=$((failures + 1))
    fi
done
exit $failures
