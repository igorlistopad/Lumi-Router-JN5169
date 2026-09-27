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
#define ZCL_NUMBER_OF_ENDPOINTS 2

/* Reserve one ZCL timer for Green Power. */
#define ZCL_NUMBER_OF_APPLICATION_TIMERS 1

/* Set this True to disable non-error default responses from clusters */
#define ZCL_DISABLE_DEFAULT_RESPONSES (TRUE)

/* ZCL global attribute commands supported by the server */
#define ZCL_ATTRIBUTE_READ_SERVER_SUPPORTED
#define ZCL_ATTRIBUTE_WRITE_SERVER_SUPPORTED
#define ZCL_ATTRIBUTE_DISCOVERY_SERVER_SUPPORTED

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

/* Enable wild card profile */
#define ZCL_ALLOW_WILD_CARD_PROFILE

/* Enable ZCL clusters and their client/server roles */
#define CLD_BASIC
#define BASIC_SERVER
#define CLD_IDENTIFY
#define IDENTIFY_SERVER
#define CLD_DEVICE_TEMPERATURE_CONFIGURATION
#define DEVICE_TEMPERATURE_CONFIGURATION_SERVER

/* Enable Green Power Proxy Basic support. */
#define CLD_GREENPOWER
#define GP_PROXY_BASIC_DEVICE

/* Basic cluster optional attributes */
#define CLD_BAS_ATTR_MANUFACTURER_NAME
#define CLD_BAS_ATTR_MODEL_IDENTIFIER
#define CLD_BAS_ATTR_DATE_CODE
#define CLD_BAS_ATTR_SW_BUILD_ID

#define BAS_MANUF_NAME_STRING "OpenLumi"

#ifdef BOARD_DGNWG05LM
#define BAS_MODEL_ID_STRING   "openlumi.gw_router.dgnwg05lm"
#endif

#ifdef BOARD_ZHWG11LM
#define BAS_MODEL_ID_STRING   "openlumi.gw_router.zhwg11lm"
#endif

#define BAS_DATE_STRING       BUILD_DATE_STRING
#define BAS_SW_BUILD_STRING   VERSION_STRING

#define CLD_BAS_MANUF_NAME_SIZE  (sizeof(BAS_MANUF_NAME_STRING) - 1U)
#define CLD_BAS_MODEL_ID_SIZE    (sizeof(BAS_MODEL_ID_STRING) - 1U)
#define CLD_BAS_DATE_SIZE        (sizeof(BAS_DATE_STRING) - 1U)
#define CLD_BAS_SW_BUILD_SIZE    (sizeof(BAS_SW_BUILD_STRING) - 1U)
#define CLD_BAS_POWER_SOURCE     E_CLD_BAS_PS_SINGLE_PHASE_MAINS

#endif /* ZCL_OPTIONS_H */
