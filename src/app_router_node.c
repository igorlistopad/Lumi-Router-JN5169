/**
 * @file  app_router_node.c
 * @brief Router application implementation.
 */

#include <jendefs.h>

/* Generated */
#include "zps_gen.h"

/* Application */
#include "PDM_IDs.h"
#include "app_device_temperature.h"
#include "app_main.h"
#include "app_reporting.h"
#include "app_router_node.h"
#include "app_serial_commands.h"
#include "app_zcl_task.h"

/* SDK JN-SW-4170 */
#include "AppApi.h"
#include "AppHardwareApi.h"
#include "PDM.h"
#include "bdb_api.h"
#include "dbg.h"
#include "pdum_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_aib.h"
#include "zps_apl_aps.h"
#include "zps_apl_zdo.h"
#include "zps_nwk_nib.h"

#ifndef TRACE_APP
#define TRACE_APP FALSE
#endif

typedef enum {
    E_STARTUP,
    E_RUNNING
} APP_teNodeState;

PRIVATE void APP_vBdbInit(void);
PRIVATE void APP_vSetNodeState(APP_teNodeState eNewState);
PRIVATE void APP_vHandleAfEvents(BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void APP_vHandleZdoEvents(BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void APP_vFactoryResetRecords(void);
#if TRACE_APP
PRIVATE void APP_vPrintAPSTable(void);
#endif

PRIVATE APP_teNodeState eNodeState;

/**
 * @brief Initialises the router application.
 */
PUBLIC void APP_vInitialiseRouter(void)
{
    uint16 u16ByteRead;

    /* TODO: Implement validated PDM storage for the application node state. */
    eNodeState = E_STARTUP;
    PDM_eReadDataFromRecord(PDM_ID_APP_ROUTER, &eNodeState, sizeof(APP_teNodeState), &u16ByteRead);

    /* Initialise ZCL. */
    APP_ZCL_vInitialise();

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

    /* Initialise the ZBPro stack. */
    ZPS_teStatus eZpsStatus = ZPS_eAplAfInit();
    if (eZpsStatus != ZPS_E_SUCCESS) {
        DBG_vPrintf(TRACE_APP, "ZPS: Initialisation failed status=%02x\n", eZpsStatus);
    }

    /* Initialise BDB. */
    APP_vBdbInit();

    /* Initialise the device temperature sensor. */
    APP_vDeviceTemperatureInit();

    /* Restore reporting configuration or load defaults. */
    PDM_teStatus eStatusReportReload = APP_eRestoreReports();
    if (eStatusReportReload != PDM_E_STATUS_OK) {
        APP_vLoadDefaultConfigForReportable();
    }

    /* Apply the restored or default reporting configuration. */
    APP_vMakeSupportedAttributesReportable();

    DBG_vPrintf(TRACE_APP,
                "APP: Router startup state=%d, BDB on network=%d\n",
                eNodeState,
                sBDB.sAttrib.bbdbNodeIsOnANetwork);

#if TRACE_APP
    DBG_vPrintf(TRACE_APP,
                "PDM: Segments free=%u used=%u\n",
                (unsigned int)PDM_u8GetSegmentCapacity(),
                (unsigned int)PDM_u8GetSegmentOccupancy());

    APP_vPrintAPSTable();
#endif

    APP_vSendSerialMessage("Router started..");
}

/**
 * @brief Callback for BDB events.
 * @note Called in SDK JN-SW-4170
 */
PUBLIC void APP_vBdbCallback(BDB_tsBdbEvent *psBdbEvent)
{
    switch (psBdbEvent->eEventType) {
    case BDB_EVENT_ZPSAF:
        APP_vHandleAfEvents(&psBdbEvent->uEventData.sZpsAfEvent);
        break;

    case BDB_EVENT_INIT_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Initialisation complete\n");
        if (eNodeState == E_STARTUP) {
            BDB_teStatus eStatus = BDB_eNsStartNwkSteering();
            DBG_vPrintf(TRACE_APP, "APP-BDB: Network steering start status=%d\n", eStatus);
        }
        else {
            DBG_vPrintf(TRACE_APP, "APP-BDB: Network state restored\n");
        }
        break;

    case BDB_EVENT_NWK_STEERING_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Network steering succeeded\n");
        APP_vSetNodeState(E_RUNNING);
        break;

    case BDB_EVENT_NO_NETWORK:
        DBG_vPrintf(TRACE_APP, "APP-BDB: No suitable open network found\n");
        /* TODO: Add the required application handling for this event. */
        break;

    case BDB_EVENT_NWK_JOIN_FAILURE:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Trust Centre link-key exchange failed\n");
        break;

    case BDB_EVENT_FAILURE_RECOVERY_FOR_REJOIN:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Rejoin failed, recovery started\n");
        break;

    case BDB_EVENT_REJOIN_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Rejoin succeeded\n");
        break;

    case BDB_EVENT_REJOIN_FAILURE:
        /*
         * Not generated while apsUseInsecureJoin is enabled because BDB
         * falls back to Network Steering after all rejoin attempts fail.
         * If apsUseInsecureJoin is disabled, this terminal event requires
         * an application recovery policy.
         */
        DBG_vPrintf(TRACE_APP, "APP-BDB: Rejoin attempts exhausted\n");
        break;

    default:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Unexpected event type=%d\n", psBdbEvent->eEventType);
        break;
    }
}

/**
 * @brief Initialises BDB attributes and registers the event queue.
 */
PRIVATE void APP_vBdbInit(void)
{
    BDB_tsInitArgs sInitArgs;

    sBDB.sAttrib.bbdbNodeIsOnANetwork = (eNodeState == E_RUNNING) ? TRUE : FALSE;
    sInitArgs.hBdbEventsMsgQ = &APP_msgBdbEvents;
    BDB_vInit(&sInitArgs);
}

/**
 * @brief Updates the application node state and saves it to PDM.
 */
PRIVATE void APP_vSetNodeState(APP_teNodeState eNewState)
{
    eNodeState = eNewState;
    PDM_teStatus eStatus = PDM_eSaveRecordData(PDM_ID_APP_ROUTER, &eNodeState, sizeof(APP_teNodeState));
    if (eStatus != PDM_E_STATUS_OK) {
        DBG_vPrintf(TRACE_APP, "PDM: Failed to save node state=%d status=%d\n", eNodeState, eStatus);
    }
}

/**
 * @brief Handles application framework events.
 */
PRIVATE void APP_vHandleAfEvents(BDB_tsZpsAfEvent *psZpsAfEvent)
{
    ZPS_tsAfEvent *psAfEvent = &psZpsAfEvent->sStackEvent;

    switch (psZpsAfEvent->u8EndPoint) {
    case LUMIROUTER_APPLICATION_ENDPOINT:
        if (psAfEvent->eType == ZPS_EVENT_APS_DATA_INDICATION) {
            APP_ZCL_vEventHandler(psAfEvent);
        }
        break;

    case LUMIROUTER_ZDO_ENDPOINT:
        APP_vHandleZdoEvents(psZpsAfEvent);
        break;

    default:
        DBG_vPrintf(TRACE_APP,
                    "APP-AF: Unexpected endpoint=%u event type=%d\n",
                    psZpsAfEvent->u8EndPoint,
                    psAfEvent->eType);
        break;
    }

    /* Free the APS data indication APDU. */
    if (psAfEvent->eType == ZPS_EVENT_APS_DATA_INDICATION) {
        PDUM_eAPduFreeAPduInstance(psAfEvent->uEvent.sApsDataIndEvent.hAPduInst);
    }
}

/**
 * @brief Handles stack events for endpoint 0 (ZDO).
 */
PRIVATE void APP_vHandleZdoEvents(BDB_tsZpsAfEvent *psZpsAfEvent)
{
    ZPS_tsAfEvent *psAfEvent = &(psZpsAfEvent->sStackEvent);

    switch (psAfEvent->eType) {
    case ZPS_EVENT_NWK_DISCOVERY_COMPLETE:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Discovery status=%02x networks=%u\n",
                    psAfEvent->uEvent.sNwkDiscoveryEvent.eStatus,
                    psAfEvent->uEvent.sNwkDiscoveryEvent.u8NetworkCount);
        break;

    case ZPS_EVENT_NWK_JOINED_AS_ROUTER:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Joined addr=%04x rejoin=%u\n",
                    psAfEvent->uEvent.sNwkJoinedEvent.u16Addr,
                    psAfEvent->uEvent.sNwkJoinedEvent.bRejoin);
        break;

    case ZPS_EVENT_NWK_FAILED_TO_JOIN:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Join failed status=%02x rejoin=%u\n",
                    psAfEvent->uEvent.sNwkJoinFailedEvent.u8Status,
                    psAfEvent->uEvent.sNwkJoinFailedEvent.bRejoin);
        break;

    case ZPS_EVENT_TC_STATUS:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Trust Centre status=%02x\n", psAfEvent->uEvent.sApsTcEvent.u8Status);
        break;

    case ZPS_EVENT_ZDO_LINK_KEY:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Link key installed type=%u ieee=%016llx\n",
                    psAfEvent->uEvent.sZdoLinkKeyEvent.u8KeyType,
                    psAfEvent->uEvent.sZdoLinkKeyEvent.u64IeeeLinkAddr);
        break;

    case ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Child joined addr=%04x\n",
                    psAfEvent->uEvent.sNwkJoinIndicationEvent.u16NwkAddr);
        break;

    case ZPS_EVENT_NWK_LEAVE_INDICATION:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Leave indication ieee=%016llx rejoin=%u\n",
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr,
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin);

        if ((psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0ULL) &&
            (psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin == 0U)) {
            /* The device was requested to leave the network without rejoining. */
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Local device leaving network without rejoin\n");

            APP_vFactoryResetRecords();
            vAHI_SwReset();
        }
        break;

    case ZPS_EVENT_NWK_LEAVE_CONFIRM:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Leave confirm status=%02x ieee=%016llx rejoin=%u\n",
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus,
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr,
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.bRejoin);

        if ((psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus == ZPS_E_SUCCESS) &&
            (psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0ULL) &&
            (psAfEvent->uEvent.sNwkLeaveConfirmEvent.bRejoin == FALSE)) {
            /* The device successfully left the network without rejoining. */
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Local device left network without rejoin\n");

            APP_vFactoryResetRecords();
            vAHI_SwReset();
        }
        break;

    case ZPS_EVENT_NWK_STATUS_INDICATION:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: NWK status=%02x addr=%04x\n",
                    psAfEvent->uEvent.sNwkStatusIndicationEvent.u8Status,
                    psAfEvent->uEvent.sNwkStatusIndicationEvent.u16NwkAddr);
        break;

    case ZPS_EVENT_NWK_ROUTE_DISCOVERY_CONFIRM:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Route discovery dst=%04x mac=%02x nwk=%02x\n",
                    psAfEvent->uEvent.sNwkRouteDiscoveryConfirmEvent.u16DstAddress,
                    psAfEvent->uEvent.sNwkRouteDiscoveryConfirmEvent.u8Status,
                    psAfEvent->uEvent.sNwkRouteDiscoveryConfirmEvent.u8NwkStatus);
        break;

    case ZPS_EVENT_NWK_ED_SCAN:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: ED scan status=%02x\n", psAfEvent->uEvent.sNwkEdScanConfirmEvent.u8Status);
        break;

    case ZPS_EVENT_NWK_FC_OVERFLOW_INDICATION:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: SECURITY frame counter ieee=%016llx count=%08x incoming=%u\n",
                    psAfEvent->uEvent.sNwkFcOverflowIndEvent.u64Address,
                    psAfEvent->uEvent.sNwkFcOverflowIndEvent.u32Count,
                    psAfEvent->uEvent.sNwkFcOverflowIndEvent.bIncoming);
        break;

    case ZPS_EVENT_ZDO_BIND:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Bind mode=%u srcEp=%u dstEp=%u addr=%016llx\n",
                    psAfEvent->uEvent.sZdoBindEvent.u8DstAddrMode,
                    psAfEvent->uEvent.sZdoBindEvent.u8SrcEp,
                    psAfEvent->uEvent.sZdoBindEvent.u8DstEp,
                    (psAfEvent->uEvent.sZdoBindEvent.u8DstAddrMode == ZPS_E_ADDR_MODE_IEEE)
                        ? psAfEvent->uEvent.sZdoBindEvent.uDstAddr.u64Addr
                        : (uint64)psAfEvent->uEvent.sZdoBindEvent.uDstAddr.u16Addr);
        break;

    case ZPS_EVENT_ZDO_UNBIND:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Unbind mode=%u srcEp=%u dstEp=%u addr=%016llx\n",
                    psAfEvent->uEvent.sZdoUnbindEvent.u8DstAddrMode,
                    psAfEvent->uEvent.sZdoUnbindEvent.u8SrcEp,
                    psAfEvent->uEvent.sZdoUnbindEvent.u8DstEp,
                    (psAfEvent->uEvent.sZdoUnbindEvent.u8DstAddrMode == ZPS_E_ADDR_MODE_IEEE)
                        ? psAfEvent->uEvent.sZdoUnbindEvent.uDstAddr.u64Addr
                        : (uint64)psAfEvent->uEvent.sZdoUnbindEvent.uDstAddr.u16Addr);
        break;

    case ZPS_EVENT_BIND_REQUEST_SERVER:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Bound transfer status=%02x srcEp=%u failures=%u\n",
                    psAfEvent->uEvent.sBindRequestServerEvent.u8Status,
                    psAfEvent->uEvent.sBindRequestServerEvent.u8SrcEndpoint,
                    psAfEvent->uEvent.sBindRequestServerEvent.u32FailureCount);
        break;

    case ZPS_EVENT_APS_DATA_INDICATION:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Data indication status=%02x src=%04x srcEp=%u dstEp=%u "
                    "profile=%04x cluster=%04x\n",
                    psAfEvent->uEvent.sApsDataIndEvent.eStatus,
                    psAfEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,
                    psAfEvent->uEvent.sApsDataIndEvent.u8SrcEndpoint,
                    psAfEvent->uEvent.sApsDataIndEvent.u8DstEndpoint,
                    psAfEvent->uEvent.sApsDataIndEvent.u16ProfileId,
                    psAfEvent->uEvent.sApsDataIndEvent.u16ClusterId);
        break;

    case ZPS_EVENT_APS_DATA_CONFIRM:
    case ZPS_EVENT_APS_DATA_ACK:
        break;

    case ZPS_EVENT_ERROR:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: AF error type=%d\n", psAfEvent->uEvent.sAfErrorEvent.eError);
        break;

    default:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Unexpected event type=%d\n", psAfEvent->eType);
        break;
    }
}

