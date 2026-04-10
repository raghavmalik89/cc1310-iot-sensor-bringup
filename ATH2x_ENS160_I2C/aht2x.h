#ifndef AHT2X_H_
#define AHT2X_H_

#include <stdint.h>
#include <stdbool.h>

#include <ti/drivers/I2C.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AHT2X_I2C_ADDRESS             (0x38U)
#define AHT2X_STATUS_BUSY_MASK        (0x80U)
#define AHT2X_POLL_DELAY_US           (5000U)
#define AHT2X_TIMEOUT_US              (200000U)

typedef enum {
    AHT2X_STATUS_OK = 0,
    AHT2X_STATUS_INVALID_ARG,
    AHT2X_STATUS_I2C_ERROR,
    AHT2X_STATUS_TIMEOUT,
    AHT2X_STATUS_BUSY
} AHT2x_Status;

typedef struct {
    I2C_Handle i2cHandle;
    uint8_t address;
} AHT2x;

void AHT2x_init(AHT2x *device, I2C_Handle i2cHandle, uint8_t address);
AHT2x_Status AHT2x_ensureCalibrated(AHT2x *device);
AHT2x_Status AHT2x_readStatus(AHT2x *device, uint8_t *status);
AHT2x_Status AHT2x_triggerMeasurement(AHT2x *device);
AHT2x_Status AHT2x_waitForMeasurementComplete(AHT2x *device,
                                              uint32_t timeoutUs,
                                              uint32_t pollDelayUs);
AHT2x_Status AHT2x_readRawData(AHT2x *device, uint8_t rawData[6]);
float AHT2x_rawToTemperatureC(const uint8_t rawData[6]);
float AHT2x_rawToHumidityPercent(const uint8_t rawData[6]);
AHT2x_Status AHT2x_readTemperatureHumidity(AHT2x *device,
                                           float *temperatureC,
                                           float *humidityPercent);

#ifdef __cplusplus
}
#endif

#endif /* AHT2X_H_ */
