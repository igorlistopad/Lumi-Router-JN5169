/**
 * @file  app_router_node.c
 * @brief Router application
 */

#include <jendefs.h>

/* Generated */
#include "pdum_gen.h"
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
#include "AppHardwareApi.h"
#include "PDM.h"
#include "bdb_api.h"
#include "dbg.h"
#include "mac_vs_sap.h"
#include "pdum_apl.h"
#include "pdum_nwk.h"
#include "pwrm.h"
#include "zps_apl_af.h"
#include "zps_apl_aib.h"
#include "zps_apl_aps.h"
#include "zps_apl_zdo.h"
#include "zps_nwk_nib.h"

#ifdef DEBUG_APP
#define TRACE_APP TRUE
#else
#define TRACE_APP FALSE
#endif

typedef enum { E_STARTUP, E_RUNNING } APP_teNodeState;

PRIVATE void APP_vBdbInit(void);
PRIVATE void APP_vHandleAfEvents(BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void APP_vHandleZdoEvents(BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void APP_vFactoryResetRecords(void);
#if TRACE_APP
PRIVATE void APP_vPrintAPSTable(void);
#endif

PRIVATE APP_teNodeState eNodeState;

#ifdef PDM_EEPROM
extern uint8 u8PDM_CalculateFileSystemCapacity();
extern uint8 u8PDM_GetFileSystemOccupancy();
#endif

/**
 * @brief Initialises the application related functions
 */
PUBLIC void APP_vInitialiseRouter(void)
{
    uint16 u16ByteRead;
    PDM_teStatus eStatusReportReload;

    /* Stay awake */
    PWRM_eStartActivity();

    eNodeState = E_STARTUP;
    PDM_eReadDataFromRecord(PDM_ID_APP_ROUTER, &eNodeState, sizeof(APP_teNodeState), &u16ByteRead);

    /* Restore any report data that is previously saved to flash */
    eStatusReportReload = APP_eRestoreReports();

    ZPS_u32MacSetTxBuffers(4);

    /* Initialise ZBPro stack */
    ZPS_eAplAfInit();

    /* Initialise ZCL */
    APP_ZCL_vInitialise();

    /* Initialise other software modules
     * HERE */
    APP_vBdbInit();

    /* Always initialise any peripherals used by the application
     * HERE */
    APP_vDeviceTemperatureInit();

#ifdef PDM_EEPROM
    /* The functions u8PDM_CalculateFileSystemCapacity and u8PDM_GetFileSystemOccupancy
     * may be called at any time to monitor space available in  the eeprom */
    DBG_vPrintf(TRACE_APP, "PDM: Capacity %d\n", u8PDM_CalculateFileSystemCapacity());
    DBG_vPrintf(TRACE_APP, "PDM: Occupancy %d\n", u8PDM_GetFileSystemOccupancy());
#endif

    DBG_vPrintf(TRACE_APP, "Start Up StaTe %d On Network %d\n", eNodeState, sBDB.sAttrib.bbdbNodeIsOnANetwork);

    /* Load the reports from the PDM or the default ones depending on the PDM load record status */
    if (eStatusReportReload != PDM_E_STATUS_OK) {
        /* Load Defaults if the data was not correct */
        APP_vLoadDefaultConfigForReportable();
    }
    /* Make the reportable attributes */
    APP_vMakeSupportedAttributesReportable();

    APP_WriteMessageToSerial("Router started..");
}

/**
 * @brief CallBack For Restart
 */
PUBLIC void APP_cbTimerRestart(void *pvParam)
{
    vAHI_SwReset();
}

/**
 * @brief Callback from the BDB
 * @note  Called in SDK JN-SW-4170
 */
PUBLIC void APP_vBdbCallback(BDB_tsBdbEvent *psBdbEvent)
{
    BDB_teStatus eStatus;

    switch (psBdbEvent->eEventType) {
    case BDB_EVENT_NONE:
        break;

    case BDB_EVENT_ZPSAF: // Use with BDB_tsZpsAfEvent
        APP_vHandleAfEvents(&psBdbEvent->uEventData.sZpsAfEvent);
        break;

    case BDB_EVENT_INIT_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP: BDB_EVENT_INIT_SUCCESS\n");
        if (eNodeState == E_STARTUP) {
            eStatus = BDB_eNsStartNwkSteering();
            DBG_vPrintf(TRACE_APP, "BDB Try Steering status %d\n", eStatus);
        }
        else {
            DBG_vPrintf(TRACE_APP, "BDB Init go Running\n");
            eNodeState = E_RUNNING;
            PDM_eSaveRecordData(PDM_ID_APP_ROUTER, &eNodeState, sizeof(APP_teNodeState));
        }
        break;

    case BDB_EVENT_NWK_FORMATION_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP: NwkFormation Success\n");
        break;

    case BDB_EVENT_NWK_STEERING_SUCCESS:
        DBG_vPrintf(TRACE_APP, "APP: NwkSteering Success\n");
        eNodeState = E_RUNNING;
        PDM_eSaveRecordData(PDM_ID_APP_ROUTER, &eNodeState, sizeof(APP_teNodeState));
        break;

    default:
        break;
    }
}

/**
 * @brief Function to initialize BDB attributes and message queue
 */
PRIVATE void APP_vBdbInit(void)
{
    BDB_tsInitArgs sInitArgs;

    sBDB.sAttrib.bbdbNodeIsOnANetwork = (eNodeState == E_RUNNING) ? TRUE : FALSE;
    sInitArgs.hBdbEventsMsgQ = &APP_msgBdbEvents;
    BDB_vInit(&sInitArgs);
}

/**
 * @brief Application handler for stack events
 */
PRIVATE void APP_vHandleAfEvents(BDB_tsZpsAfEvent *psZpsAfEvent)
{
    if (psZpsAfEvent->u8EndPoint == LUMIROUTER_APPLICATION_ENDPOINT) {
        if ((psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION) ||
            (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION)) {
            APP_ZCL_vEventHandler(&psZpsAfEvent->sStackEvent);
        }
    }
    else if (psZpsAfEvent->u8EndPoint == LUMIROUTER_ZDO_ENDPOINT) {
        APP_vHandleZdoEvents(psZpsAfEvent);
    }

    /* Ensure Freeing of Apdus */
    if (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION) {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
    else if (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION) {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsInterPanDataIndEvent.hAPduInst);
    }
}

/**
 * @brief Application handler for stack events for end point 0 (ZDO)
 */
PRIVATE void APP_vHandleZdoEvents(BDB_tsZpsAfEvent *psZpsAfEvent)
{
    ZPS_tsAfEvent *psAfEvent = &(psZpsAfEvent->sStackEvent);

    switch (psAfEvent->eType) {
    case ZPS_EVENT_APS_DATA_INDICATION:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Data Indication Status %02x from %04x Src Ep Dst %d Ep %d Profile %04x Cluster %04x\n",
                    psAfEvent->uEvent.sApsDataIndEvent.eStatus,
                    psAfEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,
                    psAfEvent->uEvent.sApsDataIndEvent.u8SrcEndpoint,
                    psAfEvent->uEvent.sApsDataIndEvent.u8DstEndpoint,
                    psAfEvent->uEvent.sApsDataIndEvent.u16ProfileId,
                    psAfEvent->uEvent.sApsDataIndEvent.u16ClusterId);
        break;

    case ZPS_EVENT_APS_DATA_CONFIRM:
        break;

    case ZPS_EVENT_APS_DATA_ACK:
        break;

    case ZPS_EVENT_NWK_STARTED:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Network started\n");
        break;

    case ZPS_EVENT_NWK_JOINED_AS_ROUTER:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Joined Network Addr %04x Rejoin %d\n",
                    psAfEvent->uEvent.sNwkJoinedEvent.u16Addr,
                    psAfEvent->uEvent.sNwkJoinedEvent.bRejoin);
        break;
    case ZPS_EVENT_NWK_FAILED_TO_START:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Network Failed To start\n");
        break;

    case ZPS_EVENT_NWK_FAILED_TO_JOIN:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Failed To Join %02x Rejoin %d\n",
                    psAfEvent->uEvent.sNwkJoinFailedEvent.u8Status,
                    psAfEvent->uEvent.sNwkJoinFailedEvent.bRejoin);
        break;

    case ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: New Node %04x Has Joined\n",
                    psAfEvent->uEvent.sNwkJoinIndicationEvent.u16NwkAddr);
        break;

    case ZPS_EVENT_NWK_DISCOVERY_COMPLETE:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Discovery Complete %02x\n", psAfEvent->uEvent.sNwkDiscoveryEvent.eStatus);
