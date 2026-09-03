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

#define UART               E_AHI_UART_0
#define UART_TX_FIFO_SIZE  16U
#define UART_RX_FIFO_SIZE  64U

PRIVATE uint8 au8UartTxFifo[UART_TX_FIFO_SIZE];
PRIVATE uint8 au8UartRxFifo[UART_RX_FIFO_SIZE];

/**
 * @brief Initialise the UART interface
 */
PUBLIC void UART_vInit(void)
{
    vAHI_UartSetRTSCTS(UART, FALSE);

    if (!bAHI_UartEnable(UART, au8UartTxFifo, UART_TX_FIFO_SIZE, au8UartRxFifo, UART_RX_FIFO_SIZE)) {
        DBG_vPrintf(TRACE_UART, "UART: Initialisation failed\n");
        return;
    }

    vAHI_UartReset(UART, E_AHI_UART_TX_RESET, E_AHI_UART_RX_RESET);
    vAHI_UartReset(UART, E_AHI_UART_TX_ENABLE, E_AHI_UART_RX_ENABLE);

    vAHI_UartSetBaudRate(UART, E_AHI_UART_RATE_115200);

    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);
    vAHI_UartSetInterrupt(UART, FALSE, FALSE, FALSE, TRUE, E_AHI_UART_FIFO_LEVEL_1);

    DBG_vPrintf(TRACE_UART, "UART: Initialised\n");
}

/**
 * @brief Handle UART receive interrupts
 */
PUBLIC void UART_vRxIsr(void)
{
    uint8 u8Byte = u8AHI_UartReadData(UART);

    if (!ZQ_bQueueSend(&APP_msgSerialRx, &u8Byte)) {
        DBG_vPrintf(TRACE_UART, "UART: RX queue full, byte dropped\n");
    }
}

/**
 * @brief Wait for available TX FIFO space and write one byte
 */
PUBLIC void UART_vWriteByte(uint8 u8Byte)
{
    while (u16AHI_UartReadTxFifoLevel(UART) >= UART_TX_FIFO_SIZE)
        ;

    vAHI_UartWriteData(UART, u8Byte);
}

/**
 * @brief Wait until UART transmission is complete
 */
PUBLIC void UART_vWaitForTxComplete(void)
{
    while ((u8AHI_UartReadLineStatus(UART) & E_AHI_UART_LS_TEMT) == 0U)
        ;
}
