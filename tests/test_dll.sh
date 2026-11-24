#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"
work="$work/dll"
mkdir -p "$work"

failures=0
expected="$(cat "$root/tests/dll/app.expected")"

check_output() {
    local name="$1" program="$2"
    local output
    output="$("$program" | tr -d '\r')"
    if [ "$output" == "$expected" ]; then
        echo "ok   $name"
    else
        echo "FAIL $name"
        diff <(printf '%s\n' "$expected") <(printf '%s\n' "$output") | head -20
        failures=$((failures + 1))
    fi
}

if ! "$rzc" "$root/tests/dll/mathlib.rz" --dll -o "$work/mathlib.dll" 2> "$work/dll.log"; then
    echo "FAIL razum DLL: $(cat "$work/dll.log")"
    exit 1
fi
if "$rzc" "$root/tests/dll/app.rz" -o "$work/app.exe" 2> "$work/app.log"; then
    check_output "razum program calls a razum DLL" "$work/app.exe"
else
    echo "FAIL razum program: $(cat "$work/app.log")"
    failures=$((failures + 1))
fi
if command -v gcc > /dev/null; then
    if gcc -Wall -Wextra -Werror -o "$work/main_c.exe" "$root/tests/dll/main.c" -I"$work" "$work/mathlib.dll" 2> "$work/c.log"; then
        check_output "C program calls a razum DLL through mathlib.h" "$work/main_c.exe"
    else
        echo "FAIL C program: $(cat "$work/c.log")"
        failures=$((failures + 1))
    fi
fi
exit $failures