#if TRACE_APP
        APP_vPrintAPSTable();
#endif
        break;

    case ZPS_EVENT_NWK_LEAVE_INDICATION:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Leave Indication %016llx Rejoin %d\n",
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr,
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin);
        if ((psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0UL) &&
            (psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin == 0)) {
            /* We are asked to Leave without rejoin */
            DBG_vPrintf(TRACE_APP, "LEAVE IND -> For Us No Rejoin\n");
            APP_vFactoryResetRecords();
            vAHI_SwReset();
        }
        break;

    case ZPS_EVENT_NWK_LEAVE_CONFIRM:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Leave Confirm status %02x Addr %016llx\n",
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus,
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr);
        if ((psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus == ZPS_E_SUCCESS) &&
            (psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0UL)) {
            DBG_vPrintf(TRACE_APP, "Leave -> Reset Data Structures\n");
            APP_vFactoryResetRecords();
            vAHI_SwReset();
        }
        break;

    case ZPS_EVENT_NWK_STATUS_INDICATION:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Network status Indication %02x addr %04x\n",
                    psAfEvent->uEvent.sNwkStatusIndicationEvent.u8Status,
                    psAfEvent->uEvent.sNwkStatusIndicationEvent.u16NwkAddr);
        break;

    case ZPS_EVENT_NWK_ROUTE_DISCOVERY_CONFIRM:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Discovery Confirm\n");
        break;

    case ZPS_EVENT_NWK_ED_SCAN:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Energy Detect Scan %02x\n", psAfEvent->uEvent.sNwkEdScanConfirmEvent.u8Status);
        break;

    case ZPS_EVENT_ZDO_BIND:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Zdo Bind event\n");
        break;

    case ZPS_EVENT_ZDO_UNBIND:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Zdo Unbind Event\n");
        break;

    case ZPS_EVENT_ZDO_LINK_KEY:
        DBG_vPrintf(TRACE_APP,
                    "APP-ZDO: Zdo Link Key Event Type %d Addr %016llx\n",
                    psAfEvent->uEvent.sZdoLinkKeyEvent.u8KeyType,
                    psAfEvent->uEvent.sZdoLinkKeyEvent.u64IeeeLinkAddr);
        break;

    case ZPS_EVENT_BIND_REQUEST_SERVER:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Bind Request Server Event\n");
        break;

    case ZPS_EVENT_ERROR:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: AF Error Event %d\n", psAfEvent->uEvent.sAfErrorEvent.eError);
        break;

    case ZPS_EVENT_TC_STATUS:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Trust Center Status %02x\n", psAfEvent->uEvent.sApsTcEvent.u8Status);
        break;

    default:
        DBG_vPrintf(TRACE_APP, "APP-ZDO: Unhandled Event %d\n", psAfEvent->eType);
        break;
    }
}

