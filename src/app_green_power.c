/**
 * @file  app_green_power.c
 * @brief Green Power Proxy Basic
 */

#include <jendefs.h>
#include <string.h>

/* Application */
#include "PDM_IDs.h"
#include "app_green_power.h"
#include "app_main.h"
#include "app_pdm.h"

/* SDK JN-SW-4170 */
#include "ZTimer.h"
#include "dbg.h"
#include "gp.h"

#ifndef TRACE_GP
#define TRACE_GP FALSE
#endif

#define APP_GP_MAGIC 0x4C524701UL /* LR + G (Green Power) + revision 1 */

#define GP_TICK_TIME ZTIMER_TIME_MSEC(1)

/* ZCL uses local endpoint numbering; the SDK maps GP endpoint 242 to this local endpoint. */
#define APP_GP_LOCAL_ENDPOINT 2

typedef struct {
    APP_PDM_tsHeader sHeader;
    tsGP_ZgppProxySinkTable asProxyTable[GP_NUMBER_OF_PROXY_SINK_TABLE_ENTRIES];
} APP_tsProxyTableRecord;

PRIVATE void APP_vLoadProxyTable(void);
PRIVATE void APP_vSaveProxyTable(void);

PRIVATE tsGP_GreenPowerDevice sGreenPower;

/**
 * @brief Initialises Green Power Proxy Basic.
 */
PUBLIC teZCL_Status APP_eInitGreenPower(tfpZCL_ZCLCallBackFunction cbCallBack)
{
    teZCL_Status eZCL_Status = eGP_RegisterProxyBasicEndPoint(APP_GP_LOCAL_ENDPOINT, cbCallBack, &sGreenPower);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        return eZCL_Status;
    }

    /* Set GP defaults before restoring the proxy table from PDM. */
    vGP_RestorePersistedData(NULL, E_GP_DEFAULT_ATTRIBUTE_VALUE | E_GP_DEFAULT_PROXY_SINK_TABLE_VALUE);
    APP_vLoadProxyTable();

    ZTIMER_eStart(u8TimerGreenPowerTick, GP_TICK_TIME);

    return E_ZCL_SUCCESS;
}

/**
 * @brief Processes the Green Power millisecond tick and restarts the timer.
 */
PUBLIC void APP_cbTimerGreenPowerTick(void *pvParam)
{
    (void)pvParam;

    tsZCL_CallBackEvent sCallBackEvent = {.eEventType = E_ZCL_CBET_TIMER_MS};
    vZCL_EventHandler(&sCallBackEvent);

    ZTIMER_eStart(u8TimerGreenPowerTick, GP_TICK_TIME);
}

/**
 * @brief Handles Green Power events.
 */
PUBLIC void APP_vHandleGreenPowerEvent(tsGP_GreenPowerCallBackMessage *psMessage)
{
    switch (psMessage->eEventType) {
    case E_GP_COMMISSION_MODE_ENTER:
        DBG_vPrintf(TRACE_GP, "GP Event: Commissioning mode entered\n");
        break;

    case E_GP_COMMISSION_MODE_EXIT:
        DBG_vPrintf(TRACE_GP, "GP Event: Commissioning mode exited\n");
        break;

    case E_GP_PAIRING_CMD_RCVD:
        DBG_vPrintf(TRACE_GP,
                    "GP Event: Pairing command received, options=%06lx\n",
                    psMessage->uMessage.psZgpPairingCmdPayload->b24Options);
        break;

    case E_GP_RESPONSE_RCVD:
        DBG_vPrintf(TRACE_GP,
                    "GP Event: Response received, GPD command=%02x temporary master=%04x\n",
                    psMessage->uMessage.psZgpResponseCmdPayload->eZgpdCmdId,
                    psMessage->uMessage.psZgpResponseCmdPayload->u16TempMasterShortAddr);
        break;

    case E_GP_ZGPD_SINK_TABLE_RESPONSE_RCVD:
        DBG_vPrintf(TRACE_GP,
                    "GP Event: Sink table response received, status=%02x\n",
                    psMessage->uMessage.psZgpSinkTableRespCmdPayload->u8Status);
        break;

    case E_GP_PERSIST_SINK_PROXY_TABLE:
        DBG_vPrintf(TRACE_GP, "GP Event: Proxy table persistence requested\n");
        APP_vSaveProxyTable();
        break;

    case E_GP_CMD_UNSUPPORTED_PAYLOAD_LENGTH:
        DBG_vPrintf(TRACE_GP, "GP Event: Unsupported command payload length\n");
        break;

    case E_GP_ADDING_GROUP_TABLE_FAIL:
        DBG_vPrintf(TRACE_GP, "GP Event: Failed to add group, status=%02x\n", psMessage->uMessage.eAddGroupTableStatus);
        break;

    case E_GP_SECURITY_PROCESSING_FAILED:
        DBG_vPrintf(TRACE_GP, "GP Event: Security processing failed\n");
        break;

    case E_GP_SHARED_SECURITY_KEY_TYPE_IS_NOT_ENABLED:
        DBG_vPrintf(TRACE_GP, "GP Event: Shared security key type attribute is not enabled\n");
        break;

    case E_GP_SHARED_SECURITY_KEY_IS_NOT_ENABLED:
        DBG_vPrintf(TRACE_GP, "GP Event: Shared security key attribute is not enabled\n");
        break;

    default:
        DBG_vPrintf(TRACE_GP, "GP Event: Unexpected event type=%d\n", psMessage->eEventType);
        break;
    }
}

/**
 * @brief Restores Green Power attribute defaults and clears the proxy table from RAM and PDM.
 */
PUBLIC void APP_vRestoreGreenPowerDefaults(void)
{
    APP_PDM_vDeleteRecord(PDM_ID_APP_GP_PROXY_TABLE);
    vGP_RestorePersistedData(NULL, E_GP_DEFAULT_ATTRIBUTE_VALUE | E_GP_DEFAULT_PROXY_SINK_TABLE_VALUE);
}

/**
 * @brief Loads the Green Power proxy table from PDM.
 */
PRIVATE void APP_vLoadProxyTable(void)
{
    APP_tsProxyTableRecord sRecord;

    if (!APP_PDM_bReadRecord(PDM_ID_APP_GP_PROXY_TABLE, &sRecord, sizeof(sRecord), APP_GP_MAGIC)) {
        return;
    }

    memcpy(sGreenPower.sGreenPowerCustomDataStruct.asZgpsSinkProxyTable,
           sRecord.asProxyTable,
           sizeof(sRecord.asProxyTable));
}

/**
 * @brief Saves the Green Power proxy table to PDM.
 */
PRIVATE void APP_vSaveProxyTable(void)
{
    APP_tsProxyTableRecord sRecord;

    memcpy(sRecord.asProxyTable,
           sGreenPower.sGreenPowerCustomDataStruct.asZgpsSinkProxyTable,
           sizeof(sRecord.asProxyTable));
    APP_PDM_bSaveRecord(PDM_ID_APP_GP_PROXY_TABLE, &sRecord, sizeof(sRecord), APP_GP_MAGIC);
}
