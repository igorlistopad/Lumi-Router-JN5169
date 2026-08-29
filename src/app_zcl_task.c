/**
 * @file  app_zcl_task.c
 * @brief ZCL Interface
 */

#include <jendefs.h>
#include <string.h>

/* Generated */
#include "pdum_gen.h"
#include "zps_gen.h"

/* Application */
#include "app_main.h"
#include "app_reporting.h"
#include "app_zcl_task.h"
#include "zcl_options.h"

/* SDK JN-SW-4170 */
#include "Basic.h"
#include "DeviceTemperatureConfiguration.h"
#include "ZTimer.h"
#include "dbg.h"
#include "zcl.h"

#ifndef TRACE_ZCL
#define TRACE_ZCL FALSE
#endif

#define ZCL_TICK_TIME ZTIMER_TIME_SEC(1)

PRIVATE void APP_ZCL_vTick(void);
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_vHandleConfigureReportingRecord(tsZCL_CallBackEvent *psEvent);
PRIVATE teZCL_Status APP_ZCL_eRegisterEndPoint(tfpZCL_ZCLCallBackFunction cbCallBack, APP_tsLumiRouter *psDeviceInfo);
PRIVATE void APP_ZCL_vDeviceSpecific_Init(void);

PUBLIC APP_tsLumiRouter sLumiRouter;

/**
 * @brief Initialises ZCL, registers the application endpoint, and starts the tick timer
 */
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZCL */
    eZCL_Status = eZCL_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        DBG_vPrintf(TRACE_ZCL, "ZCL Initialisation: Error status=%x\n", eZCL_Status);
    }

    /* Start the tick timer */
    ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME);

    /* Register Router EndPoint */
    eZCL_Status = APP_ZCL_eRegisterEndPoint(&APP_ZCL_cbEndpointCallback, &sLumiRouter);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        DBG_vPrintf(TRACE_ZCL, "ZCL Endpoint Registration: Error status=%x\n", eZCL_Status);
    }

    APP_ZCL_vDeviceSpecific_Init();
}

/**
 * @brief Dispatches a Zigbee stack event to ZCL
 */
PUBLIC void APP_ZCL_vEventHandler(ZPS_tsAfEvent *psStackEvent)
{
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = psStackEvent;

    DBG_vPrintf(TRACE_ZCL, "ZCL Stack Event: type=%d\n", psStackEvent->eType);
    sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
    vZCL_EventHandler(&sCallBackEvent);
}

/**
 * @brief Handles expiration of the ZCL tick timer
 */
PUBLIC void APP_cbTimerZclTick(void *pvParam)
{
    (void)pvParam;

    /* Notify ZCL of the one-second tick, then re-arm the application timer. */
    APP_ZCL_vTick();
    ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME);
}

/**
 * @brief Dispatches a timer tick event to ZCL
 */
PRIVATE void APP_ZCL_vTick(void)
{
    tsZCL_CallBackEvent sCallBackEvent;

    sCallBackEvent.pZPSevent = NULL;
    sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
    vZCL_EventHandler(&sCallBackEvent);
}

/**
 * @brief General callback for ZCL events
 */
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->eEventType) {
    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "ZCL General Callback: Error status=%x\n", psEvent->eZCL_Status);
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "ZCL General Callback: Unhandled event\n");
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "ZCL General Callback: Unexpected event type=%d\n", psEvent->eEventType);
        break;
    }
}

/**
 * @brief Endpoint specific callback for ZCL events
 */
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->eEventType) {
    case E_ZCL_CBET_READ_REQUEST:
        /* Use the current attribute values; do not refresh them before the read. */
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL,
                    "ZCL Endpoint Callback: Default response command=%02x status=%02x\n",
                    psEvent->uMessage.sDefaultResponse.u8CommandId,
                    psEvent->uMessage.sDefaultResponse.u8StatusCode);
        break;

    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTES_CONFIGURE:
        APP_ZCL_vHandleConfigureReportingRecord(psEvent);
        break;

    case E_ZCL_CBET_REPORT_ATTRIBUTES_CONFIGURE:
        DBG_vPrintf(TRACE_ZCL,
                    "ZCL Endpoint Callback: Configure reporting complete cluster=%04x\n",
                    psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        break;

    case E_ZCL_CBET_REPORT_REQUEST:
        /* Use the current attribute values; do not refresh them before reporting. */
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL,
                    "ZCL Endpoint Callback: Error status=%x endpoint=%d\n",
                    psEvent->eZCL_Status,
                    psEvent->u8EndPoint);
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "ZCL Endpoint Callback: Unhandled event\n");
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "ZCL Endpoint Callback: Unexpected event type=%d\n", psEvent->eEventType);
        break;
    }
}

