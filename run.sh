#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
[[ $# -ge 1 ]] || { echo 'Usage: bash run.sh /path/to/English-Rev3.sms [options]' >&2; exit 2; }
ROM="$(realpath "$1")"; shift
EXPECTED='9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88'
[[ "$(sha256sum "$ROM" | cut -d' ' -f1)" == "$EXPECTED" ]] || { echo 'ROM SHA-256 mismatch' >&2; exit 3; }
exec bin/PhantasyStarNative "$ROM" "$@"
