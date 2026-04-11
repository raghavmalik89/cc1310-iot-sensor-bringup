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
 *  ======== rfEasyLinkEchoTx.c ========
 */
/* Standard C Libraries */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

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
#include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>

/* Board Header files */
#include "Board.h"

/* Application Header files */
#include "smartrf_settings/smartrf_settings.h"
#include "aht2x.h"
#include "telemetry_protocol.h"

/* EasyLink API Header files */
#include "easylink/EasyLink.h"

/* Undefine to not use async mode */
#define RFEASYLINKECHO_ASYNC

#define RFEASYLINKECHO_TASK_STACK_SIZE    1024
#define RFEASYLINKECHO_TASK_PRIORITY      2

#define TX_DEVICE_ID                      0xA1u

Task_Struct echoTask;    /* not static so you can see in ROV */
static Task_Params echoTaskParams;
static uint8_t echoTaskStack[RFEASYLINKECHO_TASK_STACK_SIZE];

/* Pin driver handle */
static PIN_Handle pinHandle;
static PIN_State pinState;

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config pinTable[] = {
    Board_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_PIN_LED2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// static uint16_t seqNumber;

/* User added Global Variables */

static uint16_t packetCounter = 0;
static bool aht2xReady = false;
static AHT2x aht2xDevice;
static I2C_Handle i2cHandle;

static int16_t scaleTemperatureToCentiC(float temperatureC)
{
    float scaledValue;

    if (temperatureC > 327.67f)
    {
        return INT16_MAX;
    }

    if (temperatureC < -327.68f)
    {
        return INT16_MIN;
    }

    scaledValue = temperatureC * 100.0f;

    if (scaledValue >= 0.0f)
    {
        scaledValue += 0.5f;
    }
    else
    {
        scaledValue -= 0.5f;
    }

    return (int16_t)scaledValue;
}

static uint16_t scaleHumidityToCentiRh(float humidityPercent)
{
    float scaledValue;

    if (humidityPercent <= 0.0f)
    {
        return 0u;
    }

    if (humidityPercent >= 100.0f)
    {
        return 10000u;
    }

    scaledValue = (humidityPercent * 100.0f) + 0.5f;
    return (uint16_t)scaledValue;
}

static void initAht2xSensor(void)
{
    I2C_Params i2cParams;

    I2C_init();
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_100kHz;

    i2cHandle = I2C_open(Board_I2C0, &i2cParams);
    if (i2cHandle == NULL)
    {
        aht2xReady = false;
        return;
    }

    AHT2x_init(&aht2xDevice, i2cHandle, AHT2X_I2C_ADDRESS);
    aht2xReady = (AHT2x_ensureCalibrated(&aht2xDevice) == AHT2X_STATUS_OK);
}


#ifdef RFEASYLINKECHO_ASYNC
static Semaphore_Handle echoDoneSem;
#endif //RFEASYLINKECHO_ASYNC

EasyLink_TxPacket txPacket = {{0}, 0, 0, {0}};

bool isPacketCorrect(EasyLink_RxPacket *rxp, EasyLink_TxPacket *txp)
{
    uint16_t i;
    bool status = true;

    for(i = 0; i < rxp->len; i++)
    {
        if(rxp->payload[i] != txp->payload[i])
        {
            status = false;
            break;
        }
    }
    return(status);
}

#ifdef RFEASYLINKECHO_ASYNC
void echoTxDoneCb(EasyLink_Status status)
{
    if (status == EasyLink_Status_Success)
    {
        /* Toggle LED1 to indicate TX */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1,!PIN_getOutputValue(Board_PIN_LED1));
        /* Turn LED2 off, in case there was a prior error */
        PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
    }
    else
    {
        /* Set both LED1 and LED2 to indicate error */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1, 1);
        PIN_setOutputValue(pinHandle, Board_PIN_LED2, 1);
    }

    Semaphore_post(echoDoneSem);
}

