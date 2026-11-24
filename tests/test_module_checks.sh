#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"

skipped='/libraries/linux/|/linux/'
if [ "$host" == linux ]; then
    skipped='/libraries/(windows|winsock)/|/windows/'
fi
failures=0
count=0
while IFS= read -r file; do
    count=$((count + 1))
    output="$("$rzc" "$file" -I "$root/tests/modules" --check --stdin < "$file" 2>&1)"
    status=$?
    if [ -n "$output" ] || [ "$status" != 0 ]; then
        echo "FAIL $file: $output"
        failures=$((failures + 1))
    fi
done < <(find "$root/libraries" "$root/tests/modules" -name '*.rz' | sort | grep -vE "$skipped")
if [ "$count" -lt 100 ]; then
    echo "FAIL only $count library and module files were found; is a non-POSIX find first on PATH?"
    failures=$((failures + 1))
elif [ "$failures" == 0 ]; then
    echo "ok   $count library and module files check without a main"
fi
exit $failures
