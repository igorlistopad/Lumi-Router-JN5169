/**
 * @file  zcl_options.h
 * @brief Options Header for ZigBee Cluster Library functions
 */

#ifndef ZCL_OPTIONS_H
#define ZCL_OPTIONS_H

#include <jendefs.h>

/**
 * @brief ZCL Specific initialization 
 */
#define ZCL_MANUFACTURER_CODE 0x1037

/* Number of endpoints supported by this device */
#define ZCL_NUMBER_OF_ENDPOINTS 1

/* ZCL has all cooperative tasks */
#define COOPERATIVE

/* Set this True to disable non-error default responses from clusters */
#define ZCL_DISABLE_DEFAULT_RESPONSES (TRUE)
#define ZCL_DISABLE_APS_ACK           (TRUE)

/* Which Custom commands needs to be supported */
#define ZCL_ATTRIBUTE_READ_SERVER_SUPPORTED
#define ZCL_ATTRIBUTE_WRITE_SERVER_SUPPORTED

/* Configuring Attribute Reporting */
#define ZCL_ATTRIBUTE_REPORTING_SERVER_SUPPORTED
#define ZCL_CONFIGURE_ATTRIBUTE_REPORTING_SERVER_SUPPORTED
#define ZCL_READ_ATTRIBUTE_REPORTING_CONFIGURATION_SERVER_SUPPORTED
#define ZCL_SYSTEM_MIN_REPORT_INTERVAL 0
#define ZCL_SYSTEM_MAX_REPORT_INTERVAL 60

/* Reporting related configuration */
enum { REPORT_DEVICE_TEMPERATURE_CONFIGURATION_SLOT = 0, NUMBER_OF_REPORTS };

#define ZCL_NUMBER_OF_REPORTS NUMBER_OF_REPORTS
#define MIN_REPORT_INTERVAL   300
#define MAX_REPORT_INTERVAL   3600

#define CLD_BIND_SERVER
#define MAX_NUM_BIND_QUEUE_BUFFERS      ZCL_NUMBER_OF_REPORTS
#define MAX_PDU_BIND_QUEUE_PAYLOAD_SIZE 24

/* Enable wild card profile */
#define ZCL_ALLOW_WILD_CARD_PROFILE

/**
 * @brief Enable Cluster
 * @note  Enables clusters and their client or server instances
 */
#define CLD_BASIC
#define BASIC_SERVER

/* Fixing a build error
 * Due to an error in the SDK
 * JN-SW-4170/Components/ZCL/Devices/ZHA/Generic/Source/plug_control.c:157 */
#define DEVICE_TEMPERATURE_CONFIGURATION_SERVER

/**
 * @brief Basic Cluster
 * @note  Optional Attributes
 */
#define CLD_BAS_ATTR_APPLICATION_VERSION
#define CLD_BAS_ATTR_STACK_VERSION
#define CLD_BAS_ATTR_HARDWARE_VERSION
#define CLD_BAS_ATTR_MANUFACTURER_NAME
#define CLD_BAS_ATTR_MODEL_IDENTIFIER
#define CLD_BAS_ATTR_DATE_CODE
#define CLD_BAS_ATTR_SW_BUILD_ID

#define BAS_MANUF_NAME_STRING "NXP"
#define BAS_MODEL_ID_STRING   "openlumi.gw_router.jn5169"
#define BAS_DATE_STRING       BUILD_DATE_STRING
#define BAS_SW_BUILD_STRING   "1000-0001"

#define CLD_BAS_APP_VERSION      (1)
#define CLD_BAS_STACK_VERSION    (1)
#define CLD_BAS_HARDWARE_VERSION (1)
#define CLD_BAS_MANUF_NAME_SIZE  (3)
#define CLD_BAS_MODEL_ID_SIZE    (25)
#define CLD_BAS_DATE_SIZE        (8)
#define CLD_BAS_POWER_SOURCE     E_CLD_BAS_PS_SINGLE_PHASE_MAINS
#define CLD_BAS_SW_BUILD_SIZE    (9)

#define CLD_BAS_CMD_RESET_TO_FACTORY_DEFAULTS

#endif /* ZCL_OPTIONS_H */
