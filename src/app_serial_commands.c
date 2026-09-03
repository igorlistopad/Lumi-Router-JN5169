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
#include "AppHardwareApi.h"
#include "PDM.h"
#include "ZQueue.h"
#include "dbg.h"

#ifndef TRACE_SERIAL
#define TRACE_SERIAL FALSE
#endif

#define SL_START_CHAR     0x01U
#define SL_ESC_CHAR       0x02U
#define SL_END_CHAR       0x03U
#define SL_HEADER_SIZE    4U
#define SL_FRAME_MIN_SIZE 5U
#define SL_FRAME_MAX_SIZE 6U
#define SL_RX_IDLE        0xFFU

#define SERIAL_RX_PROCESS_LIMIT 16U

/* Serial link message types */
enum {
    E_SC_MSG_RESET = 0x0011,
    E_SC_MSG_ERASE_PERSISTENT_DATA = 0x0012
};

PRIVATE void APP_vProcessRxChar(uint8 u8Char);
PRIVATE void APP_vProcessCommand(uint8 u8Command);

/**
 * @brief Process queued serial receive data
 */
PUBLIC void APP_vProcessSerialRx(void)
{
    uint8 u8RxByte;
    uint8 u8BytesProcessed = 0U;

    while ((u8BytesProcessed < SERIAL_RX_PROCESS_LIMIT) &&
           ZQ_bQueueReceive(&APP_msgSerialRx, &u8RxByte)) {
        APP_vProcessRxChar(u8RxByte);
        u8BytesProcessed++;
    }
}

/**
 * @brief Send a serial message and wait for transmission to complete
 */
PUBLIC void APP_vSendSerialMessage(const char *pcMessage)
{
    DBG_vPrintf(TRACE_SERIAL, "Serial: TX message=\"%s\"\n", pcMessage);

    for (; *pcMessage != '\0'; pcMessage++) {
        UART_vWriteByte((uint8)*pcMessage);
    }

    UART_vWaitForTxComplete();
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
 * @brief Process a decoded jntool command.
 */
PRIVATE void APP_vProcessCommand(uint8 u8Command)
{
    DBG_vPrintf(TRACE_SERIAL, "Serial: RX command=%02x\n", u8Command);

    switch (u8Command) {
    case E_SC_MSG_RESET:
        APP_vSendSerialMessage("Reset...........");
        vAHI_SwReset();
        break;

    case E_SC_MSG_ERASE_PERSISTENT_DATA:
        APP_vSendSerialMessage("Erase PDM.......");
        APP_vSendSerialMessage("Reset...........");
        PDM_vDeleteAllDataRecords();
        vAHI_SwReset();
        break;

    default:
        APP_vSendSerialMessage("Unknown command.");
        break;
    }
}
