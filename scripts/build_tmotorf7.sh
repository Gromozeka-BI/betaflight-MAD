#!/usr/bin/env bash
# SPEC board hex: TMOTORF7 with GPS + MAVLink + CRSF.
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
sed -i 's/\r$//' scripts/spec_options_tmotorf7.txt scripts/build_tmotorf7.sh || true

OPTS="$(grep -v '^#' scripts/spec_options_tmotorf7.txt | tr -d '\r' | tr '\n' ' ' | sed 's/[[:space:]]*$//')"
echo "---- BUILD TMOTORF7 ----"
echo "OPTIONS=$OPTS"
make fwo CONFIG=TMOTORF7 OPTIONS="$OPTS" -j"${JOBS:-16}"
mkdir -p dist/spec
cp -f obj/betaflight_2026.6.1_STM32F722_TMOTORF7.hex dist/spec/
cp -f BOOST_MODE.md PILOT_SPEC.md LEDSTRIP_UART.md dist/spec/ 2>/dev/null || true
ls -lh dist/spec/betaflight_2026.6.1_STM32F722*.hex