/**
 * @brief Resets persisted data structures to factory new state
 */
PRIVATE void APP_vFactoryResetRecords(void)
{
    /* clear out the stack */
    ZPS_vDefaultStack();
    ZPS_vSetKeys();
    ZPS_eAplAibSetApsUseExtendedPanId(0);

    /* save everything */
    eNodeState = E_STARTUP;
    PDM_eSaveRecordData(PDM_ID_APP_ROUTER, &eNodeState, sizeof(APP_teNodeState));
    ZPS_vSaveAllZpsRecords();
}

#if TRACE_APP
/**
 * @brief Prints the content of APS table
 */
PRIVATE void APP_vPrintAPSTable(void)
{
    uint8 i;
    uint8 j;

    ZPS_tsAplAib *tsAplAib;

    tsAplAib = ZPS_psAplAibGetAib();

    for (i = 0; i < (tsAplAib->psAplDeviceKeyPairTable->u16SizeOfKeyDescriptorTable + 1); i++) {
        DBG_vPrintf(TRACE_APP,
                    "MAC: %016llx Key: ",
                    ZPS_u64NwkNibGetMappedIeeeAddr(
                        ZPS_pvAplZdoGetNwkHandle(),
                        tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].u16ExtAddrLkup));
        for (j = 0; j < 16; j++) {
            DBG_vPrintf(TRACE_APP,
                        "%02x ",
                        tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].au8LinkKey[j]);
        }
        DBG_vPrintf(TRACE_APP, "\n");
        DBG_vPrintf(TRACE_APP, "Incoming FC: %d\n", tsAplAib->pu32IncomingFrameCounter[i]);
        DBG_vPrintf(TRACE_APP,
                    "Outgoing FC: %d\n",
                    tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].u32OutgoingFrameCounter);
    }
}
#endif
