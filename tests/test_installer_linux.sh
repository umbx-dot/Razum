#!/usr/bin/env bash
set -eu

installer="$(realpath "$1")"
folder="$(mktemp -d)"
trap 'rm -rf "$folder"' EXIT
export HOME="$folder/home"
mkdir "$HOME"
cd "$folder"

prefix='relative $prefix'
bash "$installer" --quiet --no-vscode --prefix "$prefix"
"$folder/$prefix/bin/rzc" --help > /dev/null
source "$HOME/.profile"
[ "$(command -v rzc)" = "$folder/$prefix/bin/rzc" ]
bash "$installer" --quiet --prefix "$prefix" --uninstall
[ ! -L "$folder/$prefix/bin/rzc" ]
echo "ok   relative installer prefix installs a runnable compiler and uninstalls cleanly"
