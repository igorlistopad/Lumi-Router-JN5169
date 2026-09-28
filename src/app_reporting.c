/**
 * @file  app_reporting.c
 * @brief Reporting functionality
 */

#include <jendefs.h>
#include <string.h>

/* Generated */
#include "zps_gen.h"

/* Application */
#include "PDM_IDs.h"
#include "app_pdm.h"
#include "app_reporting.h"
#include "zcl_options.h"

/* SDK JN-SW-4170 */
#include "DeviceTemperatureConfiguration.h"
#include "dbg.h"
#include "zcl.h"

#ifndef TRACE_REPORT
#define TRACE_REPORT FALSE
#endif

#define APP_REPORTS_MAGIC 0x4C525201UL /* LR + R (Reports) + revision 1 */

#define APP_REPORT_INDEX_INVALID 0xFF

#define DEVICE_TEMPERATURE_MINIMUM_REPORTABLE_CHANGE   0x01
#define DEVICE_TEMPERATURE_MIN_REPORT_INTERVAL_SECONDS 300
#define DEVICE_TEMPERATURE_MAX_REPORT_INTERVAL_SECONDS 3600

typedef struct {
    uint16 u16ClusterID;
    tsZCL_AttributeReportingConfigurationRecord sAttributeReportingConfigurationRecord;
} APP_tsReports;

typedef struct {
    APP_PDM_tsHeader sHeader;
    APP_tsReports asReports[NUMBER_OF_REPORTS];
} APP_tsReportsRecord;

PRIVATE void APP_vSaveReportsRecord(void);
PRIVATE uint8 APP_u8GetRecordIndex(uint16 u16ClusterID, uint16 u16AttributeEnum);
PRIVATE void APP_vPrintReportRecord(APP_tsReports *psReport);

/* Saved reporting configurations */
PRIVATE APP_tsReports asSavedReports[NUMBER_OF_REPORTS];

/* Define the default reports */
PRIVATE APP_tsReports asDefaultReports[NUMBER_OF_REPORTS] = {
    {
        GENERAL_CLUSTER_ID_DEVICE_TEMPERATURE_CONFIGURATION,
        {
            0,
            E_ZCL_INT16,
            E_CLD_DEVTEMPCFG_ATTR_ID_CURRENT_TEMPERATURE,
            DEVICE_TEMPERATURE_MIN_REPORT_INTERVAL_SECONDS,
            DEVICE_TEMPERATURE_MAX_REPORT_INTERVAL_SECONDS,
            0,
            {.zint16ReportableChange = DEVICE_TEMPERATURE_MINIMUM_REPORTABLE_CHANGE},
        },
    },
};

/**
 * @brief Restores the reporting configuration from PDM if the record is valid.
 */
PUBLIC bool_t APP_bRestoreReports(void)
{
    APP_tsReportsRecord sRecord;

    if (!APP_PDM_bReadRecord(PDM_ID_APP_REPORTS, &sRecord, sizeof(sRecord), APP_REPORTS_MAGIC)) {
        return FALSE;
    }

    memcpy(asSavedReports, sRecord.asReports, sizeof(asSavedReports));

    return TRUE;
}

/**
 * @brief Applies the reporting configuration to ZCL.
 */
PUBLIC void APP_vApplyReportingConfig(void)
{
    uint8 i;
    uint16 u16AttributeEnum;
    uint16 u16ClusterId;
    tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord;

    DBG_vPrintf(TRACE_REPORT, "Reporting: Apply configuration endpoint=%d\n", LUMIROUTER_APPLICATION_ENDPOINT);

    for (i = 0; i < NUMBER_OF_REPORTS; i++) {
        u16AttributeEnum = asSavedReports[i].sAttributeReportingConfigurationRecord.u16AttributeEnum;
        u16ClusterId = asSavedReports[i].u16ClusterID;
        psAttributeReportingConfigurationRecord = &(asSavedReports[i].sAttributeReportingConfigurationRecord);
        APP_vPrintReportRecord(&asSavedReports[i]);
        eZCL_SetReportableFlag(LUMIROUTER_APPLICATION_ENDPOINT, u16ClusterId, TRUE, FALSE, u16AttributeEnum);
        eZCL_CreateLocalReport(LUMIROUTER_APPLICATION_ENDPOINT,
                               u16ClusterId,
                               0,
                               TRUE,
                               psAttributeReportingConfigurationRecord);
    }
}

/**
 * @brief Loads a default configuration
 */
