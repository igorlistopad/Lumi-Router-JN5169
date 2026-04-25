/****************************************************************************
 *
 * MODULE:               Lumi Router
 *
 * COMPONENT:            app_main.h
 *
 * DESCRIPTION:          Application main file
 *
 ****************************************************************************/

#ifndef APP_MAIN_H
#define APP_MAIN_H

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>

/* SDK JN-SW-4170 */
#include "ZQueue.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern PUBLIC uint8 u8TimerTick;
extern PUBLIC uint8 u8TimerRestart;
extern PUBLIC uint8 u8TimerDeviceTemperature;

extern PUBLIC tszQueue APP_msgBdbEvents;
extern PUBLIC tszQueue APP_msgAppEvents;
extern PUBLIC tszQueue APP_msgSerialTx;
extern PUBLIC tszQueue APP_msgSerialRx;

extern PUBLIC tszQueue zps_msgMlmeDcfmInd;
extern PUBLIC tszQueue zps_msgMcpsDcfmInd;
extern PUBLIC tszQueue zps_msgMcpsDcfm;
extern PUBLIC tszQueue zps_TimeEvents;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void APP_vMainLoop(void);
PUBLIC void APP_vSetUpHardware(void);
PUBLIC void APP_vInitResources(void);

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

#endif /* APP_MAIN_H */
