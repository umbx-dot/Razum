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

if "$rzc" "$root/tests/dll/lifecycle/plugin.rz" --dll -O -o "$work/lifecycle.dll" &&
   "$rzc" "$root/tests/dll/reject/plugin.rz" --dll -o "$work/reject_load.dll"; then
    if command -v gcc > /dev/null; then
        if gcc -Wall -Wextra -Werror "$root/tests/dll/lifecycle_client.c" -o "$work/lifecycle_dynamic.exe" &&
           (cd "$work" && ./lifecycle_dynamic.exe); then
            echo "ok   DLL process and thread notifications and failed attach"
        else
            echo "FAIL DLL lifecycle"
            failures=$((failures + 1))
        fi
    fi
    if command -v g++ > /dev/null && command -v dlltool > /dev/null; then
        if dlltool -d "$work/lifecycle.def" -l "$work/lifecycle.a" &&
           g++ -Wall -Wextra -Werror -DLINK_LIFECYCLE -I"$work" "$root/tests/dll/lifecycle_client.c" "$work/lifecycle.a" -o "$work/lifecycle_static.exe" &&
           (cd "$work" && ./lifecycle_static.exe); then
            echo "ok   C++ imports DLL data and callbacks through a generated import library"
        else
            echo "FAIL C++ DLL import library"
            failures=$((failures + 1))
        fi
    fi
else
    echo "FAIL lifecycle DLL compilation"
    failures=$((failures + 1))
fi

if output="$(printf '%s\n' 'int DllMain() { return 1; }' | "$rzc" "$work/bad.rz" --stdin --dll --check 2>&1)" ||
   [[ "$output" != *'DllMain must be declared as'* ]]; then
    echo "FAIL invalid DLL entry point: $output"
    failures=$((failures + 1))
else
    echo "ok   invalid DllMain signature is rejected"
fi
exit $failures
