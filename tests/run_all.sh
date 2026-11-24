#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"
if [ ! -f "$build/CMakeCache.txt" ]; then
    cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release > /dev/null || exit 100
fi
cmake --build "$build" > /dev/null || { cmake --build "$build"; exit 101; }

suites="test_programs test_errors test_codegen test_module_checks test_editor test_website test_cli test_parallel_print test_https"
if [ "$host" == windows ]; then
    suites="$suites test_unicode test_dll test_cmake"
else
    suites="$suites test_gui test_filesystem"
fi
total=0
for suite in $suites; do
    echo "== $suite"
    RZC="$rzc" bash "$root/tests/$suite.sh"
    total=$((total + $?))
done
echo "== test_programs with -O"
RZC_FLAGS=-O bash "$root/tests/test_programs.sh"
total=$((total + $?))
if [ "$host" == windows ]; then
    echo "== installer"
    "$build/installer/test_path_list.exe"
    total=$((total + $?))
fi
echo "== $total failure(s)"
exit $total
