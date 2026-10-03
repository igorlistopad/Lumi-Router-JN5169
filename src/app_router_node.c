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
#include "app_green_power.h"
#include "app_main.h"
#include "app_pdm.h"
#include "app_reporting.h"
#include "app_router_node.h"
#include "app_zcl_task.h"

/* SDK JN-SW-4170 */
#include "AppApi.h"
#include "AppHardwareApi.h"
#include "ZTimer.h"
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

#define APP_NODE_STATE_MAGIC 0x4C524E01UL /* LR + N (NodeState) + revision 1 */

#define JOIN_RETRY_TIME   ZTIMER_TIME_SEC(10)
#define REJOIN_RETRY_TIME ZTIMER_TIME_SEC(60)

typedef enum {
    E_NODE_NOT_JOINED,
    E_NODE_JOINED,
    E_NODE_REJOIN_REQUIRED
} APP_teNodeState;

typedef struct {
    APP_PDM_tsHeader sHeader;
    APP_teNodeState eNodeState;
} APP_tsNodeStateRecord;

PRIVATE void APP_vBdbInit(void);
PRIVATE void APP_vStartNetworkSteering(void);
PRIVATE void APP_vStartNetworkRejoin(void);
PRIVATE APP_teNodeState APP_eLoadNodeState(void);
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
    /* Restore node state or use the default. */
    eNodeState = APP_eLoadNodeState();

    /* Initialise ZCL. */
    APP_ZCL_vInitialise();

#ifdef BOARD_ZHWG11LM
    /* The Zigbee module in Aqara ZHWG11LM uses an external SKY66112-11 PA/LNA FEM.
     * Limit JN5169 TX power to -2 dBm on channels 11-25 and -10 dBm on channel 26.
     * The -2 dBm limit follows Skyworks' recommendation to avoid high TX current.
     * These match the effective TX power levels used by the original
     * Zigbee module firmware in the Aqara ZHWG11LM gateway.
     * At these limits, estimated FEM output power is +20 dBm on channels 11-25
     * and +10 dBm on channel 26. These are datasheet-based estimates, excluding
     * board losses and antenna gain, not measured values.
     * Set the raw CCA ED threshold to 97 to account for the typical 11 dB LNA gain,
     * approximately matching the default threshold of 65 on JN5169 without a FEM
     * when referred to the antenna input. */
    vAppApiSetComplianceLimits(-2, -10, 97);

    /* Enable RFTX and RFRX control outputs for the FEM. */
    vAHI_HighPowerModuleEnable(TRUE, TRUE);
#endif

    /* Initialise the ZBPro stack. */
    ZPS_teStatus eZpsStatus = ZPS_eAplAfInit();
    if (eZpsStatus != ZPS_E_SUCCESS) {
        DBG_vPrintf(TRACE_APP, "ZPS: Initialisation failed status=%02x\n", eZpsStatus);
    }

#ifdef BOARD_DGNWG05LM
    /* Set TX power to +10 dBm, the maximum supported by JN5169,
     * as used by the original Zigbee module firmware in the Xiaomi DGNWG05LM gateway.
     * Apply after stack initialisation, which resets the MAC and PHY. */
    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, 10);
#endif

    /* Initialise BDB. */
    APP_vBdbInit();

    /* Initialise the device temperature sensor. */
    APP_vDeviceTemperatureInit();

    /* Restore reporting configuration or load defaults. */
    if (!APP_bRestoreReports()) {
        APP_vLoadDefaultReports();
    }

    /* Apply the restored or default reporting configuration. */
    APP_vApplyReportingConfig();

    DBG_vPrintf(TRACE_APP,
                "APP: Router startup state=%d, BDB on network=%d\n",
                eNodeState,
                sBDB.sAttrib.bbdbNodeIsOnANetwork);

#if TRACE_APP
    APP_vPrintAPSTable();
