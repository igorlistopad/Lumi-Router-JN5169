/**
 * @file  app_serial_commands.c
 * @brief Serial Commands
 */

#include <jendefs.h>

/* Application */
#include "app_main.h"
#include "app_serial_commands.h"
#include "app_uart.h"

/* SDK JN-SW-4170 */
#include "PDM.h"
#include "ZQueue.h"
#include "ZTimer.h"
#include "dbg.h"
#include "portmacro.h"

#ifndef TRACE_SERIAL
#define TRACE_SERIAL FALSE
#endif

#define SL_START_CHAR    0x01
#define SL_ESC_CHAR      0x02
#define SL_END_CHAR      0x03
#define SL_HEADER_SIZE    4U
#define SL_FRAME_MIN_SIZE 5U
#define SL_FRAME_MAX_SIZE 6U
#define SL_RX_IDLE        0xFFU
#define RESTART_DELAY_MS ZTIMER_TIME_MSEC(50)

/* Serial link message types */
enum {
    E_SC_MSG_RESET = 0x0011,
    E_SC_MSG_ERASE_PERSISTENT_DATA = 0x0012
};

PRIVATE void APP_vProcessRxChar(uint8 u8Char);
PRIVATE void APP_vProcessCommand(uint8 u8Command);
PRIVATE void APP_vWriteTxChar(uint8 u8Char);

PRIVATE uint32 u32CriticalSectionStorage;

/**
 * @brief Task that obtains a message from the serial Rx message queue.
 */
PUBLIC void APP_taskAtSerial(void)
{
    uint8 u8RxByte;
    while (ZQ_bQueueReceive(&APP_msgSerialRx, &u8RxByte)) {
        APP_vProcessRxChar(u8RxByte);
    }
}

/**
 * @brief Write message to the serial link
 */
PUBLIC void APP_WriteMessageToSerial(const char *message)
{
    DBG_vPrintf(TRACE_SERIAL, "Serial: TX message=\"%s\"\n", message);

    for (; *message != '\0'; message++) {
        APP_vWriteTxChar((uint8)*message);
    }
}

/**
 * @brief Process a byte from a jntool serial command.
 * @details Wire frame:
 * START | escaped(type[2], length[2], checksum, data[0..1]) | END.
 * Bytes below 0x10 are escaped as ESC followed by the byte XOR 0x10.
 * Valid frames are passed to APP_vProcessCommand().
 */
PRIVATE void APP_vProcessRxChar(uint8 u8Char)
{
    static uint8 au8Header[SL_HEADER_SIZE];
    static uint8 u8FrameLength = SL_RX_IDLE;
    static uint8 u8Checksum;
    static bool_t bEscaped = FALSE;

    if (u8Char == SL_START_CHAR) {
        u8FrameLength = 0U;
        u8Checksum = 0U;
        bEscaped = FALSE;
        return;
    }

    if (u8FrameLength == SL_RX_IDLE) {
        return;
    }

    if (u8Char == SL_END_CHAR) {
        if (!bEscaped &&
            (u8FrameLength >= SL_FRAME_MIN_SIZE) &&
            (u8Checksum == 0U) &&
            (au8Header[0] == 0U) &&
            (au8Header[2] == 0U) &&
            (au8Header[3] == (u8FrameLength - SL_FRAME_MIN_SIZE))) {
            APP_vProcessCommand(au8Header[1]);
        }

        u8FrameLength = SL_RX_IDLE;
        bEscaped = FALSE;
        return;
    }

    if (u8Char == SL_ESC_CHAR) {
        bEscaped = TRUE;
        return;
    }

    if (bEscaped) {
        u8Char ^= 0x10U;
        bEscaped = FALSE;
    }

    if (u8FrameLength >= SL_FRAME_MAX_SIZE) {
        u8FrameLength = SL_RX_IDLE;
        bEscaped = FALSE;
        return;
    }

    if (u8FrameLength < SL_HEADER_SIZE) {
        au8Header[u8FrameLength] = u8Char;
    }

    /* A valid frame XOR is zero. */
    u8Checksum ^= u8Char;
    u8FrameLength++;
}

/**
 * @brief Execute a decoded jntool command.
 */
PRIVATE void APP_vProcessCommand(uint8 u8Command)
{
    DBG_vPrintf(TRACE_SERIAL, "Serial: RX command=%02x\n", u8Command);

    switch (u8Command) {
    case E_SC_MSG_RESET:
        APP_WriteMessageToSerial("Reset...........");
        ZTIMER_eStart(u8TimerRestart, RESTART_DELAY_MS);
        break;

    case E_SC_MSG_ERASE_PERSISTENT_DATA:
        APP_WriteMessageToSerial("Erase PDM.......");
        PDM_vDeleteAllDataRecords();
        APP_WriteMessageToSerial("Reset...........");
        ZTIMER_eStart(u8TimerRestart, RESTART_DELAY_MS);
        break;

    default:
        APP_WriteMessageToSerial("Unknown command.");
        break;
    }
}

/**
 * @brief Write byte to the serial link
 */
PRIVATE void APP_vWriteTxChar(uint8 u8Char)
{
    bool_t bByteDropped = FALSE;

    ZPS_eEnterCriticalSection(NULL, &u32CriticalSectionStorage);

    if (UART_bTxReady() && ZQ_bQueueIsEmpty(&APP_msgSerialTx)) {
        /* send byte now and enable irq */
        UART_vTxChar(u8Char);
        UART_vSetTxInterrupt(TRUE);
    }
    else {
        if (!ZQ_bQueueSend(&APP_msgSerialTx, &u8Char)) {
            bByteDropped = TRUE;
        }
    }

    ZPS_eExitCriticalSection(NULL, &u32CriticalSectionStorage);

    if (bByteDropped) {
        DBG_vPrintf(TRACE_SERIAL, "Serial: TX queue full, byte dropped\n");
    }
}
