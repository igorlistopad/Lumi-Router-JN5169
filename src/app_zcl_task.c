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

#ifdef DEBUG_ZCL
#define TRACE_ZCL TRUE
#else
#define TRACE_ZCL FALSE
#endif

#define ZCL_TICK_TIME ZTIMER_TIME_SEC(1)

PRIVATE void APP_ZCL_vTick(void);
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_vHandleClusterCustomCommands(tsZCL_CallBackEvent *psEvent);
PRIVATE teZCL_Status APP_ZCL_eRegisterEndPoint(tfpZCL_ZCLCallBackFunction cbCallBack, APP_tsLumiRouter *psDeviceInfo);
PRIVATE void APP_ZCL_vDeviceSpecific_Init(void);

PUBLIC APP_tsLumiRouter sLumiRouter;

/**
 * @brief Initialises ZCL related functions
 */
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZLL */
    eZCL_Status = eZCL_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        DBG_vPrintf(TRACE_ZCL, "Err: eZLO_Initialise:%d\n", eZCL_Status);
    }

    /* Start the tick timer */
    ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME);

    /* Register Light EndPoint */
    eZCL_Status = APP_ZCL_eRegisterEndPoint(&APP_ZCL_cbEndpointCallback, &sLumiRouter);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        DBG_vPrintf(TRACE_ZCL, "Error: APP_ZCL_eRegisterEndPoint: %02x\n", eZCL_Status);
    }

    APP_ZCL_vDeviceSpecific_Init();
}

/**
 * @brief Main ZCL processing task
 */
PUBLIC void APP_ZCL_vEventHandler(ZPS_tsAfEvent *psStackEvent)
{
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = psStackEvent;

    DBG_vPrintf(TRACE_ZCL, "ZCL_Task endpoint event:%d \n", psStackEvent->eType);
    sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
    vZCL_EventHandler(&sCallBackEvent);
}

/**
 * @brief CallBack For ZCL Tick timer
 */
PUBLIC void APP_cbTimerZclTick(void *pvParam)
{
    /*
     * If the 1 second tick timer has expired, restart it and pass
     * the event on to ZCL
     */
    APP_ZCL_vTick();
    ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME);
}

