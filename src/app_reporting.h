/****************************************************************************
 *
 * MODULE:              Lumi Router
 *
 * COMPONENT:           app_reporting.h
 *
 * DESCRIPTION:         Reporting functionality
 *
 ****************************************************************************/

#ifndef APP_REPORTING_H
#define APP_REPORTING_H

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>

/* SDK JN-SW-4170 */
#include "PDM.h"
#include "zcl.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC PDM_teStatus APP_eRestoreReports(void);
PUBLIC void APP_vMakeSupportedAttributesReportable(void);
PUBLIC void APP_vLoadDefaultConfigForReportable(void);
PUBLIC void
APP_vSaveReportableRecord(uint16 u16ClusterID,
                          tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord);
PUBLIC void
APP_vRestoreDefaultRecord(uint8 u8EndPointID,
                          uint16 u16ClusterID,
                          tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord);

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

#endif /* APP_REPORTING_H */
