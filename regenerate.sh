#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
[[ $# -eq 1 ]] || { echo 'Usage: bash regenerate.sh /path/to/English-Rev3.sms' >&2; exit 2; }
ROM="$(realpath "$1")"
EXPECTED='9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88'
[[ "$(sha256sum "$ROM" | cut -d' ' -f1)" == "$EXPECTED" ]] || { echo 'ROM SHA-256 mismatch' >&2; exit 3; }
# Refuse feedback from a self-reported/native-generated dispatch manifest.
# It is NOT an independent emulator observation; generate from ROM + tables.
[[ ! -e dispatch_manifest.txt ]] || { echo 'Unverified dispatch manifest present: quarantine it before generation' >&2; exit 4; }
cmake -S engine/recompiler -B engine/recompiler/build -DCMAKE_BUILD_TYPE=Release
cmake --build engine/recompiler/build -j2
engine/recompiler/build/SmsRecomp "$ROM" --game game.toml
bash build.sh
