/**
 * @file  app_pdm.h
 * @brief Application persistent data management.
 */

#ifndef APP_PDM_H
#define APP_PDM_H

#include <jendefs.h>

/* Must be the first member of each application PDM record. */
typedef struct {
    uint32 u32Magic;
} APP_PDM_tsHeader;

PUBLIC bool_t APP_PDM_bInitialise(void);
PUBLIC bool_t APP_PDM_bReadRecord(uint16 u16Id, void *pvRecord, uint16 u16Size, uint32 u32ExpectedMagic);
PUBLIC bool_t APP_PDM_bSaveRecord(uint16 u16Id, void *pvRecord, uint16 u16Size, uint32 u32Magic);
PUBLIC void APP_PDM_vDeleteRecord(uint16 u16Id);
PUBLIC void APP_PDM_vDeleteAllRecords(void);

#if TRACE_PDM
PUBLIC void APP_PDM_vPrintSegmentUsage(void);
#endif

#endif /* APP_PDM_H */
