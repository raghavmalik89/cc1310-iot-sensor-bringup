#include "ens160.h"

#include <stddef.h>
#include <string.h>
#include <unistd.h>

/* Device status register reports sensor operational state. */
#define ENS160_REG_STATUS       (0x20U)
/* Air quality index register returns a 1-byte AQI result. */
#define ENS160_REG_DATA_AQI     (0x21U)
/* TVOC result register stores a 16-bit little-endian ppb value. */
#define ENS160_REG_DATA_TVOC    (0x22U)
/* eCO2 result register stores a 16-bit little-endian ppm value. */
#define ENS160_REG_DATA_ECO2    (0x24U)
/* Operating mode register controls standby and standard modes. */
#define ENS160_REG_OPMODE       (0x10U)
/* Allow mode change to settle before first data read. */
#define ENS160_MODE_DELAY_US    (20000U)

static ENS160_Status ENS160_transfer(ENS160 *device,
                                     const void *writeBuf,
                                     size_t writeCount,
                                     void *readBuf,
                                     size_t readCount)
{
    I2C_Transaction transaction;

    if ((device == NULL) || (device->i2cHandle == NULL)) {
        return ENS160_STATUS_INVALID_ARG;
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
        return ENS160_STATUS_I2C_ERROR;
    }

    return ENS160_STATUS_OK;
}

static ENS160_Status ENS160_readRegister8(ENS160 *device,
                                          uint8_t reg,
                                          uint8_t *value)
{
    if (value == NULL) {
        return ENS160_STATUS_INVALID_ARG;
    }

    return ENS160_transfer(device, &reg, 1U, value, 1U);
}

static ENS160_Status ENS160_readRegister16(ENS160 *device,
                                           uint8_t reg,
                                           uint16_t *value)
{
    uint8_t data[2];
    ENS160_Status status;

    if (value == NULL) {
        return ENS160_STATUS_INVALID_ARG;
    }

    status = ENS160_transfer(device, &reg, 1U, data, sizeof(data));
    if (status != ENS160_STATUS_OK) {
        return status;
    }

    *value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);

    return ENS160_STATUS_OK;
}

void ENS160_init(ENS160 *device, I2C_Handle i2cHandle, uint8_t address)
{
    if (device == NULL) {
        return;
    }

    device->i2cHandle = i2cHandle;
    device->address = address;
}

ENS160_Status ENS160_setMode(ENS160 *device, uint8_t mode)
{
    uint8_t command[2];
    ENS160_Status status;

    if (device == NULL) {
        return ENS160_STATUS_INVALID_ARG;
    }

    command[0] = ENS160_REG_OPMODE;
    command[1] = mode;

    status = ENS160_transfer(device, command, sizeof(command), NULL, 0U);
    if (status != ENS160_STATUS_OK) {
        return status;
    }

    usleep(ENS160_MODE_DELAY_US);

    return ENS160_STATUS_OK;
}

ENS160_Status ENS160_readStatus(ENS160 *device, uint8_t *status)
{
    return ENS160_readRegister8(device, ENS160_REG_STATUS, status);
}

ENS160_Status ENS160_readAQI(ENS160 *device, uint8_t *aqi)
{
    return ENS160_readRegister8(device, ENS160_REG_DATA_AQI, aqi);
}

ENS160_Status ENS160_readTVOC(ENS160 *device, uint16_t *tvoc)
{
    return ENS160_readRegister16(device, ENS160_REG_DATA_TVOC, tvoc);
}

ENS160_Status ENS160_readECO2(ENS160 *device, uint16_t *eco2)
{
    return ENS160_readRegister16(device, ENS160_REG_DATA_ECO2, eco2);
}
