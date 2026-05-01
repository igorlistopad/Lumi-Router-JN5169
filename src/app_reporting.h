/**
 * @file  app_reporting.h
 * @brief Reporting functionality
 */

#ifndef APP_REPORTING_H
#define APP_REPORTING_H

#include <jendefs.h>

/* SDK JN-SW-4170 */
#include "PDM.h"
#include "zcl.h"

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

#endif /* APP_REPORTING_H */
