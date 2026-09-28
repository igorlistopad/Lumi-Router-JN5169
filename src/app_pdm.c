/**
 * @file  app_pdm.c
 * @brief Application persistent data management.
 */

#include <jendefs.h>

/* Application */
#include "app_pdm.h"

/* SDK JN-SW-4170 */
#include "PDM.h"
#include "dbg.h"

#ifndef TRACE_PDM
#define TRACE_PDM FALSE
#endif

#if TRACE_PDM
PRIVATE void APP_PDM_vSystemEventCallback(uint32 u32EventData, PDM_eSystemEventCode eEventCode);
#endif

/**
 * @brief Initialises the Persistent Data Manager.
 */
PUBLIC bool_t APP_PDM_bInitialise(void)
{
    /* Use all available EEPROM segments. */
    PDM_teStatus eStatus = PDM_eInitialise(0);
    if (eStatus != PDM_E_STATUS_OK) {
        DBG_vPrintf(TRACE_PDM, "PDM: Initialisation failed, status=%d\n", eStatus);
        return FALSE;
    }

#if TRACE_PDM
    PDM_vRegisterSystemCallback(APP_PDM_vSystemEventCallback);
#endif

    return TRUE;
}

/**
 * @brief Reads a record from PDM and validates its size and magic.
 */
PUBLIC bool_t APP_PDM_bReadRecord(uint16 u16Id, void *pvRecord, uint16 u16Size, uint32 u32ExpectedMagic)
{
    uint16 u16RecordLength;
    uint16 u16BytesRead = 0;

    /* JN516x PDM reads the entire record without enforcing the buffer size.
     * Check the stored length before reading to prevent a buffer overflow. */
    if (!PDM_bDoesDataExist(u16Id, &u16RecordLength)) {
        DBG_vPrintf(TRACE_PDM, "PDM: Record not found, id=%04x\n", u16Id);
        return FALSE;
    }

    if (u16RecordLength != u16Size) {
        DBG_vPrintf(TRACE_PDM, "PDM: Unexpected record length, id=%04x length=%u\n", u16Id, u16RecordLength);
        return FALSE;
    }

    PDM_teStatus eStatus = PDM_eReadDataFromRecord(u16Id, pvRecord, u16Size, &u16BytesRead);
    if ((eStatus != PDM_E_STATUS_OK) || (u16BytesRead != u16Size)) {
        DBG_vPrintf(TRACE_PDM, "PDM: Read failed, id=%04x status=%d length=%u\n", u16Id, eStatus, u16BytesRead);
        return FALSE;
    }

    const APP_PDM_tsHeader *psHeader = pvRecord;
    if (psHeader->u32Magic != u32ExpectedMagic) {
        DBG_vPrintf(TRACE_PDM, "PDM: Invalid record magic, id=%04x magic=%08lx\n", u16Id, psHeader->u32Magic);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Sets the magic value and saves a record to PDM.
 */
PUBLIC bool_t APP_PDM_bSaveRecord(uint16 u16Id, void *pvRecord, uint16 u16Size, uint32 u32Magic)
{
    ((APP_PDM_tsHeader *)pvRecord)->u32Magic = u32Magic;

    PDM_teStatus eStatus = PDM_eSaveRecordData(u16Id, pvRecord, u16Size);
    if (eStatus != PDM_E_STATUS_OK) {
        DBG_vPrintf(TRACE_PDM, "PDM: Save failed, id=%04x status=%d\n", u16Id, eStatus);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Deletes the specified record from PDM.
 */
PUBLIC void APP_PDM_vDeleteRecord(uint16 u16Id)
{
    DBG_vPrintf(TRACE_PDM, "PDM: Deleting record, id=%04x\n", u16Id);
    PDM_vDeleteDataRecord(u16Id);
}

/**
 * @brief Deletes all application and Zigbee stack records from PDM.
 */
PUBLIC void APP_PDM_vDeleteAllRecords(void)
{
    DBG_vPrintf(TRACE_PDM, "PDM: Deleting all records\n");
    PDM_vDeleteAllDataRecords();
}

#if TRACE_PDM
/**
 * @brief Prints the number of free and used PDM segments.
 */
PUBLIC void APP_PDM_vPrintSegmentUsage(void)
{
    DBG_vPrintf(TRACE_PDM, "PDM: Segments free=%d used=%d\n", PDM_u8GetSegmentCapacity(), PDM_u8GetSegmentOccupancy());
}

/**
 * @brief Logs PDM errors and warnings.
 */
PRIVATE void APP_PDM_vSystemEventCallback(uint32 u32EventData, PDM_eSystemEventCode eEventCode)
{
    switch (eEventCode) {
    case E_PDM_SYSTEM_EVENT_SYSTEM_ERROR:
        DBG_vPrintf(TRACE_PDM, "PDM: Internal error, data=%lu\n", u32EventData);
        break;

    case E_PDM_SYSTEM_EVENT_DESCRIPTOR_SAVE_FAILED:
        DBG_vPrintf(TRACE_PDM, "PDM: Record save event failed, id=%04lx\n", u32EventData);
        break;

    case E_PDM_SYSTEM_EVENT_PDM_NOT_ENOUGH_SPACE:
        DBG_vPrintf(TRACE_PDM, "PDM: Not enough space, id=%04lx\n", u32EventData);
        break;

    case E_PDM_SYSTEM_EVENT_SEGMENT_DATA_CHECKSUM_FAIL:
        DBG_vPrintf(TRACE_PDM, "PDM: Segment checksum failed, segment=%lu\n", u32EventData);
        break;

    case E_PDM_SYSTEM_EVENT_LARGEST_RECORD_FULL_SAVE_NO_LONGER_POSSIBLE:
        DBG_vPrintf(TRACE_PDM, "PDM: Insufficient space to resave the largest record, id=%04lx\n", u32EventData);
        break;

    case E_PDM_SYSTEM_EVENT_WEAR_COUNT_TRIGGER_VALUE_REACHED:
        DBG_vPrintf(TRACE_PDM, "PDM: Wear count threshold reached, segment=%lu\n", u32EventData);
        break;

    default:
        break;
    }
}
#endif
