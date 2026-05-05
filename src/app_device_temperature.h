/**
 * @file  app_device_temperature.h
 * @brief Set of functions/task for reading device temperature
 */

#ifndef APP_DEVICE_TEMPERATURE_H
#define APP_DEVICE_TEMPERATURE_H

#include <jendefs.h>

PUBLIC void APP_vDeviceTemperatureInit(void);
PUBLIC void APP_cbTimerDeviceTemperatureUpdate(void *pvParam);

#endif /* APP_DEVICE_TEMPERATURE_H */
