#!/usr/bin/env bash
# Universal F722 (no GPS/MAVLink) then TMOTORF7 board (GPS+MAVLink).
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
sed -i 's/\r$//' scripts/build_spec_mcu.sh || true
MCUS=STM32F722 bash scripts/build_spec_mcu.sh

OPTS="$(grep -v '^#' scripts/spec_options_tmotorf7.txt | tr -d '\r' | tr '\n' ' ' | sed 's/[[:space:]]*$//')"
echo "---- BUILD TMOTORF7 ----"
echo "OPTIONS=$OPTS"
make fwo CONFIG=TMOTORF7 OPTIONS="$OPTS" -j"${JOBS:-16}"
cp -f obj/betaflight_2026.6.1_STM32F722_TMOTORF7.hex dist/spec/
ls -lh dist/spec/betaflight_2026.6.1_STM32F722*.hex
