/**
 * @file  zcl_options.h
 * @brief Options Header for ZigBee Cluster Library functions
 */

#ifndef ZCL_OPTIONS_H
#define ZCL_OPTIONS_H

#include <jendefs.h>

/* Use the NXP manufacturer code */
#define ZCL_MANUFACTURER_CODE 0x1037

/* Number of endpoints supported by this device */
#define ZCL_NUMBER_OF_ENDPOINTS 1

/* Set this True to disable non-error default responses from clusters */
#define ZCL_DISABLE_DEFAULT_RESPONSES (TRUE)

/* Which custom commands need to be supported */
#define ZCL_ATTRIBUTE_READ_SERVER_SUPPORTED

/* Configuring Attribute Reporting */
#define ZCL_ATTRIBUTE_REPORTING_SERVER_SUPPORTED
#define ZCL_CONFIGURE_ATTRIBUTE_REPORTING_SERVER_SUPPORTED
#define ZCL_READ_ATTRIBUTE_REPORTING_CONFIGURATION_SERVER_SUPPORTED

/* Reporting related configuration */
enum {
    REPORT_DEVICE_TEMPERATURE_CONFIGURATION_SLOT = 0,
    NUMBER_OF_REPORTS
};

#define ZCL_NUMBER_OF_REPORTS NUMBER_OF_REPORTS
#define MIN_REPORT_INTERVAL   300
#define MAX_REPORT_INTERVAL   3600

/* Enable wild card profile */
#define ZCL_ALLOW_WILD_CARD_PROFILE

/* Enable ZCL clusters and their client/server roles */
#define CLD_BASIC
#define BASIC_SERVER
#define CLD_DEVICE_TEMPERATURE_CONFIGURATION
#define DEVICE_TEMPERATURE_CONFIGURATION_SERVER

/* Basic cluster optional attributes */
#define CLD_BAS_ATTR_MANUFACTURER_NAME
#define CLD_BAS_ATTR_MODEL_IDENTIFIER
#define CLD_BAS_ATTR_DATE_CODE
#define CLD_BAS_ATTR_SW_BUILD_ID

#define BAS_MANUF_NAME_STRING "OPENLUMI"
#define BAS_MODEL_ID_STRING   "openlumi.gw_router.jn5169"
#define BAS_DATE_STRING       BUILD_DATE_STRING
#define BAS_SW_BUILD_STRING   VERSION_STRING

#define CLD_BAS_MANUF_NAME_SIZE  (sizeof(BAS_MANUF_NAME_STRING) - 1U)
#define CLD_BAS_MODEL_ID_SIZE    (sizeof(BAS_MODEL_ID_STRING) - 1U)
#define CLD_BAS_DATE_SIZE        (sizeof(BAS_DATE_STRING) - 1U)
#define CLD_BAS_SW_BUILD_SIZE    (sizeof(BAS_SW_BUILD_STRING) - 1U)
#define CLD_BAS_POWER_SOURCE     E_CLD_BAS_PS_SINGLE_PHASE_MAINS

#endif /* ZCL_OPTIONS_H */