#endif
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

        switch (eNodeState) {
        case E_NODE_NOT_JOINED:
            APP_vStartNetworkSteering();
            break;

        case E_NODE_JOINED:
            DBG_vPrintf(TRACE_APP, "APP-BDB: Joined state restored\n");
            break;

        case E_NODE_REJOIN_REQUIRED:
            APP_vStartNetworkRejoin();
            break;
        }
        break;

    case BDB_EVENT_NWK_STEERING_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Network steering succeeded\n");
        APP_vSetNodeState(E_NODE_JOINED);
        break;

    case BDB_EVENT_NO_NETWORK:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Network steering ended without joining\n");
        ZTIMER_eStart(u8TimerNetworkRetry, JOIN_RETRY_TIME);
        break;

    case BDB_EVENT_NWK_JOIN_FAILURE:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Trust Centre link-key exchange failed\n");
        break;

    case BDB_EVENT_FAILURE_RECOVERY_FOR_REJOIN:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Rejoin failed; starting BDB rejoin cycles\n");
        break;

    case BDB_EVENT_REJOIN_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Rejoin succeeded\n");
        APP_vSetNodeState(E_NODE_JOINED);
        break;

    case BDB_EVENT_REJOIN_FAILURE:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Rejoin failed; scheduling retry\n");
        APP_vSetNodeState(E_NODE_REJOIN_REQUIRED);
        ZTIMER_eStart(u8TimerNetworkRetry, REJOIN_RETRY_TIME);
        break;

    default:
        DBG_vPrintf(TRACE_APP, "APP-BDB: Unexpected event type=%d\n", psBdbEvent->eEventType);
        break;
    }
}

/**
 * @brief Retries network steering or rejoin.
 */
PUBLIC void APP_cbTimerNetworkRetry(void *pvParam)
{
    (void)pvParam;

    switch (eNodeState) {
    case E_NODE_NOT_JOINED:
        APP_vStartNetworkSteering();
        break;

    case E_NODE_REJOIN_REQUIRED:
        APP_vStartNetworkRejoin();
        break;

    default:
        break;
    }
}

/**
 * @brief Initialises BDB attributes and registers the event queue.
 */
PRIVATE void APP_vBdbInit(void)
{
    BDB_tsInitArgs sInitArgs;

    sBDB.sAttrib.bbdbNodeIsOnANetwork = (eNodeState != E_NODE_NOT_JOINED);
    sInitArgs.hBdbEventsMsgQ = &APP_msgBdbEvents;
    BDB_vInit(&sInitArgs);
}

/**
 * @brief Starts network steering and schedules a retry if the start fails.
 */
PRIVATE void APP_vStartNetworkSteering(void)
{
    BDB_teStatus eStatus = BDB_eNsStartNwkSteering();
    if (eStatus != BDB_E_SUCCESS) {
        DBG_vPrintf(TRACE_APP, "APP: Failed to start network steering, status=%d\n", eStatus);
        ZTIMER_eStart(u8TimerNetworkRetry, JOIN_RETRY_TIME);
    }
    else {
        DBG_vPrintf(TRACE_APP, "APP: Network steering started\n");
    }
}

/**
 * @brief Starts rejoin and schedules a retry if the start fails.
 */
PRIVATE void APP_vStartNetworkRejoin(void)
{
    ZPS_teStatus eStatus = ZPS_eAplZdoRejoinNetwork(TRUE);
    if (eStatus != ZPS_E_SUCCESS) {
        DBG_vPrintf(TRACE_APP, "APP: Failed to start rejoin, status=%02x\n", eStatus);
        ZTIMER_eStart(u8TimerNetworkRetry, REJOIN_RETRY_TIME);
    }
    else {
        DBG_vPrintf(TRACE_APP, "APP: Rejoin started\n");
    }
}

/**
 * @brief Loads the node state from PDM or returns the default state.
 */
PRIVATE APP_teNodeState APP_eLoadNodeState(void)
{
    APP_tsNodeStateRecord sRecord;

    if (!APP_PDM_bReadRecord(PDM_ID_APP_NODE_STATE, &sRecord, sizeof(sRecord), APP_NODE_STATE_MAGIC)) {
        return E_NODE_NOT_JOINED;
    }

    return sRecord.eNodeState;
}

/**
 * @brief Updates and persists the application node state when it changes.
 */
