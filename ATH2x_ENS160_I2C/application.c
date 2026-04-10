/*
 *  ======== application.c ========
 *
 *  CC1310 IoT Sensor Bring-up (AHT2x + ENS160)
 *
 *  Author: Raghav Malik
 *
 *  Description:
 *  This file implements the application layer for sensor bring-up and validation
 *  on the TI CC1310 platform. It integrates AHT2x (temperature/humidity) and
 *  ENS160 (air quality) sensors over I2C and outputs measurements via UART.
 *
 *  Scope:
 *  - Peripheral initialization (GPIO, UART, I2C)
 *  - Sensor initialization and configuration
 *  - Periodic data acquisition
 *  - UART-based debugging and validation output
 *
 *  Notes:
 *  - This is development-stage firmware intended for bring-up and testing.
 *  - Focus is on system validation, not production optimization.
 *  - Timing and sequencing are critical for stable sensor readings.
 *
 *  Last Updated:
 *  - Initial bring-up completed: [09-04-2026]
 *
 */

/* For usleep() */
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Driver Header files */
#include <ti/drivers/UART.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

/* Board Header file */
#include "Board.h"
#include "aht2x.h"
#include "ens160.h"

static void uartPrint(UART_Handle uart, const char *msg)
{
    UART_write(uart, msg, strlen(msg));
}

static void uartPrintHex(UART_Handle uart, uint8_t value)
{
    char hex[] = "0123456789ABCDEF";
    char msg[6];

    msg[0] = '0';
    msg[1] = 'x';
    msg[2] = hex[(value >> 4) & 0x0F];
    msg[3] = hex[value & 0x0F];
    msg[4] = '\r';
    msg[5] = '\n';

    UART_write(uart, msg, sizeof(msg));
}

static void appendChar(char *buffer, size_t *index, char value)
{
    buffer[*index] = value;
    (*index)++;
}

static void appendString(char *buffer, size_t *index, const char *text)
{
    while (*text != '\0') {
        appendChar(buffer, index, *text);
        text++;
    }
}

static void appendUnsigned(char *buffer, size_t *index, uint32_t value)
{
    char digits[10];
    size_t count = 0U;

    if (value == 0U) {
        appendChar(buffer, index, '0');
        return;
    }

    while (value > 0U) {
        digits[count] = (char)('0' + (value % 10U));
        value /= 10U;
        count++;
    }

    while (count > 0U) {
        count--;
        appendChar(buffer, index, digits[count]);
    }
}

static void appendSignedFixed2(char *buffer, size_t *index, float value)
{
    uint32_t wholePart;
    uint32_t fractionalPart;
    float scaledValue;

    if (value < 0.0f) {
        appendChar(buffer, index, '-');
        value = -value;
    }

    scaledValue = (value * 100.0f) + 0.5f;
    wholePart = (uint32_t)(scaledValue / 100.0f);
    fractionalPart = (uint32_t)scaledValue % 100U;

    appendUnsigned(buffer, index, wholePart);
    appendChar(buffer, index, '.');
    appendChar(buffer, index, (char)('0' + (fractionalPart / 10U)));
    appendChar(buffer, index, (char)('0' + (fractionalPart % 10U)));
}

static const char *aht2xStatusToString(AHT2x_Status status)
{
    switch (status) {
        case AHT2X_STATUS_OK:
            return "OK";
        case AHT2X_STATUS_INVALID_ARG:
            return "INVALID_ARG";
        case AHT2X_STATUS_I2C_ERROR:
            return "I2C_ERROR";
        case AHT2X_STATUS_TIMEOUT:
            return "TIMEOUT";
        case AHT2X_STATUS_BUSY:
            return "BUSY";
        default:
            return "UNKNOWN";
    }
}

static const char *ens160StatusToString(ENS160_Status status)
{
    switch (status) {
        case ENS160_STATUS_OK:
            return "OK";
        case ENS160_STATUS_INVALID_ARG:
            return "INVALID_ARG";
        case ENS160_STATUS_I2C_ERROR:
            return "I2C_ERROR";
        default:
            return "UNKNOWN";
    }
}

static void uartPrintMeasurement(UART_Handle uart,
                                 float temperatureC,
                                 float humidityPercent)
{
    char msg[64];
    size_t index = 0U;

    appendString(msg, &index, "AHT2x T=");
    appendSignedFixed2(msg, &index, temperatureC);
    appendString(msg, &index, " C RH=");
    appendSignedFixed2(msg, &index, humidityPercent);
    appendString(msg, &index, " %\r\n");

    UART_write(uart, msg, index);
}

