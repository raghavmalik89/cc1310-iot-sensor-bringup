/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== rfEasyLinkEchoRx.c ========
 */
/* Standard C Libraries */
#include <stdlib.h>
#include <stdint.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/PIN.h>

/* Board Header files */
#include "Board.h"

/* Application Header files */
#include "smartrf_settings/smartrf_settings.h"
#include "telemetry_protocol.h"

/* EasyLink API Header files */
#include "easylink/EasyLink.h"

/* Uart Header files */
#include <ti/drivers/UART.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Undefine to not use async mode */
#define RFEASYLINKECHO_ASYNC

#define RFEASYLINKECHO_TASK_STACK_SIZE    1024
#define RFEASYLINKECHO_TASK_PRIORITY      2

Task_Struct echoTask;    /* not static so you can see in ROV */
static Task_Params echoTaskParams;
static uint8_t echoTaskStack[RFEASYLINKECHO_TASK_STACK_SIZE];

/* Pin driver handle */
static PIN_Handle pinHandle;
static PIN_State pinState;
/* UART handle */
static UART_Handle uart;
static UART_Params uartParams;

static volatile uint8_t g_payload[TELEMETRY_PAYLOAD_LENGTH] = {0};
static volatile int8_t  g_rssi = 0;
static volatile uint8_t g_len = 0;
static volatile bool g_packetReceived = false;

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config pinTable[] = {
    Board_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_PIN_LED2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

#ifdef RFEASYLINKECHO_ASYNC
static Semaphore_Handle echoDoneSem;
#endif //RFEASYLINKECHO_ASYNC


#ifdef RFEASYLINKECHO_ASYNC


void echoRxDoneCb(EasyLink_RxPacket * rxPacket, EasyLink_Status status)
{
   uint8_t i;

   if (status == EasyLink_Status_Success)
        {
            PIN_setOutputValue(pinHandle, Board_PIN_LED2,
                            !PIN_getOutputValue(Board_PIN_LED2));
            PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);

            g_rssi = rxPacket->rssi;
            g_len  = rxPacket->len;

            for (i = 0; (i < rxPacket->len) && (i < TELEMETRY_PAYLOAD_LENGTH); i++)
            {
                g_payload[i] = rxPacket->payload[i];
            }

            g_packetReceived = true;
        }
    else

    {
        /* Set LED1 and clear LED2 to indicate error */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1, 1);
        PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
    }

    Semaphore_post(echoDoneSem);
}
#endif //RFEASYLINKECHO_ASYNC

static void rfEasyLinkEchoRxFnx(UArg arg0, UArg arg1)
{
   // uint32_t absTime;

#ifdef RFEASYLINKECHO_ASYNC
    /* Create a semaphore for Async */
    Semaphore_Params params;
    Error_Block      eb;

    /* Init params */
    Semaphore_Params_init(&params);
    Error_init(&eb);

    /* Create semaphore instance */
    echoDoneSem = Semaphore_create(0, &params, &eb);
    if(echoDoneSem == NULL)
    {
        System_abort("Semaphore creation failed");
    }

#endif //RFEASYLINKECHO_ASYNC

    // Initialize the EasyLink parameters to their default values
    EasyLink_Params easyLink_params;
    EasyLink_Params_init(&easyLink_params);

    /*
     * Initialize EasyLink with the settings found in easylink_config.h
     * Modify EASYLINK_PARAM_CONFIG in easylink_config.h to change the default
     * PHY
     */
    if(EasyLink_init(&easyLink_params) != EasyLink_Status_Success)
    {
        System_abort("EasyLink_init failed");
    }

    /*
     * If you wish to use a frequency other than the default, use
     * the following API:
     * EasyLink_setFrequency(868000000);
     */

  while(1)
    {
    #ifdef RFEASYLINKECHO_ASYNC
        EasyLink_receiveAsync(echoRxDoneCb, 0);

        /* Wait indefinitely for Rx */
        Semaphore_pend(echoDoneSem, BIOS_WAIT_FOREVER);

        if (g_packetReceived)
        {
            TelemetryPacket telemetryPacket;
            char msg[96];
            int len;

            if (!telemetryProtocolDecodeTelemetry((const uint8_t *)g_payload, g_len, &telemetryPacket))
            {
                len = snprintf(msg, sizeof(msg),
                               "RX invalid payload LEN=%u RSSI=%d\r\n",
                               (unsigned int)g_len,
                               (int)g_rssi);
            }
            else if (!telemetryProtocolHasSupportedVersion(telemetryPacket.protocol_version))
            {
                len = snprintf(msg, sizeof(msg),
                               "RX unsupported version=0x%02X RSSI=%d LEN=%u\r\n",
                               (unsigned int)telemetryPacket.protocol_version,
                               (int)g_rssi,
                               (unsigned int)g_len);
            }
            else if (!telemetryProtocolIsTelemetryType(telemetryPacket.packet_type))
            {
                len = snprintf(msg, sizeof(msg),
                               "RX unexpected type=0x%02X RSSI=%d LEN=%u\r\n",
                               (unsigned int)telemetryPacket.packet_type,
                               (int)g_rssi,
                               (unsigned int)g_len);
            }
            else
            {
                len = snprintf(msg, sizeof(msg),
                               "VER=0x%02X TYPE=0x%02X ID=0x%02X FLAGS=0x%02X CNT=%u TEMP=%d HUM=%u BAT=%u RSSI=%d LEN=%u\r\n",
                               (unsigned int)telemetryPacket.protocol_version,
                               (unsigned int)telemetryPacket.packet_type,
                               (unsigned int)telemetryPacket.device_id,
                               (unsigned int)telemetryPacket.status_flags,
                               (unsigned int)telemetryPacket.packet_counter,
                               (int)telemetryPacket.temperature_centi_c,
                               (unsigned int)telemetryPacket.humidity_centi_rh,
                               (unsigned int)telemetryPacket.battery_mv,
                               (int)g_rssi,
                               (unsigned int)g_len);
            }

            UART_write(uart, msg, len);
            g_packetReceived = false;
        }
    #endif
   
    }
}

void echoTask_init(PIN_Handle inPinHandle) {
    pinHandle = inPinHandle;

    Task_Params_init(&echoTaskParams);
    echoTaskParams.stackSize = RFEASYLINKECHO_TASK_STACK_SIZE;
    echoTaskParams.priority = RFEASYLINKECHO_TASK_PRIORITY;
    echoTaskParams.stack = &echoTaskStack;
    echoTaskParams.arg0 = (UInt)1000000;

    Task_construct(&echoTask, rfEasyLinkEchoRxFnx, &echoTaskParams, NULL);
}

/*
 *  ======== main ========
 */
int main(void)
{
    /* Call driver init functions. */
    Board_initGeneral();

    /* UART Init*/
    UART_init();
    UART_Params_init(&uartParams);
    uartParams.baudRate = 115200;
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode  = UART_DATA_TEXT;
    uartParams.readReturnMode = UART_RETURN_NEWLINE;
    uartParams.readEcho = UART_ECHO_OFF;
    uart = UART_open(Board_UART0, &uartParams);
    Assert_isTrue(uart != NULL, NULL);

    /* Open LED pins */
    pinHandle = PIN_open(&pinState, pinTable);
    Assert_isTrue(pinHandle != NULL, NULL);

    /* Clear LED pins */
    PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
    PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);

    echoTask_init(pinHandle);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
