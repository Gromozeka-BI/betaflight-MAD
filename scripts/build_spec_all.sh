#!/usr/bin/env bash
# Build Betaflight-BOOST-SPEC hexes for every board config.
# Skips boards that already have a hex in dist/spec/.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

DIST="$ROOT/dist/spec"
LOG="$ROOT/dist/spec_build.log"
FAIL="$ROOT/dist/spec_failed.txt"
OK="$ROOT/dist/spec_ok.txt"

mkdir -p "$DIST"
cp -f "$ROOT/PILOT_SPEC.md" "$ROOT/BOOST_MODE.md" "$ROOT/LEDSTRIP_UART.md" "$DIST/" 2>/dev/null || true
touch "$FAIL" "$OK"
echo "=== SPEC mass build $(date -Iseconds) ===" | tee -a "$LOG"

# Parent flags required by common_post.h when CLOUD_BUILD is set.
OPTIONS="CLOUD_BUILD \
USE_BOOST_MODE_SPEC \
USE_ACRO_TRAINER \
USE_BATTERY_CONTINUE \
USE_DSHOT \
USE_ESCSERIAL_SIMONK \
USE_GPS \
USE_LED_STRIP \
USE_LED_STRIP_64 \
USE_OSD_HD \
USE_OSD_SD \
USE_PINIO \
USE_PWM_OUTPUT \
USE_RACE_PRO \
USE_SERIALRX \
USE_SERIALRX_CRSF \
USE_SERIALRX_MAVLINK \
USE_SOFTSERIAL \
USE_TELEMETRY \
USE_TELEMETRY_MAVLINK \
USE_TELEMETRY_CRSF \
USE_VTX"

export OPTIONS

list_boards() {
    find src/config/configs -name config.h | while read -r f; do
        # configs/<BOARD>/config.h  OR  configs/<MFG>/<BOARD>/config.h
        basename "$(dirname "$f")"
    done | sort -u
}

ordered_boards() {
    printf '%s\n' TMOTORF7 BETAFPVG473 TMOTORF7V2
    list_boards | grep -vxE 'TMOTORF7|BETAFPVG473|TMOTORF7V2' || true
}

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 8)}"

total=0
ok=0
fail=0
skip=0

while read -r board; do
    [ -n "$board" ] || continue
    total=$((total + 1))
    existing="$(ls "$DIST"/betaflight_*_"${board}".hex 2>/dev/null | head -1 || true)"
    if [ -n "$existing" ]; then
        echo "SKIP $board (already have $(basename "$existing"))" | tee -a "$LOG"
        skip=$((skip + 1))
        continue
    fi

    echo "---- [$total] BUILD $board ----" | tee -a "$LOG"
    if make "$board" OPTIONS="$OPTIONS" -j"$JOBS" >>"$LOG" 2>&1; then
        hex="$(ls -1 obj/betaflight_*_"${board}".hex 2>/dev/null | head -1 || true)"
        if [ -z "$hex" ]; then
            echo "FAIL $board (no hex produced)" | tee -a "$FAIL" | tee -a "$LOG"
            fail=$((fail + 1))
            continue
        fi
        cp -f "$hex" "$DIST/"
        echo "OK   $board -> $(basename "$hex")" | tee -a "$OK" | tee -a "$LOG"
        ok=$((ok + 1))
    else
        echo "FAIL $board" | tee -a "$FAIL" | tee -a "$LOG"
        fail=$((fail + 1))
        # Keep going; small MCUs often overflow with this option set.
    fi
done < <(ordered_boards)

echo "=== done $(date -Iseconds) total=$total ok=$ok fail=$fail skip=$skip ===" | tee -a "$LOG"
