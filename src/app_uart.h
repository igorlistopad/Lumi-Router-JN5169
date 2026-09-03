/**
 * @file  app_uart.h
 * @brief UART interface
 */

#ifndef APP_UART_H
#define APP_UART_H

#include <jendefs.h>

PUBLIC void UART_vInit(void);
PUBLIC void UART_vRxIsr(void);
PUBLIC void UART_vWriteByte(uint8 u8Byte);
PUBLIC void UART_vWaitForTxComplete(void);

#endif /* APP_UART_H */
