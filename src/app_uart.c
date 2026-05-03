/**
 * @file  app_uart.c
 * @brief UART interface
 */

#include <jendefs.h>
#include <stdlib.h>

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
#define UART_BAUD_RATE 115200
#define UART_START_ADR 0x02003000UL
#define MAX_TX_BUFFER  16
#define MAX_RX_BUFFER  255

PRIVATE void UART_vSetBaudRate(uint32 u32BaudRate);

PRIVATE uint8 au8UartHwTxFifo[MAX_TX_BUFFER];
PRIVATE uint8 au8UartHwRxFifo[MAX_RX_BUFFER];

/**
 * @brief Initialising UART
 */
PUBLIC void UART_vInit(void)
{
    DBG_vPrintf(TRACE_UART, "Initialising UART... ");

    vAHI_UartSetRTSCTS(UART, FALSE);

    bAHI_UartEnable(UART, au8UartHwTxFifo, MAX_TX_BUFFER, au8UartHwRxFifo, MAX_RX_BUFFER);

    vAHI_UartReset(UART, TRUE, TRUE);
    vAHI_UartReset(UART, FALSE, FALSE);

    /* Set the clock divisor register to give required baud, this has to be done
       directly as the normal routines (in ROM) do not support all baud rates */
    UART_vSetBaudRate(UART_BAUD_RATE);

    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);
    vAHI_UartSetInterrupt(UART, FALSE, FALSE, FALSE, TRUE, E_AHI_UART_FIFO_LEVEL_1);

    DBG_vPrintf(TRACE_UART, "Done\n");
}

/**
 * @brief Handle interrupts from uart
 */
PUBLIC void APP_isrUart(void)
{
    uint32 u32ItemBitmap = ((*((volatile uint32 *)(UART_START_ADR + 0x08))) >> 1) & 0x0007;
    uint8 u8Byte;

    if (u32ItemBitmap & E_AHI_UART_INT_RXDATA) {
        u8Byte = u8AHI_UartReadData(UART);
        if (!ZQ_bQueueSend(&APP_msgSerialRx, &u8Byte)) {
            /* Queue full - assert RTS to stop sender and drain HW FIFO */
            UART_vRtsStopFlow();
            u8AHI_UartReadData(UART);
        }
    }
    else if (u32ItemBitmap & E_AHI_UART_INT_TX) {
        if (ZQ_bQueueReceive(&APP_msgSerialTx, &u8Byte)) {
            UART_vSetTxInterrupt(TRUE);
            vAHI_UartWriteData(UART, u8Byte);
        }
        else {
            /* disable tx interrupt as nothing to send */
            UART_vSetTxInterrupt(FALSE);
        }
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
 * @brief Read line status
 */
PUBLIC bool_t UART_bTxReady()
{
    return u8AHI_UartReadLineStatus(UART) & E_AHI_UART_LS_THRE;
}

/**
 * @brief Enable / disable the tx interrupt
 */
PUBLIC void UART_vSetTxInterrupt(bool_t bState)
{
    vAHI_UartSetInterrupt(UART, FALSE, FALSE, bState, TRUE, E_AHI_UART_FIFO_LEVEL_1);
}

/**
 * @brief Set UART RS-232 RTS line low to allow further data
 */
PUBLIC void UART_vRtsStartFlow(void)
{
    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, E_AHI_UART_RTS_LOW);
}

/**
 * @brief Set UART RS-232 RTS line high to stop any further data coming in
 */
PUBLIC void UART_vRtsStopFlow(void)
{
    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, E_AHI_UART_RTS_HIGH);
}

/**
 * @brief Set baud rates UART
 */
PRIVATE void UART_vSetBaudRate(uint32 u32BaudRate)
{
    uint16 u16Divisor = 0;
    uint32 u32Remainder;
    uint8 u8ClocksPerBit = 16;
    uint32 u32CalcBaudRate = 0;
    int32 i32BaudError = 0x7FFFFFFF;

    while (abs(i32BaudError) > (int32)(u32BaudRate >> 4)) {
        if (--u8ClocksPerBit < 3) {
            return;
        }

        /* Calculate Divisor register = 16MHz / (16 x baud rate) */
        u16Divisor = (uint16)(16000000UL / ((u8ClocksPerBit + 1) * u32BaudRate));

        /* Correct for rounding errors */
        u32Remainder = (uint32)(16000000UL % ((u8ClocksPerBit + 1) * u32BaudRate));

        if (u32Remainder >= (((u8ClocksPerBit + 1) * u32BaudRate) / 2)) {
            u16Divisor += 1;
        }

        u32CalcBaudRate = (16000000UL / ((u8ClocksPerBit + 1) * u16Divisor));

        i32BaudError = (int32)u32CalcBaudRate - (int32)u32BaudRate;
    }

    /* Set the calculated clocks per bit */
    vAHI_UartSetClocksPerBit(UART, u8ClocksPerBit);

    /* Set the calculated divisor */
    vAHI_UartSetBaudDivisor(UART, u16Divisor);
}
