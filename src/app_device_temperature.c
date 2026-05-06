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

#define DEVICE_TEMPERATURE_UPDATE_TIME ZTIMER_TIME_SEC(60)

PRIVATE void APP_vDeviceTemperatureUpdate(void);
PRIVATE int16 APP_i16GetDeviceTemperature(void);
PRIVATE int16 APP_i16ConvertChipTemp(uint16 u16AdcValue);

/**
 * @brief Initialise the device temperature sensor
 */
PUBLIC void APP_vDeviceTemperatureInit(void)
{
    /* Configure analogue peripherals */
    vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
                     E_AHI_AP_INT_DISABLE,
                     E_AHI_AP_SAMPLE_2,
                     E_AHI_AP_CLOCKDIV_500KHZ,
                     E_AHI_AP_INTREF);

    /* Wait for analogue regulator to start */
    while (!bAHI_APRegulatorEnabled())
        ;

    DBG_vPrintf(TRACE_DEVICE_TEMPERATURE, "APP: Init Device Temperature\n");

    /* Perform the first temperature reading immediately so that
     * the ZCL attribute i16CurrentTemperature is valid from
     * application start */
    APP_vDeviceTemperatureUpdate();

    /* Start the Device Temperature periodic update timer */
    ZTIMER_eStart(u8TimerDeviceTemperature, DEVICE_TEMPERATURE_UPDATE_TIME);
}

/**
 * @brief Callback for Device Temperature periodic update timer
 */
PUBLIC void APP_cbTimerDeviceTemperatureUpdate(void *pvParam)
{
    APP_vDeviceTemperatureUpdate();
    ZTIMER_eStart(u8TimerDeviceTemperature, DEVICE_TEMPERATURE_UPDATE_TIME);
}

/**
 * @brief Update the device temperature attribute
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

    /* Enable the ADC for a single conversion */
    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_1, E_AHI_ADC_SRC_TEMP);
    
    /* Trigger the ADC conversion */
    vAHI_AdcStartSample();

    /* Wait for the conversion to complete */
    while (bAHI_AdcPoll())
        ;

    /* Read the 10-bit ADC result */
    u16AdcTempSensor = u16AHI_AdcRead();

    /* Disable the ADC to save power */
    vAHI_AdcDisable();

    return APP_i16ConvertChipTemp(u16AdcTempSensor);
}

/**
 * @brief   Convert 10-bit ADC reading to degrees Celsius
 * @details Formula: T = 25 - (ADC10 - Typ10) * Scale10 / 1000
 *
 *          Typ10 = 614: 10-bit ADC reading at 25°C
 *            V_sensor(25°C) = 720 mV (JN5169 datasheet Table 31)
 *            ADC range = Vref_int = 1200 mV
 *            Typ10 = (720 / 1200) × 1024 = 614.4 → 614
 *
 *          Scale10 = 706: temperature change per ADC10 LSB, in 0.001 °C
 *            1 LSB = 1200 / 1024 = 1.171875 mV
 *            Sensor coefficient = −1.66 mV/°C (JN5169 datasheet Table 31)
 *            Scale10 = (1.171875 / 1.66) * 1000 = 705.94 -> 706
 *
 *          Max formula error: +/-1 °C (integer truncation)
 *          Sensor accuracy: +/-7 °C (JN5169 datasheet Table 31)
 */
PRIVATE int16 APP_i16ConvertChipTemp(uint16 u16AdcValue)
{
    return (int16)((int32)25 - ((((int32)u16AdcValue - (int32)614) * (int32)706) / (int32)1000));
}
