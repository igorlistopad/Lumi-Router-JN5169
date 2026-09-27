/**
 * @file  app_green_power.h
 * @brief Green Power Proxy Basic
 */

#ifndef APP_GREEN_POWER_H
#define APP_GREEN_POWER_H

#include <jendefs.h>

/* SDK JN-SW-4170 */
#include "GreenPower.h"
#include "zcl.h"

PUBLIC teZCL_Status APP_eInitGreenPower(tfpZCL_ZCLCallBackFunction cbCallBack);
PUBLIC void APP_cbTimerGreenPowerTick(void *pvParam);
PUBLIC void APP_vHandleGreenPowerEvent(tsGP_GreenPowerCallBackMessage *psMessage);
PUBLIC void APP_vRestoreGreenPowerDefaults(void);

#endif /* APP_GREEN_POWER_H */
