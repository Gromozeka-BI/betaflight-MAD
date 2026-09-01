/*
 * This file is part of Betaflight.
 *
 * Betaflight is free software. You can redistribute this software
 * and/or modify this software under the terms of the GNU General
 * Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later
 * version.
 *
 * Betaflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "platform.h"

#ifdef USE_CMS
#ifdef USE_RPM_LIMIT

#include "cms/cms.h"
#include "cms/cms_types.h"
#include "config/config.h"
#include "pg/stats.h"
#include "flight/mixer.h"

#include "cms/cms_menu_rpm_limit.h"

uint16_t rpm_limit_value;
uint16_t kv;
bool rpm_limit;
#ifdef USE_BOOST_MODE
#ifndef USE_BOOST_MODE_SPEC
uint16_t rpm_limit_boost;
uint8_t rpm_limit_boost_duration;
uint8_t rpm_limit_boost_count;
bool rpm_limit_boost_hold;
uint8_t rpm_limit_boost_delay;
bool rpm_limit_boost_delay_reset;
bool rpm_limit_boost_count_reset;
bool rpm_limit_boost_led;
uint8_t rpm_limit_boost_led_hz;
#endif
#endif

static const void *cmsx_RpmLimit_onEnter(displayPort_t *pDisp)
{
    UNUSED(pDisp);

    rpm_limit_value = mixerConfig()->rpm_limit_value;
    kv = motorConfig()->kv;
    rpm_limit = mixerConfig()->rpm_limit;
#ifdef USE_BOOST_MODE
#ifndef USE_BOOST_MODE_SPEC
    rpm_limit_boost = mixerConfig()->rpm_limit_boost;
    rpm_limit_boost_duration = mixerConfig()->rpm_limit_boost_duration;
    rpm_limit_boost_count = mixerConfig()->rpm_limit_boost_count;
    rpm_limit_boost_hold = mixerConfig()->rpm_limit_boost_hold;
    rpm_limit_boost_delay = mixerConfig()->rpm_limit_boost_delay;
    rpm_limit_boost_delay_reset = mixerConfig()->rpm_limit_boost_delay_reset;
    rpm_limit_boost_count_reset = mixerConfig()->rpm_limit_boost_count_reset;
    rpm_limit_boost_led = mixerConfig()->rpm_limit_boost_led;
    rpm_limit_boost_led_hz = mixerConfig()->rpm_limit_boost_led_hz;
#endif
#endif

    return NULL;
}

static const void *cmsx_RpmLimit_onExit(displayPort_t *pDisp, const OSD_Entry *self)
{
    UNUSED(pDisp);
    UNUSED(self);

    mixerConfigMutable()->rpm_limit_value = rpm_limit_value;
    motorConfigMutable()->kv = kv;
    mixerConfigMutable()->rpm_limit = rpm_limit;
#ifdef USE_BOOST_MODE
#ifndef USE_BOOST_MODE_SPEC
    mixerConfigMutable()->rpm_limit_boost = rpm_limit_boost;
    mixerConfigMutable()->rpm_limit_boost_duration = rpm_limit_boost_duration;
    mixerConfigMutable()->rpm_limit_boost_count = rpm_limit_boost_count;
    mixerConfigMutable()->rpm_limit_boost_hold = rpm_limit_boost_hold;
    mixerConfigMutable()->rpm_limit_boost_delay = rpm_limit_boost_delay;
    mixerConfigMutable()->rpm_limit_boost_delay_reset = rpm_limit_boost_delay_reset;
    mixerConfigMutable()->rpm_limit_boost_count_reset = rpm_limit_boost_count_reset;
    mixerConfigMutable()->rpm_limit_boost_led = rpm_limit_boost_led;
    mixerConfigMutable()->rpm_limit_boost_led_hz = rpm_limit_boost_led_hz;
#endif
#endif

    return NULL;
}

static const OSD_Entry cmsx_menuRpmLimitEntries[] =
{
    { "-- RPM LIMIT --", OME_Label, NULL, NULL },
#ifndef USE_BOOST_MODE_SPEC
    {  "ACTIVE",   OME_Bool | REBOOT_REQUIRED,  NULL, &rpm_limit },
    { "MAX RPM", OME_UINT16, NULL, &(OSD_UINT16_t){ &rpm_limit_value, 0, UINT16_MAX, 100} },
#endif
    { "KV", OME_UINT16, NULL, &(OSD_UINT16_t){ &kv, 0, UINT16_MAX, 1} },
#ifdef USE_BOOST_MODE
#ifndef USE_BOOST_MODE_SPEC
    { "BOOST RPM", OME_UINT16, NULL, &(OSD_UINT16_t){ &rpm_limit_boost, 0, UINT16_MAX, 100} },
    { "BOOST SEC", OME_UINT8, NULL, &(OSD_UINT8_t){ &rpm_limit_boost_duration, 1, 60, 1} },
    { "BOOST CNT", OME_UINT8, NULL, &(OSD_UINT8_t){ &rpm_limit_boost_count, 0, 20, 1} },
    { "HOLD USE", OME_Bool, NULL, &rpm_limit_boost_hold },
    { "FLY DELAY", OME_UINT8, NULL, &(OSD_UINT8_t){ &rpm_limit_boost_delay, 0, 60, 1} },
    { "RST DELAY", OME_Bool, NULL, &rpm_limit_boost_delay_reset },
    { "RST COUNT", OME_Bool, NULL, &rpm_limit_boost_count_reset },
    { "LED BLINK", OME_Bool, NULL, &rpm_limit_boost_led },
    { "LED HZ", OME_UINT8, NULL, &(OSD_UINT8_t){ &rpm_limit_boost_led_hz, 1, 20, 1} },
#endif
#endif

    { "SAVE&REBOOT",     OME_OSD_Exit, cmsMenuExit,   (void *)CMS_POPUP_SAVEREBOOT },
    { "BACK", OME_Back, NULL, NULL },
    { NULL, OME_END, NULL, NULL}
};

CMS_Menu cmsx_menuRpmLimit = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "RPMLIMIT",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_RpmLimit_onEnter,
    .onExit = cmsx_RpmLimit_onExit,
    .onDisplayUpdate = NULL,
    .entries = cmsx_menuRpmLimitEntries
};

#endif
#endif // USE_RPM_LIMIT