void echoRxDoneCb(EasyLink_RxPacket * rxPacket, EasyLink_Status status)
{
    if ((status == EasyLink_Status_Success) &&
            (isPacketCorrect(rxPacket, &txPacket)))
    {
        /* Toggle LED1, clear LED2 to indicate Echo RX */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1,!PIN_getOutputValue(Board_PIN_LED1));
        PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);
    }
    else if (status == EasyLink_Status_Aborted)
    {
        /* Set LED2 and clear LED1 to indicate Abort */
        PIN_setOutputValue(pinHandle, Board_PIN_LED2, 1);
        PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
    }
    else
    {
        /* Set both LED1 and LED2 to indicate error */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1, 1);
        PIN_setOutputValue(pinHandle, Board_PIN_LED2, 1);
    }

    Semaphore_post(echoDoneSem);
}
#endif //RFEASYLINKECHO_ASYNC

static void rfEasyLinkEchoTxFnx(UArg arg0, UArg arg1)
{
    uint32_t absTime;
    TelemetryPacket telemetryPacket;
    AHT2x_Status sensorStatus;
    float temperatureC = 0.0f;
    float humidityPercent = 0.0f;

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

#else
    EasyLink_RxPacket rxPacket = {{0}, 0, 0, 0, 0, {0}};
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

    initAht2xSensor();

    /*
     * If you wish to use a frequency other than the default, use
     * the following API:
     * EasyLink_setFrequency(868000000);
     */

    // Packet Originator
while(1) {
        telemetryPacket.status_flags = 0u;

        if (aht2xReady)
        {
            sensorStatus = AHT2x_readTemperatureHumidity(&aht2xDevice,
                                                         &temperatureC,
                                                         &humidityPercent);
        }
        else
        {
            sensorStatus = AHT2X_STATUS_I2C_ERROR;
        }

        telemetryPacket.protocol_version = TELEMETRY_PROTOCOL_VERSION;
        telemetryPacket.packet_type = TELEMETRY_PACKET_TYPE_TELEMETRY;
        telemetryPacket.device_id = TX_DEVICE_ID;
        telemetryPacket.packet_counter = packetCounter++;
        telemetryPacket.battery_mv = 0u;

        if (sensorStatus == AHT2X_STATUS_OK)
        {
            telemetryPacket.status_flags |= TELEMETRY_STATUS_SENSOR_DATA_VALID;
            telemetryPacket.temperature_centi_c = scaleTemperatureToCentiC(temperatureC);
            telemetryPacket.humidity_centi_rh = scaleHumidityToCentiRh(humidityPercent);
        }
        else
        {
            telemetryPacket.status_flags |= TELEMETRY_STATUS_AHT2X_READ_FAULT;
            telemetryPacket.temperature_centi_c = 0;
            telemetryPacket.humidity_centi_rh = 0u;
        }

        telemetryProtocolEncodeTelemetry(txPacket.payload, &telemetryPacket);

        txPacket.len = TELEMETRY_PAYLOAD_LENGTH;
        txPacket.dstAddr[0] = 0xaa;

        if(EasyLink_getAbsTime(&absTime) != EasyLink_Status_Success)
        {
            // optional error handling
        }

        txPacket.absTime = absTime + EasyLink_ms_To_RadioTime(1000);

        EasyLink_transmitAsync(&txPacket, echoTxDoneCb);
        Semaphore_pend(echoDoneSem, BIOS_WAIT_FOREVER);

        Task_sleep(1000 * 1000 / Clock_tickPeriod);
    }
}

void echoTask_init(PIN_Handle inPinHandle) {
    pinHandle = inPinHandle;

    Task_Params_init(&echoTaskParams);
    echoTaskParams.stackSize = RFEASYLINKECHO_TASK_STACK_SIZE;
    echoTaskParams.priority = RFEASYLINKECHO_TASK_PRIORITY;
    echoTaskParams.stack = &echoTaskStack;
    echoTaskParams.arg0 = (UInt)1000000;

    Task_construct(&echoTask, rfEasyLinkEchoTxFnx, &echoTaskParams, NULL);
}

/*
 *  ======== main ========
 */
int main(void)
{
    /* Call driver init functions. */
    Board_initGeneral();

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
