#include "aht2x.h"

#include <stddef.h>
#include <string.h>
#include <unistd.h>

static AHT2x_Status AHT2x_transfer(AHT2x *device,
                                   const void *writeBuf,
                                   size_t writeCount,
                                   void *readBuf,
                                   size_t readCount)
{
    I2C_Transaction transaction;

    if ((device == NULL) || (device->i2cHandle == NULL)) {
        return AHT2X_STATUS_INVALID_ARG;
    }

    memset(&transaction, 0, sizeof(transaction));
    transaction.slaveAddress = device->address;
    transaction.writeCount = writeCount;
    transaction.readCount = readCount;
    if (writeCount > 0U) {
        transaction.writeBuf = (void *)writeBuf;
    }
    if (readCount > 0U) {
        transaction.readBuf = readBuf;
    }

    if (!I2C_transfer(device->i2cHandle, &transaction)) {
        return AHT2X_STATUS_I2C_ERROR;
    }

    return AHT2X_STATUS_OK;
}

void AHT2x_init(AHT2x *device, I2C_Handle i2cHandle, uint8_t address)
{
    if (device == NULL) {
        return;
    }

    device->i2cHandle = i2cHandle;
    device->address = address;
}

AHT2x_Status AHT2x_ensureCalibrated(AHT2x *device)
{
    static const uint8_t command[3] = { 0xBEU, 0x08U, 0x00U };
    uint8_t status;
    AHT2x_Status result;

    result = AHT2x_readStatus(device, &status);
    if (result != AHT2X_STATUS_OK) {
        return result;
    }

    if ((status & 0x08U) != 0U) {
        return AHT2X_STATUS_OK;
    }

    result = AHT2x_transfer(device, command, sizeof(command), NULL, 0U);
    if (result != AHT2X_STATUS_OK) {
        return result;
    }

    usleep(20000U);

    return AHT2X_STATUS_OK;
}

AHT2x_Status AHT2x_readStatus(AHT2x *device, uint8_t *status)
{
    if ((device == NULL) || (status == NULL)) {
    return AHT2X_STATUS_INVALID_ARG;
    }

    return AHT2x_transfer(device, NULL, 0U, status, 1U);
}

AHT2x_Status AHT2x_triggerMeasurement(AHT2x *device)
{
    static const uint8_t command[3] = { 0xACU, 0x33U, 0x00U };
    uint8_t status;
    AHT2x_Status result;

    result = AHT2x_readStatus(device, &status);
    if (result != AHT2X_STATUS_OK) {
        return result;
    }

    if ((status & AHT2X_STATUS_BUSY_MASK) != 0U) {
        return AHT2X_STATUS_BUSY;
    }

    return AHT2x_transfer(device, command, sizeof(command), NULL, 0U);
}

AHT2x_Status AHT2x_waitForMeasurementComplete(AHT2x *device,
                                              uint32_t timeoutUs,
                                              uint32_t pollDelayUs)
{
    uint32_t elapsedUs = 0U;
    uint8_t status = 0U;
    AHT2x_Status result;

    if ((timeoutUs == 0U) || (pollDelayUs == 0U)) {
        return AHT2X_STATUS_INVALID_ARG;
    }

    while (elapsedUs < timeoutUs) {
        result = AHT2x_readStatus(device, &status);
        if (result != AHT2X_STATUS_OK) {
            return result;
        }

        if ((status & AHT2X_STATUS_BUSY_MASK) == 0U) {
            return AHT2X_STATUS_OK;
        }

        usleep(pollDelayUs);
        elapsedUs += pollDelayUs;
    }

    return AHT2X_STATUS_TIMEOUT;
}

AHT2x_Status AHT2x_readRawData(AHT2x *device, uint8_t rawData[6])
{
    if (rawData == NULL) {
        return AHT2X_STATUS_INVALID_ARG;
    }

    return AHT2x_transfer(device, NULL, 0U, rawData, 6U);
}

float AHT2x_rawToTemperatureC(const uint8_t rawData[6])
{
    uint32_t rawTemperature;

    rawTemperature = ((uint32_t)(rawData[3] & 0x0FU) << 16)
                   | ((uint32_t)rawData[4] << 8)
                   | (uint32_t)rawData[5];

    return ((float)rawTemperature * 200.0f / 1048576.0f) - 50.0f;
}

float AHT2x_rawToHumidityPercent(const uint8_t rawData[6])
{
    uint32_t rawHumidity;

    rawHumidity = ((uint32_t)rawData[1] << 12)
                | ((uint32_t)rawData[2] << 4)
                | ((uint32_t)(rawData[3] & 0xF0U) >> 4);

    return ((float)rawHumidity * 100.0f / 1048576.0f);
}

AHT2x_Status AHT2x_readTemperatureHumidity(AHT2x *device,
                                           float *temperatureC,
                                           float *humidityPercent)
{
    uint8_t rawData[6];
    AHT2x_Status result;

    if ((temperatureC == NULL) || (humidityPercent == NULL)) {
        return AHT2X_STATUS_INVALID_ARG;
    }

    result = AHT2x_triggerMeasurement(device);
    if (result != AHT2X_STATUS_OK) {
        return result;
    }

    result = AHT2x_waitForMeasurementComplete(device,
                                              AHT2X_TIMEOUT_US,
                                              AHT2X_POLL_DELAY_US);
    if (result != AHT2X_STATUS_OK) {
        return result;
    }

    result = AHT2x_readRawData(device, rawData);
    if (result != AHT2X_STATUS_OK) {
        return result;
    }

    if ((rawData[0] & AHT2X_STATUS_BUSY_MASK) != 0U) {
        return AHT2X_STATUS_BUSY;
    }

    *humidityPercent = AHT2x_rawToHumidityPercent(rawData);
    *temperatureC = AHT2x_rawToTemperatureC(rawData);

    return AHT2X_STATUS_OK;
}