/**
 * @brief ZCL Tick
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
    case E_ZCL_CBET_LOCK_MUTEX:
        DBG_vPrintf(TRACE_ZCL, "EVT: Lock Mutex\n");
        break;

    case E_ZCL_CBET_UNLOCK_MUTEX:
        DBG_vPrintf(TRACE_ZCL, "EVT: Unlock Mutex\n");
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "EVT: Unhandled Event\n");
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "EVT: Read attributes response\n");
        break;

    case E_ZCL_CBET_READ_REQUEST:
        DBG_vPrintf(TRACE_ZCL, "EVT: Read request\n");
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "EVT: Default response\n");
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "EVT: Error\n");
        break;

    case E_ZCL_CBET_TIMER:
        DBG_vPrintf(TRACE_ZCL, "EVT: Timer\n");
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "EVT: ZigBee\n");
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom\n");
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "Invalid event type\n");
        break;
    }
}

/**
 * @brief Endpoint specific callback for ZCL events
 */
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{

    switch (psEvent->eEventType) {
    case E_ZCL_CBET_LOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNLOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Unhandled event\n");
        break;

    case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
        DBG_vPrintf(TRACE_ZCL,
                    "EP EVT: Rd Attr Rsp %04x AS %d\n",
                    psEvent->uMessage.sIndividualAttributeResponse.u16AttributeEnum,
                    psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus);
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Read attributes response\n");
        break;

    case E_ZCL_CBET_READ_REQUEST:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Read request\n");
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Default response\n");
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Error\n");
        break;

    case E_ZCL_CBET_TIMER:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Timer\n");
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: ZigBee\n");
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom Cl %04x\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);
        APP_ZCL_vHandleClusterCustomCommands(psEvent);
        break;

    case E_ZCL_CBET_WRITE_INDIVIDUAL_ATTRIBUTE:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Write Individual Attribute Status %02x\n", psEvent->eZCL_Status);
        break;

    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE: {
        tsZCL_IndividualAttributesResponse *psIndividualAttributeResponse =
            &psEvent->uMessage.sIndividualAttributeResponse;
        DBG_vPrintf(TRACE_ZCL,
                    "Individual Report attribute for Cluster = %d\n",
                    psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        DBG_vPrintf(TRACE_ZCL, "eAttributeDataType = %d\n", psIndividualAttributeResponse->eAttributeDataType);
        DBG_vPrintf(TRACE_ZCL, "u16AttributeEnum = %d\n", psIndividualAttributeResponse->u16AttributeEnum);
        DBG_vPrintf(TRACE_ZCL, "eAttributeStatus = %d\n", psIndividualAttributeResponse->eAttributeStatus);
    } break;

    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTES_CONFIGURE: {
        tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingRecord =
            &psEvent->uMessage.sAttributeReportingConfigurationRecord;
        DBG_vPrintf(
            TRACE_ZCL,
            "Individual Configure Report Cluster %d Attrib %d Type %d Min %d Max %d IntV %d Direct %d Change %d\n",
            psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum,
            psAttributeReportingRecord->u16AttributeEnum,
            psAttributeReportingRecord->eAttributeDataType,
            psAttributeReportingRecord->u16MinimumReportingInterval,
            psAttributeReportingRecord->u16MaximumReportingInterval,
            psAttributeReportingRecord->u16TimeoutPeriodField,
            psAttributeReportingRecord->u8DirectionIsReceived,
            psAttributeReportingRecord->uAttributeReportableChange);

        if (E_ZCL_SUCCESS == psEvent->eZCL_Status) {
            APP_vSaveReportableRecord(psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum,
                                      psAttributeReportingRecord);
        }
        else if (E_ZCL_RESTORE_DEFAULT_REPORT_CONFIGURATION == psEvent->eZCL_Status) {
            APP_vRestoreDefaultRecord(LUMIROUTER_APPLICATION_ENDPOINT,
                                      psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum,
                                      psAttributeReportingRecord);
        }
    } break;

    case E_ZCL_CBET_CLUSTER_UPDATE:
        DBG_vPrintf(TRACE_ZCL, "Update Id %04x\n", psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        break;

    case E_ZCL_CBET_REPORT_REQUEST:
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Invalid evt type 0x%x\n", (uint8)psEvent->eEventType);
        break;
    }
}

/**
 * @brief Callback for ZCL cluster custom command events
 */
PRIVATE void APP_ZCL_vHandleClusterCustomCommands(tsZCL_CallBackEvent *psEvent)
{
    if (psEvent->uMessage.sClusterCustomMessage.u16ClusterId == GENERAL_CLUSTER_ID_BASIC) {
        tsCLD_BasicCallBackMessage *psCallBackMessage =
            (tsCLD_BasicCallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
        if (psCallBackMessage->u8CommandId == E_CLD_BASIC_CMD_RESET_TO_FACTORY_DEFAULTS) {
            DBG_vPrintf(TRACE_ZCL, "Basic Factory Reset Received\n");
            memset(&sLumiRouter, 0, sizeof(APP_tsLumiRouter));
            APP_ZCL_eRegisterEndPoint(&APP_ZCL_cbEndpointCallback, &sLumiRouter);
            APP_ZCL_vDeviceSpecific_Init();
        }
    }
}

/**
 * @brief Register ZLO endpoints
 */
PRIVATE teZCL_Status APP_ZCL_eRegisterEndPoint(tfpZCL_ZCLCallBackFunction cbCallBack, APP_tsLumiRouter *psDeviceInfo)
{
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

    if (eCLD_BasicCreateBasic(&psDeviceInfo->sClusterInstance.sBasicServer,
                              TRUE,
                              &sCLD_Basic,
                              &psDeviceInfo->sBasicServerCluster,
                              &au8BasicClusterAttributeControlBits[0]) != E_ZCL_SUCCESS) {
        return E_ZCL_FAIL;
    }

    if (eCLD_DeviceTemperatureConfigurationCreateDeviceTemperatureConfiguration(
            &psDeviceInfo->sClusterInstance.sDeviceTemperatureConfigurationServer,
            TRUE,
            &sCLD_DeviceTemperatureConfiguration,
            &psDeviceInfo->sDeviceTemperatureConfigurationServerCluster,
            &au8DeviceTempConfigClusterAttributeControlBits[0]) != E_ZCL_SUCCESS) {
        return E_ZCL_FAIL;
    }

    return eZCL_Register(&psDeviceInfo->sEndPoint);
}

/**
 * @brief ZCL specific initialization
 */
PRIVATE void APP_ZCL_vDeviceSpecific_Init(void)
{
    memcpy(sLumiRouter.sBasicServerCluster.au8ManufacturerName, BAS_MANUF_NAME_STRING, CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8ModelIdentifier, BAS_MODEL_ID_STRING, CLD_BAS_MODEL_ID_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8DateCode, BAS_DATE_STRING, CLD_BAS_DATE_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8SWBuildID, BAS_SW_BUILD_STRING, CLD_BAS_SW_BUILD_SIZE);

    sLumiRouter.sDeviceTemperatureConfigurationServerCluster.i16CurrentTemperature = 0;
}
