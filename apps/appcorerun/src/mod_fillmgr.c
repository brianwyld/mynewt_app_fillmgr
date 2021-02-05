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
#include "wyres-generic/gpiomgr.h"
#include "wyres-generic/rebootmgr.h"
#include "wyres-generic/movementmgr.h"
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
// Beacuse we use speaker for trigger output, the mosfet inverts the sense of the signal...
#define TRIG_LO (1)
#define TRIG_HI (0)

// Context data
static struct appctx {
    uint8_t USEnabled;      // is US enabled
    uint8_t USFullLevel;    // limit below which we say FULL
    uint8_t USRange;        // last range reading
    int64_t startTuS;       // ts in uS when echo went high
    uint32_t countEchos;
} _ctx;

//predecs
static void echo_irq(void* ctx);
static void trig();

// My api functions
static uint32_t start() {
    log_debug("MFM:start:1s");
    _ctx.startTuS = 0l;       // so we know not currently checking echo
    _ctx.USRange = 0;
    _ctx.countEchos = 0;
    if (_ctx.USEnabled) {
        // Hook up the GPIOs
        GPIO_irq_enable(US_ECHO);
        // Trigger the US 
        trig();
    }
    // May have other kinds of distance measuring hw in future...
    // TODO
    return 1*1000;
}

static void stop() {
    if (_ctx.USEnabled) {
        GPIO_irq_disable(US_ECHO);
        GPIO_write(US_TRIG, TRIG_LO);
    }
    log_debug("MFM:done, range=%d, echoisrs=%d, startTuS=%ld", _ctx.USRange, _ctx.countEchos, _ctx.startTuS);
}
static void off() {
}
static void deepsleep() {
}
static bool getData(APP_CORE_UL_t* ul) {
    uint8_t range = 0;
    uint8_t state = FILL_STATE_INTERMEDIATE;

    if (_ctx.USEnabled) {
        range = _ctx.USRange;
        if (_ctx.USRange==0) {
            if (_ctx.startTuS==0l) {
                state = FILL_STATE_HS;          // Never got echo initial L->H IRQ -> hw is duff
            } else {
                state = FILL_STATE_EMPTY;       // no echo -> no contents to reflect -> empty
            }
        } else  if (_ctx.USRange<_ctx.USFullLevel) {
            state = FILL_STATE_FULL;
        }
    } else {
        // Some other hw? Not currenlty
        state = FILL_STATE_DISABLED;
    }
    app_core_msg_ul_addTLV(ul, UL_APP_FILLMGR_RANGE, 1, &range);
    app_core_msg_ul_addTLV(ul, UL_APP_FILLMGR_STATE, 1, &state);
    log_info("MFM: UL %d %d", _ctx.USRange, state);
    return (state==FILL_STATE_FULL);       // critical to send UL if full
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

    // hook app-core for us
    AppCore_registerModule("FM", MY_MOD_ID, &_api, EXEC_PARALLEL);

    // hook US sensor if used
    if (_ctx.USEnabled==1) {
        // Ok, hook the gpios
        GPIO_define_out("trig", US_TRIG, TRIG_LO, LP_DOZE, HIGH_Z);
        GPIO_define_irq("echo", US_ECHO, echo_irq, &_ctx, HAL_GPIO_TRIG_BOTH, HAL_GPIO_PULL_NONE, LP_DOZE, HIGH_Z);
    }
    log_info("MFM: FillMgr operation initialised, US[%s]", _ctx.USEnabled?"ON":"OFF");

}

// TEST
void mod_fillmgr_test() {
    start();
    TMMgr_busySleep(100);     // SOrry, this is totally against the spirit
    stop();
}

// Internals
static void trig() {
    GPIO_write(US_TRIG, TRIG_HI);
    TMMgr_busySleep(1);     // SOrry, this is totally against the spirit
    GPIO_write(US_TRIG, TRIG_LO);
    // TODO need timer to pull it low again after 10us?
}

static void echo_irq(void* ctx) {
    _ctx.countEchos++;
    // If echo is high, record start time
    if (GPIO_read(US_ECHO)!=0) {
        _ctx.startTuS = os_get_uptime_usec();
    } else {
        // Echo line is low, end of echo sensing (== echo detected)
        if (_ctx.startTuS!=0l) {
            // else calculate time and therefore distance
            int32_t distcm = (int32_t)((os_get_uptime_usec() - _ctx.startTuS)/58l);
            if (distcm>0 && distcm<500) {
                if (_ctx.USRange==0) {
                    _ctx.USRange = distcm;
                } else {
                    _ctx.USRange = (_ctx.USRange+distcm)/2;
                } 
            } else {
                // for debug, just set to the bad distance.. normally ignore it
                _ctx.USRange = distcm;
            }
        }   // else shouldn't happen! We trigger again anyway....

        // Set timer to call trig again in 100ms (so we get multiple readings and average)
        //callInXms(10, trig);
        // TODO how to do this? 
    }
}
