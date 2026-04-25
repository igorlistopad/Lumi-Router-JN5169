/****************************************************************************
 *
 * MODULE:              Lumi Router
 *
 * COMPONENT:           app_zcl_task.h
 *
 * DESCRIPTION:         ZCL Interface
 *
 ****************************************************************************/

/****************************************************************************/
/* Description.                                                             */
/* If you do not need this file to be parsed by doxygen then delete @file   */
/****************************************************************************/

/** @file
 * Add brief description here.
 * Add more detailed description here
 */

/****************************************************************************/
/* Description End                                                          */
/****************************************************************************/

#ifndef APP_ZCL_TASK_H
#define APP_ZCL_TASK_H

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>

/* SDK JN-SW-4170 */
#include "Basic.h"
#include "DeviceTemperatureConfiguration.h"
#include "zcl.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct {
    tsZCL_ClusterInstance sBasicServer;
    tsZCL_ClusterInstance sDeviceTemperatureConfigurationServer;

} APP_tsLumiRouterClusterInstances __attribute__((aligned(4)));

typedef struct {
    tsZCL_EndPointDefinition sEndPoint;

    /* Cluster instances */
    APP_tsLumiRouterClusterInstances sClusterInstance;

    /* Basic Cluster - Server */
    tsCLD_Basic sBasicServerCluster;

    /* Device Temperature Configuration Cluster - Server */
    tsCLD_DeviceTemperatureConfiguration sDeviceTemperatureConfigurationServerCluster;

} APP_tsLumiRouter;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern PUBLIC APP_tsLumiRouter sLumiRouter;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void APP_ZCL_vInitialise(void);
PUBLIC void APP_ZCL_vEventHandler(ZPS_tsAfEvent *psStackEvent);
PUBLIC void APP_cbTimerZclTick(void *pvParam);

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

#endif /* APP_ZCL_TASK_H */
