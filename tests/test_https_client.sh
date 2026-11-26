#!/usr/bin/env bash
set -eu

source "$(dirname "$0")/environment.sh"
target="$work/https_client$exe"
"$rzc" "$root/tests/https/https_client.rz" -O -o "$target"
"$target" http://localhost/ 1000 4096 -31000

if [ "$host" = linux ] && [ ! -x "$build/razum-https-backend" ]; then
    "$target" https://localhost/ 1000 4096 -31001
    echo "ok   Linux HTTPS client reports its optional backend is unavailable"
    exit 0
fi

"$rzc" "$root/tests/https/https_server.rz" -o "$work/https_client_server$exe"
"$rzc" "$root/tests/https/https_stall.rz" -o "$work/https_client_stall$exe"

port_from() {
    for attempt in $(seq 50); do
        port="$(head -1 "$1" | tr -d '\r')"
        if [[ "$port" =~ ^[0-9]+$ ]]; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

"$work/https_client_server$exe" "$root/tests/https/localhost.pfx" > "$work/https_client_server.out" 2>&1 &
server=$!
trap 'kill "$server" 2>/dev/null || true' EXIT
port_from "$work/https_client_server.out"
url="https://localhost:$port"
"$target" "$url/" 3000 4096 -31004
echo "ok   HTTPS client rejects an untrusted server"

if [ "$host" = linux ]; then
    ca="$root/tests/https/localhost.pem"
    env PATH="$work/no-curl-on-path" "$target" "$url/" 3000 14 200 "$ca"
    "$target" "$url/binary" 3000 4 201 "$ca"
    "$target" "$url/large" 3000 16 -31003 "$ca"
    "$target" "https://127.0.0.1:$port/" 3000 4096 -31004 "$ca"
    echo "ok   HTTPS client verifies the CA and hostname, keeps binary bytes and enforces the body limit"
fi
curl -sk --max-time 3 "$url/stop" > /dev/null
wait "$server"

"$work/https_client_stall$exe" > "$work/https_client_stall.out" 2>&1 &
server=$!
port_from "$work/https_client_stall.out"
"$target" "https://127.0.0.1:$port/" 250 4096 -31002
wait "$server"
echo "ok   HTTPS client cancels a stalled TLS handshake"
trap - EXIT
