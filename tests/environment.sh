root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${RZ_BUILD:-$root/build}"
case "$(uname -s)" in
    MINGW* | MSYS* | CYGWIN*) host=windows exe=.exe ;;
    *) host=linux exe= ;;
esac
rzc="$build/rzc$exe"
work="$build/tests"
mkdir -p "$work"

fits_host() {
    local wanted
    wanted="$(sed -n 's|^// platform: ||p' "$1" | head -n 1)"
    [ -z "$wanted" ] || [ "$wanted" == "$host" ]
}
