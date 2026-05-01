/**
 * @file  app_zcl_task.h
 * @brief ZCL Interface
 */

#ifndef APP_ZCL_TASK_H
#define APP_ZCL_TASK_H

#include <jendefs.h>

/* SDK JN-SW-4170 */
#include "Basic.h"
#include "DeviceTemperatureConfiguration.h"
#include "zcl.h"

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

extern PUBLIC APP_tsLumiRouter sLumiRouter;

PUBLIC void APP_ZCL_vInitialise(void);
PUBLIC void APP_ZCL_vEventHandler(ZPS_tsAfEvent *psStackEvent);
PUBLIC void APP_cbTimerZclTick(void *pvParam);

#endif /* APP_ZCL_TASK_H */