/**
 * @brief Handles a Configure Reporting record
 */
PRIVATE void APP_ZCL_vHandleConfigureReportingRecord(tsZCL_CallBackEvent *psEvent)
{
    tsZCL_AttributeReportingConfigurationRecord *psRecord = &psEvent->uMessage.sAttributeReportingConfigurationRecord;
    uint16 u16ClusterId = psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum;

    if (psEvent->eZCL_Status == E_ZCL_SUCCESS) {
        DBG_vPrintf(TRACE_ZCL,
                    "ZCL Endpoint Callback: Configure reporting "
                    "cluster=%04x attribute=%04x type=%d min=%d max=%d\n",
                    u16ClusterId,
                    psRecord->u16AttributeEnum,
                    psRecord->eAttributeDataType,
                    psRecord->u16MinimumReportingInterval,
                    psRecord->u16MaximumReportingInterval);

        APP_vSaveReportableRecord(u16ClusterId, psRecord);
    }
    else if (psEvent->eZCL_Status == E_ZCL_RESTORE_DEFAULT_REPORT_CONFIGURATION) {
        DBG_vPrintf(TRACE_ZCL,
                    "ZCL Endpoint Callback: Configure reporting restore default "
                    "cluster=%04x attribute=%04x\n",
                    u16ClusterId,
                    psRecord->u16AttributeEnum);

        APP_vRestoreDefaultRecord(LUMIROUTER_APPLICATION_ENDPOINT, u16ClusterId, psRecord);
    }
    else {
        /* An empty request may leave the reporting record uninitialized. */
        DBG_vPrintf(TRACE_ZCL,
                    "ZCL Endpoint Callback: Configure reporting failed "
                    "cluster=%04x status=%x\n",
                    u16ClusterId,
                    psEvent->eZCL_Status);
    }
}

/**
 * @brief Creates cluster instances and registers the application endpoint with ZCL
 */
PRIVATE teZCL_Status APP_ZCL_eRegisterEndPoint(tfpZCL_ZCLCallBackFunction cbCallBack, APP_tsLumiRouter *psDeviceInfo)
{
    teZCL_Status eZCL_Status;

    /* Fill in end point details */
    psDeviceInfo->sEndPoint.u8EndPointNumber = LUMIROUTER_APPLICATION_ENDPOINT;
    psDeviceInfo->sEndPoint.u16ManufacturerCode = ZCL_MANUFACTURER_CODE;
    psDeviceInfo->sEndPoint.u16ProfileEnum = HA_PROFILE_ID;
    psDeviceInfo->sEndPoint.bIsManufacturerSpecificProfile = FALSE;
    psDeviceInfo->sEndPoint.u16NumberOfClusters =
        sizeof(APP_tsLumiRouterClusterInstances) / sizeof(tsZCL_ClusterInstance);
    psDeviceInfo->sEndPoint.psClusterInstance = (tsZCL_ClusterInstance *)&psDeviceInfo->sClusterInstance;
    psDeviceInfo->sEndPoint.bDisableDefaultResponse = ZCL_DISABLE_DEFAULT_RESPONSES;
    psDeviceInfo->sEndPoint.pCallBackFunctions = cbCallBack;

    eZCL_Status = eCLD_BasicCreateBasic(&psDeviceInfo->sClusterInstance.sBasicServer,
                                        TRUE,
                                        &sCLD_Basic,
                                        &psDeviceInfo->sBasicServerCluster,
                                        &au8BasicClusterAttributeControlBits[0]);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        return eZCL_Status;
    }

    eZCL_Status = eCLD_DeviceTemperatureConfigurationCreateDeviceTemperatureConfiguration(
        &psDeviceInfo->sClusterInstance.sDeviceTemperatureConfigurationServer,
        TRUE,
        &sCLD_DeviceTemperatureConfiguration,
        &psDeviceInfo->sDeviceTemperatureConfigurationServerCluster,
        &au8DeviceTempConfigClusterAttributeControlBits[0]);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        return eZCL_Status;
    }

    return eZCL_Register(&psDeviceInfo->sEndPoint);
}

/**
 * @brief Initialise ZCL device-specific attributes
 */
PRIVATE void APP_ZCL_vDeviceSpecific_Init(void)
{
    memcpy(sLumiRouter.sBasicServerCluster.au8ManufacturerName, BAS_MANUF_NAME_STRING, CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8ModelIdentifier, BAS_MODEL_ID_STRING, CLD_BAS_MODEL_ID_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8DateCode, BAS_DATE_STRING, CLD_BAS_DATE_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8SWBuildID, BAS_SW_BUILD_STRING, CLD_BAS_SW_BUILD_SIZE);
}
