#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"
cd "$root"

failures=0

check_program() {
    local name="$1" source="$2" expected_file="$3" input_file="$4" expected_exit="$5"
    if ! fits_host "$source"; then
        echo "skip $name (needs $(sed -n 's|^// platform: ||p' "$source" | head -n 1))"
        return
    fi
    if ! "$rzc" ${RZC_FLAGS:-} "$source" -o "$work/$name$exe" 2> "$work/$name.log"; then
        echo "FAIL $name: rzc failed: $(cat "$work/$name.log")"
        failures=$((failures + 1))
        return
    fi
    local output status
    if [ -f "$input_file" ]; then
        output="$("$work/$name$exe" < "$input_file")"
    else
        output="$("$work/$name$exe" < /dev/null)"
    fi
    status=$?
    output="${output//$'\r'/}"
    local expected
    expected="$(cat "$expected_file")"
    if [ "$output" == "$expected" ] && [ "$status" == "$expected_exit" ]; then
        echo "ok   $name"
    else
        echo "FAIL $name: exit code $status, expected $expected_exit"
        diff <(printf '%s\n' "$expected") <(printf '%s\n' "$output") | head -20
        failures=$((failures + 1))
    fi
}

for source in "$root"/tests/programs/*.rz; do
    name="$(basename "$source" .rz)"
    expected_exit="$(sed -n 's|^// exit: ||p' "$source" | head -n 1)"
    check_program "$name" "$source" "$root/tests/programs/$name.expected" \
        "$root/tests/programs/$name.input" "${expected_exit:-0}"
done

check_program "modules" "$root/tests/modules/app.rz" "$root/tests/modules/app.expected" "" 0

exit $failures
