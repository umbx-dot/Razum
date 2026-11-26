#!/usr/bin/env bash
set -eu
source "$(dirname "$0")/environment.sh"
args=()
if [ "$host" = windows ]; then
    if [ "${RZ_TRUST_TEST_CA:-0}" = 1 ]; then args+=(--trust-local); fi
    python "$(cygpath -m "$root/tests/discord/test_discord.py")" "$(cygpath -m "$rzc")" "${args[@]}"
else
    if [ ! -x "$build/razum-https-backend" ]; then args+=(--no-tls); fi
    python3 "$root/tests/discord/test_discord.py" "$rzc" "${args[@]}"
fi
