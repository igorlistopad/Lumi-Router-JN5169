/**
 * @file  app_uart.h
 * @brief UART interface
 */

#ifndef APP_UART_H
#define APP_UART_H

#include <jendefs.h>

PUBLIC void UART_vInit(void);
PUBLIC void APP_isrUart(void);
PUBLIC void UART_vTxChar(uint8 u8TxChar);
PUBLIC bool_t UART_bTxReady(void);
PUBLIC void UART_vSetTxInterrupt(bool_t bState);

#endif /* APP_UART_H */
