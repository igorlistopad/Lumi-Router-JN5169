/**
 * @file  app_device_temperature.c
 * @brief Set of functions/task for read device temperature
 */

#include <jendefs.h>

/* Application */
#include "app_device_temperature.h"
#include "app_main.h"
#include "app_zcl_task.h"

/* SDK JN-SW-4170 */
#include "AppHardwareApi.h"
#include "ZTimer.h"
#include "dbg.h"

#ifndef TRACE_DEVICE_TEMPERATURE
#define TRACE_DEVICE_TEMPERATURE FALSE
#endif

#define DEVICE_TEMPERATURE_UPDATE_TIME ZTIMER_TIME_SEC(10)

PRIVATE void APP_vDeviceTemperatureUpdate(void);
PRIVATE int16 APP_i16GetDeviceTemperature(void);
PRIVATE int16 APP_i16ConvertChipTemp(uint16 u16AdcValue);

/**
 * @brief Init Device Temperature
 */
PUBLIC void APP_vDeviceTemperatureInit(void)
{
    vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
                     E_AHI_AP_INT_DISABLE,
                     E_AHI_AP_SAMPLE_8,
                     E_AHI_AP_CLOCKDIV_500KHZ,
                     E_AHI_AP_INTREF);

    while (!bAHI_APRegulatorEnabled())
        ;

    APP_vDeviceTemperatureUpdate();

    DBG_vPrintf(TRACE_DEVICE_TEMPERATURE, "APP: Init Device Temperature\n");

    /* Start the Device Temperature timer */
    ZTIMER_eStart(u8TimerDeviceTemperature, DEVICE_TEMPERATURE_UPDATE_TIME);
}

/**
 * @brief CallBack For Device Temperature Update timer
 */
PUBLIC void APP_cbTimerDeviceTemperatureUpdate(void *pvParam)
{
    APP_vDeviceTemperatureUpdate();
    ZTIMER_eStart(u8TimerDeviceTemperature, DEVICE_TEMPERATURE_UPDATE_TIME);
}

/**
 * @brief Device Temperature update
 */
PRIVATE void APP_vDeviceTemperatureUpdate(void)
{
    int16 i16DeviceTemperature = APP_i16GetDeviceTemperature();

    DBG_vPrintf(TRACE_DEVICE_TEMPERATURE, "APP: Temp = %d C\n", i16DeviceTemperature);

    sLumiRouter.sDeviceTemperatureConfigurationServerCluster.i16CurrentTemperature = i16DeviceTemperature;
}

/**
 * @brief Read Device Temperature
 */
PRIVATE int16 APP_i16GetDeviceTemperature(void)
{
    uint16 u16AdcTempSensor;

    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, E_AHI_ADC_SRC_TEMP);
    vAHI_AdcStartSample();

    while (bAHI_AdcPoll())
        ;

    u16AdcTempSensor = u16AHI_AdcRead();

    return APP_i16ConvertChipTemp(u16AdcTempSensor);
}

/**
 * @brief   Helper Function to convert 10bit ADC reading to degrees C
 * @details Formula: DegC = Typical DegC - ((Reading12 - Typ12) * ScaleFactor)
 *          Where C = 25 and temps sensor output 730mv at 25C (from datasheet)
 *          As we use 2Vref and 10bit adc this gives (730/2400)*4096  [=Typ12 =1210]
 *          Scale factor is half the 0.706 data-sheet resolution DegC/LSB (2Vref)
 */
PRIVATE int16 APP_i16ConvertChipTemp(uint16 u16AdcValue)
{
    return (int16)((int32)25 - ((((int32)(u16AdcValue * 4) - (int32)1210) * (int32)353) / (int32)1000));
}