static void uartPrintEns160Measurement(UART_Handle uart,
                                       uint8_t aqi,
                                       uint16_t tvoc,
                                       uint16_t eco2)
{
    char msg[64];
    size_t index = 0U;

    appendString(msg, &index, "ENS160 AQI=");
    appendUnsigned(msg, &index, aqi);
    appendString(msg, &index, " TVOC=");
    appendUnsigned(msg, &index, tvoc);
    appendString(msg, &index, "ppb eCO2=");
    appendUnsigned(msg, &index, eco2);
    appendString(msg, &index, "ppm\r\n");

    UART_write(uart, msg, index);
}

/*
 *  ======== mainThread ========
 */

 /*
 * System Flow:
 * 1. Initialize peripherals
 * 2. Initialize sensors
 * 3. Validate communication
 * 4. Enter measurement loop
 *
 * Design Note:
 * UART is used as primary observability tool during bring-up.
 */
 
void *mainThread(void *arg0)
{
    UART_Handle uart;
    UART_Params uartParams;

    I2C_Handle i2c;
    I2C_Params i2cParams;
    AHT2x aht2x;
    ENS160 ens160;
    AHT2x_Status sensorStatus;
    ENS160_Status ens160Status;
    float temperatureC;
    float humidityPercent;
    uint8_t aqi;
    uint16_t tvoc;
    uint16_t eco2;

    GPIO_init();
    UART_init();
    I2C_init();

    UART_Params_init(&uartParams);
    uartParams.baudRate = 115200;

    uart = UART_open(Board_UART0, &uartParams);
    if (uart == NULL) {
        while (1) {}
    }

    uartPrint(uart, "CC1310 UART OK\r\n");

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_100kHz;

    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        uartPrint(uart, "I2C open failed\r\n");
        while (1) {}
    }

    uartPrint(uart, "I2C open OK\r\n");
    uartPrint(uart, "AHT2x address: ");
    uartPrintHex(uart, AHT2X_I2C_ADDRESS);
    uartPrint(uart, "ENS160 address: ");
    uartPrintHex(uart, ENS160_I2C_ADDRESS);

    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    AHT2x_init(&aht2x, i2c, AHT2X_I2C_ADDRESS);
    ENS160_init(&ens160, i2c, ENS160_I2C_ADDRESS);

    sensorStatus = AHT2x_ensureCalibrated(&aht2x);
    if (sensorStatus != AHT2X_STATUS_OK) {
        uartPrint(uart, "AHT2x calibration failed: ");
        uartPrint(uart, aht2xStatusToString(sensorStatus));
        uartPrint(uart, "\r\n");
        while (1) {}
    }

    ens160Status = ENS160_setMode(&ens160, ENS160_MODE_STANDARD);
    if (ens160Status != ENS160_STATUS_OK) {
        uartPrint(uart, "ENS160 mode set failed: ");
        uartPrint(uart, ens160StatusToString(ens160Status));
        uartPrint(uart, "\r\n");
        while (1) {}
    }
    {
        uint8_t ensStatusRaw;

        if (ENS160_readStatus(&ens160, &ensStatusRaw) == ENS160_STATUS_OK) {
            uartPrint(uart, "ENS160 status: ");
            uartPrintHex(uart, ensStatusRaw);
        } else {
            uartPrint(uart, "ENS160 status read failed\r\n");
        }
    }
     
    while (1) {
        sensorStatus = AHT2x_readTemperatureHumidity(&aht2x,
                                                     &temperatureC,
                                                     &humidityPercent);

        GPIO_toggle(Board_GPIO_LED0);
        if (sensorStatus == AHT2X_STATUS_OK) {
            uartPrintMeasurement(uart, temperatureC, humidityPercent);
        } else {
            uartPrint(uart, "AHT2x read failed: ");
            uartPrint(uart, aht2xStatusToString(sensorStatus));
            uartPrint(uart, "\r\n");
        }

        ens160Status = ENS160_readAQI(&ens160, &aqi);
        if (ens160Status == ENS160_STATUS_OK) {
            ens160Status = ENS160_readTVOC(&ens160, &tvoc);
        }
        if (ens160Status == ENS160_STATUS_OK) {
            ens160Status = ENS160_readECO2(&ens160, &eco2);
        }

        if (ens160Status == ENS160_STATUS_OK) {
            uartPrintEns160Measurement(uart, aqi, tvoc, eco2);
        } else {
            uartPrint(uart, "ENS160 read failed: ");
            uartPrint(uart, ens160StatusToString(ens160Status));
            uartPrint(uart, "\r\n");
        }

        usleep(1000000);
    }
}
