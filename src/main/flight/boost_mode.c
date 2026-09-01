/*
 * This file is part of Betaflight.
 */

#include "platform.h"

#ifdef USE_BOOST_MODE

#include "common/filter.h"
#include "common/maths.h"

#include "drivers/dshot.h"
#include "drivers/time.h"

#include "fc/core.h"
#include "fc/rc_modes.h"
#include "fc/runtime_config.h"

#include "flight/boost_mode.h"
#include "flight/mixer.h"
#include "flight/mixer_init.h"
#include "flight/pid.h"

#include "sensors/acceleration.h"
#include "sensors/gyro.h"
#include "sensors/sensors.h"

// Count time only when the craft is actually flying, not sitting armed
// with motors spinning. Gyro/acc/RPM detect takeoff; after that the
// timer stays running through a hover until throttle drops to idle.
#define BOOST_FLIGHT_THROTTLE_MIN   15
#define BOOST_FLIGHT_THROTTLE_IDLE  8
#define BOOST_FLIGHT_GYRO_DPS       35.0f
#define BOOST_FLIGHT_ACC_LOW        0.82f
#define BOOST_FLIGHT_ACC_HIGH       1.22f
#define BOOST_FLIGHT_RPM_RATIO      0.45f

static bool wasArmed;
static bool switchWasActive;
static bool boostActive;
static uint8_t boostsRemaining;
static float tankPercent;
static float currentRpmLimit;
static float flightTimeS;
static bool boostUnlocked;
static bool airborne;

static uint16_t cfgBaseRpm(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_RPM_LIMIT;
#else
    return mixerConfig()->rpm_limit_value;
#endif
}

static uint16_t cfgBoostRpm(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_RPM_BOOST;
#else
    return mixerConfig()->rpm_limit_boost;
#endif
}

static uint8_t cfgDurationS(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_DURATION_S;
#else
    return mixerConfig()->rpm_limit_boost_duration;
#endif
}

static uint8_t cfgCount(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_COUNT;
#else
    return mixerConfig()->rpm_limit_boost_count;
#endif
}

static bool cfgHoldToUse(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_HOLD_TO_USE;
#else
    return mixerConfig()->rpm_limit_boost_hold;
#endif
}

static uint8_t cfgFlightDelayS(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_FLIGHT_DELAY_S;
#else
    return mixerConfig()->rpm_limit_boost_delay;
#endif
}

static bool cfgDelayResetOnDisarm(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_DELAY_RESET;
#else
    return mixerConfig()->rpm_limit_boost_delay_reset;
#endif
}

static bool cfgCountResetOnDisarm(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_COUNT_RESET;
#else
    return mixerConfig()->rpm_limit_boost_count_reset;
#endif
}

static bool cfgLedBlink(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_LED;
#else
    return mixerConfig()->rpm_limit_boost_led;
#endif
}

static uint8_t cfgLedHz(void)
{
#ifdef USE_BOOST_MODE_SPEC
    return BOOST_MODE_SPEC_LED_HZ;
#else
    return mixerConfig()->rpm_limit_boost_led_hz;
#endif
}

static bool isTakeoffCue(void)
{
    const float gx = gyro.gyroADCf[FD_ROLL];
    const float gy = gyro.gyroADCf[FD_PITCH];
    const float gz = gyro.gyroADCf[FD_YAW];
    if ((sq(gx) + sq(gy) + sq(gz)) >= sq(BOOST_FLIGHT_GYRO_DPS)) {
        return true;
    }

#ifdef USE_ACC
    if (sensors(SENSOR_ACC) && acc.accMagnitude > 0.0f) {
        if (acc.accMagnitude < BOOST_FLIGHT_ACC_LOW || acc.accMagnitude > BOOST_FLIGHT_ACC_HIGH) {
            return true;
        }
    }
#endif

#ifdef USE_DSHOT
    if (isDshotTelemetryActive()) {
        const float rpmThreshold = (float)cfgBaseRpm() * BOOST_FLIGHT_RPM_RATIO;
        if (getDshotRpmAverage() >= rpmThreshold) {
            return true;
        }
    }
#endif

    return false;
}

static bool isActualFlight(void)
{
    if (isCrashFlipModeActive()) {
        airborne = false;
        return false;
    }

    const uint8_t throttle = calculateThrottlePercentAbs();

    if (throttle < BOOST_FLIGHT_THROTTLE_IDLE) {
        airborne = false;
        return false;
    }

    if (airborne) {
        return true;
    }

    if (throttle >= BOOST_FLIGHT_THROTTLE_MIN && isTakeoffCue()) {
        airborne = true;
        return true;
    }

    return false;
}

