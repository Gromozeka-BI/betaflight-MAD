/*
 * This file is part of Betaflight.
 *
 * BoostMode: temporary RPM-limit raise on a transmitter switch.
 * Open firmware exposes CLI/CMS settings. Spec firmware locks values
 * at compile time (define USE_BOOST_MODE_SPEC when building).
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#ifdef USE_BOOST_MODE

#ifdef USE_BOOST_MODE_SPEC
#define BOOST_MODE_SPEC_RPM_LIMIT       12000
#define BOOST_MODE_SPEC_RPM_BOOST       13600
#define BOOST_MODE_SPEC_RPM_P           25
#define BOOST_MODE_SPEC_RPM_I           10
#define BOOST_MODE_SPEC_RPM_D           8
#define BOOST_MODE_SPEC_DURATION_S      5
#define BOOST_MODE_SPEC_COUNT           3
#define BOOST_MODE_SPEC_HOLD_TO_USE     false
#define BOOST_MODE_SPEC_RESET_DISARM    false
#define BOOST_MODE_SPEC_FLIGHT_DELAY_S  10
#define BOOST_MODE_SPEC_DELAY_RESET     false
#define BOOST_MODE_SPEC_COUNT_RESET     false
#define BOOST_MODE_SPEC_LED             true
#define BOOST_MODE_SPEC_LED_HZ          4
#endif

#define BOOST_MODE_DEFAULT_RPM_LIMIT    35000
#define BOOST_MODE_DEFAULT_RPM_BOOST    38000
#define BOOST_MODE_DEFAULT_DURATION_S   5
#define BOOST_MODE_DEFAULT_COUNT        3
#define BOOST_MODE_DEFAULT_DELAY_S      10
#define BOOST_MODE_DEFAULT_LED_HZ       4

void boostModeInit(void);
void boostModeUpdate(void);

uint8_t getBoostRemaining(void);
float getBoostPercent(void);
bool isBoostActive(void);
bool isBoostUnlocked(void);
uint8_t getBoostUnlockSecondsLeft(void);
bool boostModeLedEnabled(void);
bool boostModeLedShouldBlank(void);
uint16_t getBoostBaseRpm(void);
uint16_t getBoostTargetRpm(void);
uint16_t getBoostCurrentRpmLimit(void);

#endif