PRIVATE void APP_vSetNodeState(APP_teNodeState eNewState)
{
    if (eNodeState == eNewState) {
        return;
    }

    eNodeState = eNewState;

    APP_tsNodeStateRecord sRecord = {.eNodeState = eNodeState};
    APP_PDM_bSaveRecord(PDM_ID_APP_NODE_STATE, &sRecord, sizeof(sRecord), APP_NODE_STATE_MAGIC);
}

/**
 * @brief Handles application framework events.
 */
PRIVATE void APP_vHandleAfEvents(BDB_tsZpsAfEvent *psZpsAfEvent)
{
    ZPS_tsAfEvent *psAfEvent = &psZpsAfEvent->sStackEvent;

    if ((psZpsAfEvent->u8EndPoint == LUMIROUTER_APPLICATION_ENDPOINT) ||
        (psZpsAfEvent->u8EndPoint == LUMIROUTER_GREENPOWERPROXY_ENDPOINT) ||
        (psAfEvent->eType == ZPS_EVENT_APS_ZGP_DATA_CONFIRM)) {
        if ((psAfEvent->eType == ZPS_EVENT_APS_DATA_INDICATION) ||
            (psAfEvent->eType == ZPS_EVENT_APS_ZGP_DATA_INDICATION) ||
            (psAfEvent->eType == ZPS_EVENT_APS_ZGP_DATA_CONFIRM)) {
            APP_ZCL_vEventHandler(psAfEvent);
        }
    }
    else if (psZpsAfEvent->u8EndPoint == LUMIROUTER_ZDO_ENDPOINT) {
        APP_vHandleZdoEvents(psZpsAfEvent);
    }

    /* Free the data indication APDU. */
    if (psAfEvent->eType == ZPS_EVENT_APS_DATA_INDICATION) {
        PDUM_eAPduFreeAPduInstance(psAfEvent->uEvent.sApsDataIndEvent.hAPduInst);
    }
    else if (psAfEvent->eType == ZPS_EVENT_APS_ZGP_DATA_INDICATION) {
        PDUM_eAPduFreeAPduInstance(psAfEvent->uEvent.sApsZgpDataIndEvent.hAPduInst);
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
                    "APP-ZDO: Link key received type=%u ieee=%016llx\n",
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

        if (psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0ULL) {
            if (psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin != 0U) {
                APP_vSetNodeState(E_NODE_REJOIN_REQUIRED);
            }
            else {
                /* The device was requested to leave the network without rejoining. */
                DBG_vPrintf(TRACE_APP, "APP-ZDO: Local device leaving network without rejoin\n");

                APP_vFactoryResetRecords();
                vAHI_SwReset();
            }
        }
        break;

    case ZPS_EVENT_NWK_LEAVE_CONFIRM:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Leave confirm status=%02x ieee=%016llx rejoin=%u\n",
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus,
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr,
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.bRejoin);

        if ((psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus == ZPS_E_SUCCESS) &&
            (psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0ULL)) {
            if (psAfEvent->uEvent.sNwkLeaveConfirmEvent.bRejoin) {
                APP_vSetNodeState(E_NODE_REJOIN_REQUIRED);
            }
            else {
                /* The device successfully left the network without rejoining. */
                DBG_vPrintf(TRACE_APP, "APP-ZDO: Local device left network without rejoin\n");

                APP_vFactoryResetRecords();
                vAHI_SwReset();
            }
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
                    "APP-ZDO: Route discovery dst=%04x status=%02x detail=%02x\n",
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
                    "APP-ZDO: Data indication status=%02x srcMode=%u src=%016llx "
                    "srcEp=%u dstEp=%u profile=%04x cluster=%04x\n",
                    psAfEvent->uEvent.sApsDataIndEvent.eStatus,
                    psAfEvent->uEvent.sApsDataIndEvent.u8SrcAddrMode,
                    (psAfEvent->uEvent.sApsDataIndEvent.u8SrcAddrMode == ZPS_E_ADDR_MODE_IEEE)
                        ? psAfEvent->uEvent.sApsDataIndEvent.uSrcAddress.u64Addr
                        : (uint64)psAfEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,
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
    APP_vSetNodeState(E_NODE_NOT_JOINED);
    APP_vLoadDefaultReports();
    APP_vRestoreGreenPowerDefaults();
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