static void boostModeResetState(void)
{
    boostActive = false;
    tankPercent = 100.0f;
    boostsRemaining = cfgCount();
    switchWasActive = false;
    currentRpmLimit = cfgBaseRpm();
    flightTimeS = 0.0f;
    boostUnlocked = (cfgFlightDelayS() == 0);
    airborne = false;
}

static void boostModeOnDisarm(void)
{
    boostActive = false;
    switchWasActive = false;
    currentRpmLimit = cfgBaseRpm();

    if (cfgCountResetOnDisarm()) {
        boostsRemaining = cfgCount();
        tankPercent = 100.0f;
    } else if (boostsRemaining > 0) {
        tankPercent = 100.0f;
    }

    if (cfgDelayResetOnDisarm()) {
        flightTimeS = 0.0f;
        boostUnlocked = (cfgFlightDelayS() == 0);
    }
    airborne = false;
}

void boostModeInit(void)
{
    wasArmed = false;
    boostModeResetState();
}

static void startBoost(void)
{
    if (boostUnlocked && !boostActive && boostsRemaining > 0 && tankPercent > 0.0f) {
        boostActive = true;
    }
}

static float rpmLimitForTank(void)
{
    const float baseRpm = cfgBaseRpm();
    const float extraRpm = (float)cfgBoostRpm() - baseRpm;

    if (extraRpm <= 0.0f) {
        return baseRpm;
    }

    return baseRpm + extraRpm * (tankPercent * 0.01f);
}

void boostModeUpdate(void)
{
    const bool armed = ARMING_FLAG(ARMED);

    if (!armed) {
        if (wasArmed) {
            boostModeOnDisarm();
        } else {
            currentRpmLimit = cfgBaseRpm();
        }
        wasArmed = false;
        mixerRuntime.rpmLimiterRpmLimit = currentRpmLimit;
        return;
    }

    if (!wasArmed) {
        switchWasActive = IS_RC_MODE_ACTIVE(BOXBOOST);
        boostActive = false;
        currentRpmLimit = cfgBaseRpm();
        if (boostsRemaining > 0) {
            tankPercent = 100.0f;
        }
    }
    wasArmed = true;

    if (!boostUnlocked) {
        if (isActualFlight()) {
            flightTimeS += pidGetDT();
        }
        if (flightTimeS >= (float)cfgFlightDelayS()) {
            boostUnlocked = true;
        }
    }

    const bool switchActive = IS_RC_MODE_ACTIVE(BOXBOOST);

    if (cfgHoldToUse()) {
        if (switchActive) {
            startBoost();
        } else {
            boostActive = false;
        }
    } else if (switchActive && !switchWasActive) {
        startBoost();
    }
    switchWasActive = switchActive;

    if (boostActive) {
        const float duration = MAX((float)cfgDurationS(), 1.0f);

        tankPercent -= (pidGetDT() / duration) * 100.0f;
        if (tankPercent <= 0.0f) {
            tankPercent = 0.0f;
            boostActive = false;
            if (boostsRemaining > 0) {
                boostsRemaining--;
            }
            if (boostsRemaining > 0) {
                tankPercent = 100.0f;
            }
        }
        currentRpmLimit = rpmLimitForTank();
    } else {
        currentRpmLimit = cfgBaseRpm();
    }

    mixerRuntime.rpmLimiterRpmLimit = currentRpmLimit;
}

uint8_t getBoostRemaining(void)
{
    return boostsRemaining;
}

float getBoostPercent(void)
{
    return tankPercent;
}

bool isBoostActive(void)
{
    return boostActive;
}

bool isBoostUnlocked(void)
{
    return boostUnlocked;
}

uint8_t getBoostUnlockSecondsLeft(void)
{
    if (boostUnlocked) {
        return 0;
    }

    const float remaining = (float)cfgFlightDelayS() - flightTimeS;
    if (remaining <= 0.0f) {
        return 0;
    }

    return (uint8_t)constrain((int)(remaining + 0.999f), 0, 255);
}

uint16_t getBoostBaseRpm(void)
{
    return cfgBaseRpm();
}

uint16_t getBoostTargetRpm(void)
{
    return cfgBoostRpm();
}

uint16_t getBoostCurrentRpmLimit(void)
{
    return (uint16_t)currentRpmLimit;
}

bool boostModeLedEnabled(void)
{
    return cfgLedBlink() && isBoostActive();
}

bool boostModeLedShouldBlank(void)
{
    if (!boostModeLedEnabled()) {
        return false;
    }

    const uint8_t hz = MAX(cfgLedHz(), 1);
    const uint32_t periodMs = 1000 / hz;

    return (millis() % periodMs) >= (periodMs / 2);
}

#endif
