#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"

failures=0
cli="$work/cli"
rm -rf "$cli"
mkdir -p "$cli/project"
cd "$cli"

report() {
    if [ "$2" == 0 ]; then
        echo "ok   $1"
    else
        echo "FAIL $1"
        failures=$((failures + 1))
    fi
}

printf 'import stdio;\n\nint main() {\n    print("built");\n    return 0;\n}\n' > sample.rz
printf 'my own notes\n' > sample.asm
"$rzc" sample.rz -o "sample$exe" > /dev/null 2>&1
[ "$(cat sample.asm)" == "my own notes" ] && [ -f "sample$exe" ] && [ -z "$(ls ./*.rzc-*.asm 2> /dev/null)" ]
report "a normal build leaves an existing sample.asm alone and no temporary file behind" $?

"$rzc" sample.rz -o "kept$exe" --asm > /dev/null 2>&1
grep -q "built" kept.asm
report "--asm keeps the assembly next to the program" $?

printf 'int main() {\n    return 127;\n}\n' > exit127.rz
"$rzc" exit127.rz -o "exit127$exe" --run > /dev/null 2>&1
report "--run passes on a program's own exit code 127" $(( $? == 127 ? 0 : 1 ))

"$rzc" --help > /dev/null
report "--help succeeds" $?

cp sample.rz source.original
if "$rzc" sample.rz -o ./sample.rz > /dev/null 2>&1; then
    report "an output cannot alias its source" 1
else
    cmp -s sample.rz source.original
    report "an output cannot alias its source" $?
fi

if "$rzc" sample.rz -o colliding.asm --asm > /dev/null 2>&1; then
    report "kept assembly cannot alias the binary" 1
else
    [ ! -e colliding.asm ]
    report "kept assembly cannot alias the binary" $?
fi

printf 'void ping() {\n}\n' > project/http.rz
printf 'import http;\n\nint main() {\n    ping();\n    return 0;\n}\n' > project/main.rz
"$rzc" project/main.rz -o "project/main$exe" > /dev/null 2>&1
report "a local module named like a Windows-only library is found first" $?

if [ "$host" == linux ]; then
    cp sample.rz extensionless
    if "$rzc" extensionless > /dev/null 2>&1; then
        report "the default output preserves extensionless source" 1
    else
        cmp -s extensionless source.original
        report "the default output preserves extensionless source" $?
    fi

    ln sample.rz hardlink
    if "$rzc" sample.rz -o hardlink > /dev/null 2>&1; then
        report "an output cannot be a hard link to the source" 1
    else
        cmp -s sample.rz source.original
        report "an output cannot be a hard link to the source" $?
    fi

    case_dir="$(mktemp -d)"
    printf 'int upper() { return 1; }\n' > "$case_dir/Part.rz"
    printf 'int lower() { return 2; }\n' > "$case_dir/part.rz"
    printf 'import Part;\nimport part;\nint main() { return upper() + lower() - 3; }\n' > "$case_dir/case_probe.rz"
    "$rzc" "$case_dir/case_probe.rz" --check
    report "case-distinct Linux modules both load" $?
    rm -f "$case_dir/Part.rz" "$case_dir/part.rz" "$case_dir/case_probe.rz"
    rmdir "$case_dir"

    printf 'import stdio;
import http;

int main() {
    HttpServer *server = http_new();
    http_listen(server, "127.0.0.1", 0);
    print(http_run(server));
    print("stopped cleanly");
    return 0;
}
' > interrupt.rz
    "$rzc" interrupt.rz -o interrupt > /dev/null 2>&1
    ./interrupt > interrupt.out &
    server=$!
    sleep 0.5
    kill -INT "$server"
    wait "$server"
    status=$?
    [ "$status" == 0 ] && grep -q "stopped cleanly" interrupt.out
    report "Ctrl+C stops an http server cleanly on Linux" $?
else
    mkdir '漢字'
    printf 'int value() { return 42; }\n' > '漢字/neighbor.rz'
    printf 'import neighbor;\nint main() { return value(); }\n' > '漢字/入力.rz'
    "$rzc" '漢字/入力.rz' -o '漢字/出力.exe' --asm --run
    [ "$?" -eq 42 ] && [ -s '漢字/出力.asm' ]
    report "Unicode source, directory and output paths work through NASM" $?
fi

exit $failures
