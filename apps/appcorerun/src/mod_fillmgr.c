/**
 * Copyright 2019 Wyres
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at
 *    http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, 
 * software distributed under the License is distributed on 
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
 * either express or implied. See the License for the specific 
 * language governing permissions and limitations under the License.
*/
/**
 * Fill level manager module using app-core
 */

#include "os/os.h"
#include "bsp/bsp.h"

#include "wyres-generic/wutils.h"
#include "wyres-generic/timemgr.h"
#include "wyres-generic/ledmgr.h"
#include "wyres-generic/pwmplayer.h"
#include "wyres-generic/gpiomgr.h"
#include "wyres-generic/rebootmgr.h"
#include "wyres-generic/movementmgr.h"
#include "wyres-generic/sensormgr.h"
#include "wyres-generic/configmgr.h"

#include "app-core/app_core.h"
#include "app-core/app_msg.h"

#include "fillmgr.h"

// Our config items
#define CFG_UTIL_KEY_APP_US_ENABLED      CFGKEY(CFG_MODULE_APP, 1)
#define CFG_UTIL_KEY_APP_FULL_LEVEL      CFGKEY(CFG_MODULE_APP, 2)

// IOs
#define US_ECHO    (EXT_BUTTON)
#define US_TRIG    (SPEAKER)

// Context data
static struct appctx {
    uint8_t USEnabled;      // is US enabled
    uint8_t USFullLevel;    // limit below which we say FULL
    uint8_t USRange;        // last range reading
} _ctx;

// My api functions
static uint32_t start() {
    log_debug("MFM:start:1s");
    return 1*1000;
}

static void stop() {
    log_debug("MFM:done");
}
static void off() {
}
static void deepsleep() {
}
static bool getData(APP_CORE_UL_t* ul) {
    log_info("MFM: UL ");
    app_core_msg_ul_addTLV(ul, UL_APP_FILLMGR_RANGE, 1, &_ctx.USRange);
    uint8_t state = FILL_STATE_OK;
    if (_ctx.USRange==0) {
        state = FILL_STATE_HS;
    } else  if (_ctx.USRange<_ctx.USFullLevel) {
        state = FILL_STATE_FULL;
    }
    app_core_msg_ul_addTLV(ul, UL_APP_FILLMGR_STATE, 1, &state);
    return true;       // all critical!
}

static APP_CORE_API_t _api = {
    .startCB = &start,
    .stopCB = &stop,
    .offCB = &off,
    .deepsleepCB = &deepsleep,
    .getULDataCB = &getData,
    .ticCB = NULL,    
};
// Initialise module
void mod_fillmgr_init(void) {
    // Get config
    // default values for this build
    _ctx.USEnabled = MYNEWT_VAL(US_RANGER_ENABLED);
    _ctx.USFullLevel = MYNEWT_VAL(FULL_LEVEL);

    // US ranger enabled
    CFMgr_getOrAddElementCheckRangeUINT8(CFG_UTIL_KEY_APP_US_ENABLED, &_ctx.USEnabled, 0, 1);
    CFMgr_getOrAddElementCheckRangeUINT8(CFG_UTIL_KEY_APP_FULL_LEVEL, &_ctx.USFullLevel, 0, 255);

    // hook US sensor if used
    if (_ctx.USEnabled==1) {
        // TODO GPIO irq both edges
    }
    // hook app-core for us
    AppCore_registerModule("FM", MY_MOD_ID, &_api, EXEC_PARALLEL);

    log_info("MFM: FillMgr operation initialised");

}