/**
 * @brief Resets and persists application and Zigbee state to factory defaults.
 */
PRIVATE void APP_vFactoryResetRecords(void)
{
    /* Reset Zigbee network context and security configuration. */
    ZPS_vDefaultStack();
    ZPS_eAplAibSetApsUseExtendedPanId(0);
    ZPS_vSetKeys();

    /* Persist factory-default application and stack state. */
    APP_vSetNodeState(E_STARTUP);
    APP_vLoadDefaultConfigForReportable();
    ZPS_vSaveAllZpsRecords();
}

#if TRACE_APP
/**
 * @brief Prints the APS key descriptor table.
 */
PRIVATE void APP_vPrintAPSTable(void)
{
    uint16 i;
    uint8 j;
    ZPS_tsAplAib *psAplAib = ZPS_psAplAibGetAib();

    for (i = 0; i < (psAplAib->psAplDeviceKeyPairTable->u16SizeOfKeyDescriptorTable + 1); i++) {
        const ZPS_tsAplApsKeyDescriptorEntry *psKeyDescriptor =
            &psAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i];

        DBG_vPrintf(TRACE_APP,
                    "IEEE: %016llx Key: ",
                    ZPS_u64NwkNibGetMappedIeeeAddr(ZPS_pvAplZdoGetNwkHandle(), psKeyDescriptor->u16ExtAddrLkup));
        for (j = 0; j < ZPS_SEC_KEY_LENGTH; j++) {
            DBG_vPrintf(TRACE_APP, "%02x ", psKeyDescriptor->au8LinkKey[j]);
        }
        DBG_vPrintf(TRACE_APP, "\n");
        DBG_vPrintf(TRACE_APP, "Incoming FC: %lu\n", (unsigned long)psAplAib->pu32IncomingFrameCounter[i]);
        DBG_vPrintf(TRACE_APP, "Outgoing FC: %lu\n", (unsigned long)psKeyDescriptor->u32OutgoingFrameCounter);
    }
}
#endif
