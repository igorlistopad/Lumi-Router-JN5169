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
#include "app_reporting.h"
#include "zcl_options.h"

/* SDK JN-SW-4170 */
#include "DeviceTemperatureConfiguration.h"
#include "PDM.h"
#include "dbg.h"
#include "zcl.h"
#include "zcl_common.h"

#ifdef DEBUG_REPORT
#define TRACE_REPORT TRUE
#else
#define TRACE_REPORT FALSE
#endif

#define DEVICE_TEMPERATURE_MINIMUM_REPORTABLE_CHANGE 0x01

typedef struct {
    uint16 u16ClusterID;
    tsZCL_AttributeReportingConfigurationRecord sAttributeReportingConfigurationRecord;
} APP_tsReports;

PRIVATE uint8 APP_u8GetRecordIndex(uint16 u16ClusterID, uint16 u16AttributeEnum);

/* Just Two reports for time being */
PRIVATE APP_tsReports asSavedReports[ZCL_NUMBER_OF_REPORTS];

/* define the default reports */
PRIVATE APP_tsReports asDefaultReports[ZCL_NUMBER_OF_REPORTS] = {
    {
        GENERAL_CLUSTER_ID_DEVICE_TEMPERATURE_CONFIGURATION,
        {
            0,
            E_ZCL_INT16,
            E_CLD_DEVTEMPCFG_ATTR_ID_CURRENT_TEMPERATURE,
            MIN_REPORT_INTERVAL,
            MAX_REPORT_INTERVAL,
            0,
            {DEVICE_TEMPERATURE_MINIMUM_REPORTABLE_CHANGE},
        },
    },
};

/**
 * @brief Loads the reporting information from the EEPROM/PDM
 */
PUBLIC PDM_teStatus APP_eRestoreReports(void)
{
    /* Restore any report data that is previously saved to flash */
    uint16 u16ByteRead;
    PDM_teStatus eStatusReportReload =
        PDM_eReadDataFromRecord(PDM_ID_APP_REPORTS, asSavedReports, sizeof(asSavedReports), &u16ByteRead);

    DBG_vPrintf(TRACE_REPORT, "eStatusReportReload = %d\n", eStatusReportReload);
    /* Restore any application data previously saved to flash */

    return (eStatusReportReload);
}

/**
 * @brief Makes the attributes reportable
 */
PUBLIC void APP_vMakeSupportedAttributesReportable(void)
{
    int i;
    uint16 u16AttributeEnum;
    uint16 u16ClusterId;
    tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord;

    DBG_vPrintf(TRACE_REPORT, "MAKE Reportable ep %d\n", LUMIROUTER_APPLICATION_ENDPOINT);

    for (i = 0; i < ZCL_NUMBER_OF_REPORTS; i++) {
        u16AttributeEnum = asSavedReports[i].sAttributeReportingConfigurationRecord.u16AttributeEnum;
        u16ClusterId = asSavedReports[i].u16ClusterID;
        psAttributeReportingConfigurationRecord = &(asSavedReports[i].sAttributeReportingConfigurationRecord);
        DBG_vPrintf(
            TRACE_REPORT,
            "Cluster %04x Attribute %04x Min %d Max %d IntV %d Direct %d Change %d\n",
            u16ClusterId,
            u16AttributeEnum,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
            asSavedReports[i].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zint8ReportableChange);
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
PUBLIC void APP_vLoadDefaultConfigForReportable(void)
{
    int i;

    DBG_vPrintf(TRACE_REPORT, "Loading default configuration for reports\n");

    memset(asSavedReports, 0, sizeof(asSavedReports));

    for (i = 0; i < ZCL_NUMBER_OF_REPORTS; i++) {
        asSavedReports[i] = asDefaultReports[i];
        DBG_vPrintf(
            TRACE_REPORT,
            "Cluster %04x Type %d Attr %04x Min %d Max %d IntV %d Direct %d Change %d\n",
            asSavedReports[i].u16ClusterID,
            asSavedReports[i].sAttributeReportingConfigurationRecord.eAttributeDataType,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u16AttributeEnum,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
            asSavedReports[i].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
            asSavedReports[i].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange);
    }

    /* Save this Records */
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS, asSavedReports, sizeof(asSavedReports));
}

/**
 * @brief Save reportable record
 */
PUBLIC void
APP_vSaveReportableRecord(uint16 u16ClusterID,
                          tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord)
{
    uint8 u8Index = APP_u8GetRecordIndex(u16ClusterID, psAttributeReportingConfigurationRecord->u16AttributeEnum);

    if (u8Index == 0xFF) {
        return;
    }

    DBG_vPrintf(TRACE_REPORT, "Save to report %d\n", u8Index);

    /* For CurrentLevel attribute in LevelControl Cluster */
    asSavedReports[u8Index].u16ClusterID = u16ClusterID;
    memcpy(&(asSavedReports[u8Index].sAttributeReportingConfigurationRecord),
           psAttributeReportingConfigurationRecord,
           sizeof(tsZCL_AttributeReportingConfigurationRecord));

    DBG_vPrintf(TRACE_REPORT,
                "Cluster %04x Type %d Attrib %04x Min %d Max %d IntV %d Direction %d Change %d\n",
                asSavedReports[u8Index].u16ClusterID,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.eAttributeDataType,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16AttributeEnum,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
                asSavedReports[u8Index]
                    .sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange);

    /* Save this Records */
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS, asSavedReports, sizeof(asSavedReports));
}

/**
 * @brief Restore Default Record
 */
PUBLIC void
APP_vRestoreDefaultRecord(uint8 u8EndPointID,
                          uint16 u16ClusterID,
                          tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingConfigurationRecord)
{
    uint8 u8Index = APP_u8GetRecordIndex(u16ClusterID, psAttributeReportingConfigurationRecord->u16AttributeEnum);

    if (u8Index == 0xFF) {
        return;
    }

    eZCL_CreateLocalReport(u8EndPointID,
                           u16ClusterID,
                           0,
                           TRUE,
                           &(asDefaultReports[u8Index].sAttributeReportingConfigurationRecord));

    DBG_vPrintf(TRACE_REPORT, "Save to report %d\n", u8Index);

    memcpy(&(asSavedReports[u8Index].sAttributeReportingConfigurationRecord),
           &(asDefaultReports[u8Index].sAttributeReportingConfigurationRecord),
           sizeof(tsZCL_AttributeReportingConfigurationRecord));

    DBG_vPrintf(TRACE_REPORT,
                "Cluster %04x Type %d Attrib %04x Min %d Max %d IntV %d Direction %d Change %d\n",
                asSavedReports[u8Index].u16ClusterID,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.eAttributeDataType,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16AttributeEnum,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
                asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
                asSavedReports[u8Index]
                    .sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange);

    /* Save this Records */
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS, asSavedReports, sizeof(asSavedReports));
}

/**
 * @brief Get record index
 */
PRIVATE uint8 APP_u8GetRecordIndex(uint16 u16ClusterID, uint16 u16AttributeEnum)
{
    uint8 u8Index = 0xFF;

    if (u16ClusterID == GENERAL_CLUSTER_ID_DEVICE_TEMPERATURE_CONFIGURATION) {
        u8Index = REPORT_DEVICE_TEMPERATURE_CONFIGURATION_SLOT;
    }

    return u8Index;
}
