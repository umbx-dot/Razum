#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")" && pwd)"
build="${RZ_BUILD:-$root/build}"
run_tests=0
prefix=""
installer=0

usage() {
    cat <<'TEXT'
Usage: ./build.sh [--test] [--installer] [--install [PREFIX]]

Builds rzc into ./build (or $RZ_BUILD) with CMake.

  --test               run the test suite afterwards (tests/run_all.sh)
  --installer          also build build/razum-setup.sh, the self-extracting Linux installer
  --install [PREFIX]   copy rzc to PREFIX/bin; PREFIX defaults to ~/.local
  -h, --help           show this text

Needs a C17 compiler (gcc or clang), CMake 3.20+, NASM, and Ninja or make.
  Debian, Ubuntu, Raspberry Pi OS:  sudo apt install build-essential cmake ninja-build nasm
  Fedora:                           sudo dnf install gcc cmake ninja-build nasm
TEXT
}

while [ $# -gt 0 ]; do
    case "$1" in
        --test) run_tests=1 ;;
        --installer) installer=1 ;;
        --install)
            prefix="$HOME/.local"
            if [ $# -gt 1 ] && [ "${2#-}" == "$2" ]; then
                prefix="$2"
                shift
            fi
            ;;
        -h | --help) usage; exit 0 ;;
        *) echo "build.sh: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

missing=""
for tool in cmake nasm; do
    command -v "$tool" > /dev/null || missing="$missing $tool"
done
command -v cc > /dev/null || command -v gcc > /dev/null || command -v clang > /dev/null || missing="$missing cc"
if [ -n "$missing" ]; then
    echo "build.sh: missing:$missing" >&2
    usage >&2
    exit 1
fi
case "$(uname -m)" in
    x86_64 | amd64) ;;
    *) echo "build.sh: rzc itself builds here, but razum programs are x86-64 code and will not run on $(uname -m) yet" >&2 ;;
esac

generator=()
if [ ! -f "$build/CMakeCache.txt" ] && command -v ninja > /dev/null; then
    generator=(-G Ninja)
fi
cmake -S "$root" -B "$build" "${generator[@]}" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build"
echo "built $build/rzc"

if [ "$installer" == 1 ]; then
    cmake --build "$build" --target razum-setup
fi
if [ "$run_tests" == 1 ]; then
    RZ_BUILD="$build" bash "$root/tests/run_all.sh"
fi
if [ -n "$prefix" ]; then
    cmake --install "$build" --prefix "$prefix"
    case ":$PATH:" in
        *":$prefix/bin:"*) ;;
        *) echo "add $prefix/bin to PATH to run rzc from anywhere" ;;
    esac
fi
