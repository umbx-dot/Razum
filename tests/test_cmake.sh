#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"
work="$work/mixed_project"

if ! command -v g++ > /dev/null || ! command -v cmake > /dev/null; then
    echo "skip mixed C/C++/razum CMake project: g++ or cmake not found"
    exit 0
fi
rm -rf "$work"
if ! cmake -S "$root/examples/mixed_project" -B "$work" -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
        -DRazum_DIR="$root/cmake/package" -DRAZUM_COMPILER="$rzc" > "$work.log" 2>&1 ||
   ! cmake --build "$work" >> "$work.log" 2>&1; then
    echo "FAIL mixed project build:"
    tail -20 "$work.log"
    exit 1
fi
output="$("$work/app.exe"; "$work/report.exe")"
output="${output//$'\r'/}"
expected="$(cat "$root/examples/mixed_project/expected_output.txt")"
if [ "$output" == "$expected" ]; then
    echo "ok   mixed C/C++/razum CMake project"
    exit 0
fi
echo "FAIL mixed project output"
diff <(printf '%s\n' "$expected") <(printf '%s\n' "$output")
exit 1
