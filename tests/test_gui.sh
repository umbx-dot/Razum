#!/usr/bin/env bash
set -eu

source "$(dirname "$0")/environment.sh"

"$rzc" "$root/tests/gui/backend.rz" -O -o "$work/gui_backend"
env -u DISPLAY -u WAYLAND_DISPLAY -u WAYLAND_SOCKET timeout 10 "$work/gui_backend" none

if [ -n "${WAYLAND_DISPLAY:-}" ]; then
    result="$(env -u DISPLAY timeout 10 "$work/gui_backend" wayland)"
    [ "$result" = wayland ]
    echo "ok   native Wayland GUI"
else
    [ "${RZ_REQUIRE_GUI:-0}" != 1 ] || { echo "FAIL native Wayland GUI: WAYLAND_DISPLAY is unset"; exit 1; }
    echo "skip native Wayland GUI: WAYLAND_DISPLAY is unset"
fi

case "${DISPLAY:-}" in
    :* | unix:*)
        result="$(env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET timeout 10 "$work/gui_backend" x11)"
        [ "$result" = x11 ]
        echo "ok   X11 GUI"
        ;;
    *)
        [ "${RZ_REQUIRE_GUI:-0}" != 1 ] || { echo "FAIL X11 GUI: no local DISPLAY"; exit 1; }
        echo "skip X11 GUI: no local DISPLAY"
        ;;
esac