PUBLIC void APP_vLoadDefaultReports(void)
{
    uint8 i;

    DBG_vPrintf(TRACE_REPORT, "Reporting: Load default configuration\n");

    for (i = 0; i < NUMBER_OF_REPORTS; i++) {
        asSavedReports[i] = asDefaultReports[i];
        APP_vPrintReportRecord(&asSavedReports[i]);
    }

    APP_vSaveReportsRecord();
}

/**
 * @brief Save reportable record
 */
PUBLIC void
APP_vSaveReportableRecord(uint16 u16ClusterID,
                          tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord)
{
    /* Save only outgoing report configurations (direction 0). */
    if (psAttributeReportingConfigurationRecord->u8DirectionIsReceived != 0) {
        return;
    }

    uint8 u8Index = APP_u8GetRecordIndex(u16ClusterID, psAttributeReportingConfigurationRecord->u16AttributeEnum);
    if (u8Index == APP_REPORT_INDEX_INVALID) {
        return;
    }

    DBG_vPrintf(TRACE_REPORT, "Reporting: Save record index=%d\n", u8Index);

    /* Update the reportable record with new configuration */
    asSavedReports[u8Index].u16ClusterID = u16ClusterID;
    asSavedReports[u8Index].sAttributeReportingConfigurationRecord = *psAttributeReportingConfigurationRecord;
    APP_vPrintReportRecord(&asSavedReports[u8Index]);
    APP_vSaveReportsRecord();
}

/**
 * @brief Restore default record
 */
PUBLIC void
APP_vRestoreDefaultRecord(uint8 u8EndPointID,
                          uint16 u16ClusterID,
                          tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord)
{
    uint8 u8Index = APP_u8GetRecordIndex(u16ClusterID, psAttributeReportingConfigurationRecord->u16AttributeEnum);
    if (u8Index == APP_REPORT_INDEX_INVALID) {
        return;
    }

    teZCL_Status eStatus = eZCL_CreateLocalReport(u8EndPointID,
                                                  u16ClusterID,
                                                  0,
                                                  TRUE,
                                                  &asDefaultReports[u8Index].sAttributeReportingConfigurationRecord);
    if (eStatus != E_ZCL_SUCCESS) {
        DBG_vPrintf(TRACE_REPORT, "Reporting: Failed to restore default record index=%d status=%d\n", u8Index, eStatus);
        return;
    }

    DBG_vPrintf(TRACE_REPORT, "Reporting: Restore default record index=%d\n", u8Index);

    asSavedReports[u8Index] = asDefaultReports[u8Index];
    APP_vPrintReportRecord(&asSavedReports[u8Index]);
    APP_vSaveReportsRecord();
}

/**
 * @brief Saves the reporting configuration to PDM.
 */
PRIVATE void APP_vSaveReportsRecord(void)
{
    APP_tsReportsRecord sRecord;

    memcpy(sRecord.asReports, asSavedReports, sizeof(asSavedReports));
    APP_PDM_bSaveRecord(PDM_ID_APP_REPORTS, &sRecord, sizeof(sRecord), APP_REPORTS_MAGIC);
}

/**
 * @brief Get record index
 */
PRIVATE uint8 APP_u8GetRecordIndex(uint16 u16ClusterID, uint16 u16AttributeEnum)
{
    if ((u16ClusterID == GENERAL_CLUSTER_ID_DEVICE_TEMPERATURE_CONFIGURATION) &&
        (u16AttributeEnum == E_CLD_DEVTEMPCFG_ATTR_ID_CURRENT_TEMPERATURE)) {
        return REPORT_DEVICE_TEMPERATURE_CONFIGURATION_SLOT;
    }

    return APP_REPORT_INDEX_INVALID;
}

/**
 * @brief Print report record for debugging
 */
PRIVATE void APP_vPrintReportRecord(APP_tsReports *psReport)
{
    DBG_vPrintf(TRACE_REPORT,
                "Reporting: Record cluster=%04x attribute=%04x type=%d "
                "min=%d max=%d timeout=%d direction=%d change=%d\n",
                psReport->u16ClusterID,
                psReport->sAttributeReportingConfigurationRecord.u16AttributeEnum,
                psReport->sAttributeReportingConfigurationRecord.eAttributeDataType,
                psReport->sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
                psReport->sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
                psReport->sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
                psReport->sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
                psReport->sAttributeReportingConfigurationRecord.uAttributeReportableChange.zint16ReportableChange);
}
