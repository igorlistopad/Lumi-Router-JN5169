/**
 * @file  app_uart.c
 * @brief UART interface
 */

#include <jendefs.h>

/* Application */
#include "app_main.h"
#include "app_uart.h"

/* SDK JN-SW-4170 */
#include "AppHardwareApi.h"
#include "ZQueue.h"
#include "dbg.h"

#ifndef TRACE_UART
#define TRACE_UART FALSE
#endif

#define UART           E_AHI_UART_0
#define MAX_TX_BUFFER  16
#define MAX_RX_BUFFER  64

PRIVATE uint8 au8UartHwTxFifo[MAX_TX_BUFFER];
PRIVATE uint8 au8UartHwRxFifo[MAX_RX_BUFFER];

/**
 * @brief Initialise the UART peripheral
 */
PUBLIC void UART_vInit(void)
{
    vAHI_UartSetRTSCTS(UART, FALSE);

    if (!bAHI_UartEnable(UART, au8UartHwTxFifo, MAX_TX_BUFFER, au8UartHwRxFifo, MAX_RX_BUFFER)) {
        DBG_vPrintf(TRACE_UART, "UART: Initialisation failed\n");
        return;
    }

    vAHI_UartReset(UART, TRUE, TRUE);
    vAHI_UartReset(UART, FALSE, FALSE);

    vAHI_UartSetBaudRate(UART, E_AHI_UART_RATE_115200);

    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);
    vAHI_UartSetInterrupt(UART, FALSE, FALSE, FALSE, TRUE, E_AHI_UART_FIFO_LEVEL_1);

    DBG_vPrintf(TRACE_UART, "UART: Initialised\n");
}

/**
 * @brief Handle interrupts from UART
 */
PUBLIC void UART_vIsr(void)
{
    uint8 u8Byte;
    uint8 u8InterruptId = (uint8)((u8AHI_UartReadInterruptStatus(UART) >> 1) & 0x07U);

    switch (u8InterruptId) {
    case E_AHI_UART_INT_RXDATA:
    case E_AHI_UART_INT_TIMEOUT:
        u8Byte = u8AHI_UartReadData(UART);
        if (!ZQ_bQueueSend(&APP_msgSerialRx, &u8Byte)) {
            DBG_vPrintf(TRACE_UART, "UART: RX queue full, byte dropped\n");
        }
        break;

    case E_AHI_UART_INT_TX:
        if (ZQ_bQueueReceive(&APP_msgSerialTx, &u8Byte)) {
            vAHI_UartWriteData(UART, u8Byte);
        }
        else {
            /* Prevent repeated TX-empty interrupts while the queue is empty. */
            UART_vSetTxInterrupt(FALSE);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Send the character
 */
PUBLIC void UART_vTxChar(uint8 u8Char)
{
    vAHI_UartWriteData(UART, u8Char);
}

/**
 * @brief Check if the UART transmitter is ready
 */
PUBLIC bool_t UART_bTxReady(void)
{
    return (bool_t)(u8AHI_UartReadLineStatus(UART) & E_AHI_UART_LS_THRE);
}

/**
 * @brief Enable / disable the tx interrupt
 */
PUBLIC void UART_vSetTxInterrupt(bool_t bState)
{
    vAHI_UartSetInterrupt(UART, FALSE, FALSE, bState, TRUE, E_AHI_UART_FIFO_LEVEL_1);
}
