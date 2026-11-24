#!/usr/bin/env bash
set -eu

source "$(dirname "$0")/environment.sh"
folder="$(mktemp -d)"
other="$(mktemp -d /dev/shm/razum-copy.XXXXXX)"
trap 'rm -rf "$folder" "$other"' EXIT

"$rzc" "$root/tests/filesystem/replace.rz" -O -o "$folder/replace"
cd "$folder"

printf original > source
ln source hardlink
ln -s source symlink
if ./replace copy source hardlink || ./replace copy source symlink; then
    exit 1
fi
[ "$(cat source)" = original ]
[ "$(cat hardlink)" = original ]
[ -L symlink ]

dd if=/dev/zero of=large bs=8192 count=1 status=none
printf previous > destination
if (trap '' XFSZ; ulimit -f 1; ./replace copy large destination); then
    exit 1
fi
[ "$(cat destination)" = previous ]
[ -z "$(find . -name '.razum-copy-*' -print)" ]

chmod 600 source
ln -s "$folder/destination" "$other/moved"
./replace move source "$other/moved"
[ ! -e source ]
[ ! -L "$other/moved" ]
[ "$(cat "$other/moved")" = original ]
[ "$(stat -c %a "$other/moved")" = 600 ]
[ "$(cat destination)" = previous ]

echo "ok   filesystem replacement preserves files on failure and replaces destination links"
