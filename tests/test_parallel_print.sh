#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"

if ! "$rzc" "$root/tests/threads/parallel_print.rz" -o "$work/parallel_print$exe" 2> "$work/parallel_print.log"; then
    echo "FAIL parallel_print: rzc failed: $(cat "$work/parallel_print.log")"
    exit 1
fi
summary="$("$work/parallel_print$exe" | tr -d '\r' | sort | uniq -c | awk '{print $1 " " $2 " " $3}' | tr '
' ';')"
expected=""
for thread in 0 1 2 3 4 5 6 7; do
    expected="${expected}5000 thread ${thread};"
done
if [ "$summary" == "$expected" ]; then
    echo "ok   8 threads printed 40000 whole lines"
    exit 0
fi
echo "FAIL parallel_print: $summary"
exit 1
