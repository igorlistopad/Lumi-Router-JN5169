/**
 * @file  app_device_temperature.c
 * @brief Set of functions/task for reading device temperature
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
 * @brief Callback For Device Temperature Update timer
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
 * @brief   Convert 10-bit ADC reading to degrees Celsius
 * @details Formula: T = 25 - ((ADC12 - Typ12) * Scale) / 1000
 *
 *          ADC12 = u16AdcValue * 4 (10-bit left-aligned to 12-bit)
 *
 *          Typ12 = 1245: 12-bit ADC reading at 25°C
 *          V_sensor(25°C) = 730 mV (JN5169 datasheet, typical)
 *          ADC range = 2 × Vref_int = 2 × 1.2V = 2.4V
 *          Typ12 = (730 / 2400) × 4096 = 1245
 *
 *          Scale = 283: temperature change per ADC12 LSB, in 0.001 °C
 *          1 LSB = 2400 / 4096 = 0.5859 mV
 *          Sensor coefficient = −2.07 mV/°C (JN5169 datasheet, typical)
 *          Scale = 0.5859 / 2.07 = 0.283 °C/LSB
 *
 *          Note: V_25 and Tc are typical values and vary per chip (±3-5°C accuracy).
 */
PRIVATE int16 APP_i16ConvertChipTemp(uint16 u16AdcValue)
{
    return (int16)((int32)25 - ((((int32)(u16AdcValue * 4) - (int32)1245) * (int32)283) / (int32)1000));
}
