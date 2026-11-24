#!/bin/sh
set -eu

prefix="$HOME/.local"
quiet=0
vscode=1
action=install
while [ $# -gt 0 ]; do
    case "$1" in
        --quiet) quiet=1 ;;
        --no-vscode) vscode=0 ;;
        --uninstall) action=uninstall ;;
        --prefix) shift; prefix="${1:?--prefix needs a folder}" ;;
        -h | --help)
            echo "razum-setup.sh [--quiet] [--prefix FOLDER] [--no-vscode] [--uninstall]"
            echo "Installs razum for this user into FOLDER (default ~/.local): rzc, NASM if the system has none,"
            echo "the CMake package and, with a native VS Code, the editor extension. No root needed."
            exit 0
            ;;
        *) echo "razum-setup.sh: unknown option $1" >&2; exit 2 ;;
    esac
    shift
done

case "$prefix" in
    /*) ;;
    *) prefix="$(pwd -P)/$prefix" ;;
esac

data="$prefix/share/razum"
bin="$prefix/bin"
profile="$HOME/.profile"
quoted_bin=$(printf '%s' "$bin" | sed "s/'/'\\\\''/g")
path_line="export PATH='$quoted_bin':\"\$PATH\""

ask() {
    [ "$quiet" = 1 ] && return 0
    printf '%s [Y/n] ' "$1"
    read -r answer || answer=y
    case "$answer" in
        n* | N*) return 1 ;;
        *) return 0 ;;
    esac
}

native_code() {
    command -v code > /dev/null 2>&1 || return 1
    case "$(command -v code)" in
        /mnt/*) return 1 ;;
    esac
    return 0
}

if [ "$action" = uninstall ]; then
    if [ ! -f "$data/installed" ]; then
        echo "razum is not installed in $prefix"
        exit 1
    fi
    while IFS= read -r line; do
        case "$line" in
            link:*) rm -f "${line#link:}" ;;
            path:*)
                if [ -f "$profile" ]; then
                    grep -vxF "${line#path:}" "$profile" > "$profile.razum" || true
                    mv "$profile.razum" "$profile"
                fi
                ;;
            vscode:*) native_code && code --uninstall-extension "${line#vscode:}" > /dev/null 2>&1 || true ;;
        esac
    done < "$data/installed"
    rm -rf "$data"
    echo "razum removed from $prefix"
    exit 0
fi

ask "Install razum into $prefix?" || exit 1
marker=$(awk '/^__RAZUM_PAYLOAD__$/ { print NR + 1; exit }' "$0")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
tail -n +"$marker" "$0" | tar -xzf - -C "$work"

if [ -f "$data/installed" ]; then
    "$0" --quiet --prefix "$prefix" --uninstall > /dev/null
fi
mkdir -p "$data/bin" "$data/cmake" "$bin"
: > "$data/installed"
cp "$work/rzc" "$data/bin/rzc"
cp "$work/RazumConfig.cmake" "$data/cmake/RazumConfig.cmake"
cp "$work/LICENSE.txt" "$data/LICENSE.txt"
cp "$work/linux-gui-NOTICES.txt" "$data/linux-gui-NOTICES.txt"
chmod 755 "$data/bin/rzc"
ln -sf "$data/bin/rzc" "$bin/rzc"
echo "link:$bin/rzc" >> "$data/installed"
if ! command -v nasm > /dev/null 2>&1 && [ -f "$work/nasm" ]; then
    cp "$work/nasm" "$data/bin/nasm"
    cp "$work/nasm-LICENSE" "$data/bin/nasm-LICENSE"
    chmod 755 "$data/bin/nasm"
    ln -sf "$data/bin/nasm" "$bin/nasm"
    echo "link:$bin/nasm" >> "$data/installed"
fi
case ":$PATH:" in
    *":$bin:"*) ;;
    *)
        if ! grep -qxF "$path_line" "$profile" 2>/dev/null; then
            printf '%s\n' "$path_line" >> "$profile"
            printf 'path:%s\n' "$path_line" >> "$data/installed"
        fi
        ;;
esac
if [ "$vscode" = 1 ] && [ -f "$work/razum.vsix" ] && native_code && ask "Install the razum extension for VS Code?"; then
    if code --install-extension "$work/razum.vsix" --force > /dev/null 2>&1; then
        echo "vscode:$(cat "$work/extension-id")" >> "$data/installed"
        echo "VS Code extension installed"
    fi
fi
echo "razum installed: $bin/rzc"
case ":$PATH:" in
    *":$bin:"*) echo "try: rzc hello.rz --run" ;;
    *) echo "open a new terminal (or run: . $profile), then try: rzc hello.rz --run" ;;
esac
echo "uninstall: $0 --uninstall"
exit 0
__RAZUM_PAYLOAD__
