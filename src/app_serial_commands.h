/**
 * @file  app_serial_commands.h
 * @brief Serial Commands
 */

#ifndef APP_SERIAL_COMMANDS_H
#define APP_SERIAL_COMMANDS_H

#include <jendefs.h>

PUBLIC void APP_taskAtSerial(void);
PUBLIC void APP_WriteMessageToSerial(const char *message);

#endif /* APP_SERIAL_COMMANDS_H */
