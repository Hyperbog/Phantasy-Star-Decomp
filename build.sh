#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p bin
# Build a headless Linux executable from the generated C and patched engine.
# No ROM is needed for compiling; it is supplied separately at runtime.
gcc -std=c11 -O0 -DSMSGG_HAVE_GAME_LAYOUT -DSMS_TRACE_PC -w \
  -Iengine/runner -Iengine/runner/include -Iengine/runner/video -Igenerated \
  engine/runner/main.c engine/runner/glue.c engine/runner/video/sms_vdp.c \
  engine/runner/audio/sn76489.c engine/runner/external/superzazu/z80.c \
  generated/ps1en_full.c generated/ps1en_dispatch.c generated/ps1en_layout.c \
  -o bin/PhantasyStarNative
printf 'Built %s\n' bin/PhantasyStarNative
