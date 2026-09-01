#!/usr/bin/env bash
# Build Betaflight-BOOST-SPEC MCU cores (no per-board config.h, no CLOUD_BUILD).
#
# Use `make TARGET=<MCU> OPTIONS=...` (not `make <MCU>`) so OPTIONS with spaces
# stay in the same make process. `make STM32F722 OPTIONS='A B'` drops B via MAKEFLAGS.
#
# Classic !CLOUD_BUILD already #defines DShot/OSD/SerialRX/VTX — do not pass those.
# 512 KB cores cannot also take GPS + LED×64 + MAVLink on top of every gyro driver.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

DIST="$ROOT/dist/spec"
LOG="$ROOT/dist/spec_mcu_build.log"
FAIL="$ROOT/dist/spec_failed.txt"
OK="$ROOT/dist/spec_ok.txt"

mkdir -p "$DIST"
cp -f "$ROOT/PILOT_SPEC.md" "$ROOT/BOOST_MODE.md" "$ROOT/LEDSTRIP_UART.md" "$DIST/" 2>/dev/null || true
# Full kit rebuild clears logs. Single-MCU (MCUS=STM32F722) appends.
if [ -z "${MCUS+x}" ] || [ -z "${MCUS:-}" ]; then
    : > "$FAIL"
    : > "$OK"
    echo "=== SPEC MCU-core build $(date -Iseconds) ===" | tee "$LOG"
else
    echo "=== SPEC MCU rebuild $(date -Iseconds) MCUS=$(echo $MCUS) ===" | tee -a "$LOG"
fi

# 512 KB cores: CLOUD_BUILD, all gyros, GPS + MAVLink + CRSF.
OPTIONS_512="CLOUD_BUILD \
USE_BOOST_MODE_SPEC USE_RACE_PRO \
USE_DSHOT USE_ACC USE_GYRO \
USE_ACC_MPU6500 USE_GYRO_MPU6500 \
USE_ACC_SPI_MPU6000 USE_GYRO_SPI_MPU6000 \
USE_ACC_SPI_MPU6500 USE_GYRO_SPI_MPU6500 \
USE_ACC_SPI_ICM20689 USE_GYRO_SPI_ICM20689 \
USE_ACCGYRO_LSM6DSO USE_ACCGYRO_LSM6DSV16X USE_ACCGYRO_LSM6DSK320X \
USE_ACCGYRO_BMI270 \
USE_GYRO_SPI_ICM42605 USE_ACCGYRO_ICM42622P USE_ACCGYRO_ICM42686P \
USE_GYRO_SPI_ICM42688P USE_ACCGYRO_ICM45686 USE_ACCGYRO_ICM45605 \
USE_ACCGYRO_IIM42652 USE_ACCGYRO_IIM42653 \
USE_ACC_SPI_ICM42605 USE_ACC_SPI_ICM42688P USE_ACCGYRO_ICM40609D \
USE_BARO USE_BARO_DPS310 USE_BARO_SPI_DPS310 \
USE_BARO_BMP280 USE_BARO_SPI_BMP280 USE_BARO_BMP388 USE_BARO_SPI_BMP388 \
USE_SERIALRX USE_SERIALRX_CRSF USE_SERIALRX_MAVLINK \
USE_TELEMETRY USE_TELEMETRY_CRSF USE_TELEMETRY_MAVLINK \
USE_GPS \
USE_OSD_HD USE_OSD_SD \
USE_LED_STRIP USE_SOFTSERIAL USE_VTX USE_PINIO \
USE_FLASH USE_FLASH_M25P16 USE_FLASH_W25N01G USE_FLASH_W25Q128FV"

# Always safe extras for classic 1 MB+ cores (no CLOUD_BUILD).
OPTIONS_COMMON="USE_BOOST_MODE_SPEC USE_RACE_PRO USE_LED_STRIP USE_SOFTSERIAL"

# 1024 KB+ classic already has GPS/MAVLink/LED_STRIP; these are extra.
OPTIONS_LARGE="USE_LED_STRIP_64 USE_ACRO_TRAINER"

MCUS_512="STM32F411 STM32F446 STM32F722 STM32G474"

MCUS="${MCUS:-
STM32F722
STM32G474
STM32F405
STM32F411
STM32F446
STM32F745
STM32H743
STM32H750
STM32H723
STM32H725
STM32H730
STM32H735
AT32F435G
AT32F435M
APM32F405
APM32F407
}"

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 8)}"

is_512() {
    case " $MCUS_512 " in
        *" $1 "*) return 0 ;;
        *) return 1 ;;
    esac
}

ok=0
fail=0

for mcu in $MCUS; do
    if is_512 "$mcu"; then
        opts="$OPTIONS_512"
    else
        opts="$OPTIONS_COMMON $OPTIONS_LARGE"
    fi

    echo "---- BUILD $mcu ----" | tee -a "$LOG"
    echo "OPTIONS=$opts" | tee -a "$LOG"
    # TARGET= on the same make as OPTIONS — do not use `make $mcu`.
    if make TARGET="$mcu" OPTIONS="$opts" -j"$JOBS" >>"$LOG" 2>&1; then
        hex=""
        for f in obj/betaflight_*_"${mcu}".hex; do
            [ -f "$f" ] || continue
            base="$(basename "$f" .hex)"
            case "$base" in
                *_"${mcu}") hex="$f" ;;
            esac
        done
        if [ -z "$hex" ]; then
            echo "FAIL $mcu (no hex)" | tee -a "$FAIL" | tee -a "$LOG"
            fail=$((fail + 1))
            continue
        fi
        cp -f "$hex" "$DIST/"
        echo "OK   $mcu -> $(basename "$hex")" | tee -a "$OK" | tee -a "$LOG"
        ok=$((ok + 1))
    else
        echo "FAIL $mcu" | tee -a "$FAIL" | tee -a "$LOG"
        fail=$((fail + 1))
    fi
done

echo "=== done $(date -Iseconds) ok=$ok fail=$fail ===" | tee -a "$LOG"
