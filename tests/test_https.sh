#!/usr/bin/env bash
set -u

source "$(dirname "$0")/environment.sh"
failures=0

check() {
    local name="$1" expected="$2" actual="$3"
    if [ "$expected" == "$actual" ]; then
        echo "ok   https $name"
    else
        echo "FAIL https $name: expected '$expected', got '$actual'"
        failures=$((failures + 1))
    fi
}

if ! "$rzc" "$root/tests/https/https_server.rz" -o "$work/https_server$exe" 2> "$work/https_server.log"; then
    echo "FAIL https: rzc failed: $(cat "$work/https_server.log")"
    exit 1
fi

keys="${APPDATA:-}/Microsoft/Crypto/Keys"
keys_before=$(ls "$keys" 2>/dev/null | wc -l)
"$work/https_server$exe" "$root/tests/https/localhost.pfx" > "$work/https_server.out" 2>&1 &
server=$!
port=""
for attempt in $(seq 50); do
    port="$(head -1 "$work/https_server.out" 2>/dev/null | tr -d '\r')"
    [ -n "$port" ] && break
    sleep 0.1
done
url="https://127.0.0.1:$port"

check "default handshake" "Hello over TLS" "$(curl -sk --max-time 5 "$url/")"
check "TLS 1.3" "200" "$(curl -sk --tlsv1.3 --tls-max 1.3 --max-time 5 -o /dev/null -w '%{http_code}' "$url/")"
check "TLS 1.2" "200" "$(curl -sk --tlsv1.2 --tls-max 1.2 --max-time 5 -o /dev/null -w '%{http_code}' "$url/")"
check "260 KB response" "260000" "$(curl -sk --max-time 5 "$url/large" | wc -c | tr -d ' ')"
head -c 100000 /dev/urandom > "$work/https_body.bin"
check "100 KB upload echoed" "$(md5sum < "$work/https_body.bin")" "$(curl -sk --max-time 5 --data-binary "@$work/https_body.bin" "$url/echo" | md5sum)"
check "keep-alive reuse" "Hello over TLSHello over TLSHello over TLS" "$(curl -sk --max-time 5 "$url/" "$url/" "$url/")"
check "plain HTTP on the TLS port is refused" "000" "$(curl -s --max-time 3 -o /dev/null -w '%{http_code}' "http://127.0.0.1:$port/")"
check "stop" "stopping" "$(curl -sk --max-time 5 "$url/stop")"
wait $server
check "server exit code" "0" "$?"
check "requests answered" "9" "$(tail -1 "$work/https_server.out" | tr -d '\r')"
check "no private key left behind" "$keys_before" "$(ls "$keys" 2>/dev/null | wc -l)"
exit $failures
