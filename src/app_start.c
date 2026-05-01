/**
 * @file  app_start.c
 * @brief Router Initialisation
 */

#include <jendefs.h>

/* Generated */
#include "pdum_gen.h"

/* Application */
#include "app_main.h"
#include "app_router_node.h"
#include "app_uart.h"

/* SDK JN-SW-4170 */
#include "AppApi.h"
#include "AppHardwareApi.h"
#include "PDM.h"
#include "bdb_api.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "pwrm.h"
#include "zps_nwk_pub.h"

#ifdef DEBUG_APP
#define TRACE_APP TRUE
#else
#define TRACE_APP FALSE
#endif

PRIVATE void APP_vInitialise(void);
PRIVATE void vfExtendedStatusCallBack(ZPS_teExtendedStatus eExtendedStatus);

extern void *_stack_low_water_mark;

/**
 * @brief Entry point for application from a cold start.
 * @note  Called in SDK JN-SW-4170
 */
PUBLIC void vAppMain(void)
{
    /* Wait until FALSE i.e. on XTAL - otherwise UART data will be at wrong speed */
    while (bAHI_GetClkSource() == TRUE)
        ;

    /* Move CPU to 32 MHz; vAHI_OptimiseWaitStates automatically called */
    bAHI_SetClockRate(3);

#ifdef UART_DEBUGGING
    /* Initialise the debug diagnostics module to use UART1 at 115K Baud */
    DBG_vUartInit(DBG_E_UART_1, DBG_E_UART_BAUD_RATE_115200);
#endif

    /* Initialise the stack overflow exception to trigger if the end of the
     * stack is reached. See the linker command file to adjust the allocated
     * stack size. */
    vAHI_SetStackOverflow(TRUE, (uint32)&_stack_low_water_mark);

    /* Catch resets due to watchdog timer expiry. Comment out to harden code. */
    if (bAHI_WatchdogResetEvent()) {
        DBG_vPrintf(TRACE_APP, "APP: Watchdog timer has reset device!\n");
        DBG_vDumpStack();
    }

#ifdef ENABLING_HIGH_POWER_MODE
    /* After testing on Xiaomi DGNWG05LM and Aqara ZHWG11LM devices, it was
     * decided to use the deprecated vAppApiSetHighPowerMode method for use on
     * JN5168 instead of the new vAHI_ModuleConfigure method for use on JN5169.
     * I checked the following options:
     * - vAHI_ModuleConfigure(E_MODULE_DEFAULT) does not work on Aqara
     * - vAHI_ModuleConfigure(E_MODULE_JN5169_001_M03_ETSI) does not work on Aqara
     * - vAHI_ModuleConfigure(E_MODULE_JN5169_001_M06_FCC) low signal on Xiaomi
     * - vAppApiSetHighPowerMode (APP_API_MODULE_HPM05, TRUE) works well both on Xiaomi and Aqara */
    vAppApiSetHighPowerMode(APP_API_MODULE_HPM05, TRUE);
#endif

    /* idle task commences here */
    DBG_vPrintf(TRACE_APP, "*** ROUTER RESET ***\n");

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vSetUpHardware()\n");
    APP_vSetUpHardware();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vInitResources()\n");
    APP_vInitResources();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vInitialise()\n");
    APP_vInitialise();

    DBG_vPrintf(TRACE_APP, "APP: Entering BDB_vStart()\n");
    BDB_vStart();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vMainLoop()\n");
    APP_vMainLoop();
}

/**
 * @brief   Power manager callback.
 * @details Called to allow the application to register sleep and wake callbacks.
 * @note    Called in SDK JN-SW-4170
 */
PUBLIC void vAppRegisterPWRMCallbacks(void)
{
    /* nothing to register as device does not sleep */
}

/**
 * @brief Initialises Zigbee stack, hardware and application.
 */
PRIVATE void APP_vInitialise(void)
{
    /* Initialise Power Manager even on non-sleeping nodes as it allows the
     * device to doze when in the idle task */
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);

    /* Initialise the Persistent Data Manager */
    PDM_eInitialise(63);

    /* Initialise Protocol Data Unit Manager */
    PDUM_vInit();

    UART_vInit();
    UART_vRtsStartFlow();

    ZPS_vExtendedStatusSetCallback(vfExtendedStatusCallBack);

    /* Initialise application */
    APP_vInitialiseRouter();
}

/**
 * @brief Callback from stack on extended error situations.
 */
PRIVATE void vfExtendedStatusCallBack(ZPS_teExtendedStatus eExtendedStatus)
{
    DBG_vPrintf(TRACE_APP, "ERROR: Extended status 0x%02x\n", eExtendedStatus);
}
