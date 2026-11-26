#!/usr/bin/env bash
set -eu
source "$(dirname "$0")/environment.sh"
if [ "$host" = windows ] && command -v python > /dev/null; then
    python=python
elif command -v python3 > /dev/null; then
    python=python3
elif command -v python > /dev/null; then
    python=python
else
    echo "Python is needed for project driver tests" >&2
    exit 1
fi
if [ "$host" = windows ]; then
    "$python" "$(cygpath -m "$root/tests/project/test_projects.py")" "$(cygpath -m "$build/rz$exe")"
else
    "$python" "$root/tests/project/test_projects.py" "$build/rz$exe"
fi
