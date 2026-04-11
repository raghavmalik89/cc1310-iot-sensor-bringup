#ifndef ENS160_H_
#define ENS160_H_

#include <stdint.h>

#include <ti/drivers/I2C.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENS160_I2C_ADDRESS      (0x53U)
#define ENS160_MODE_STANDARD    (0x02U)

typedef enum {
    ENS160_STATUS_OK = 0,
    ENS160_STATUS_INVALID_ARG,
    ENS160_STATUS_I2C_ERROR
} ENS160_Status;

typedef struct {
    I2C_Handle i2cHandle;
    uint8_t address;
} ENS160;

void ENS160_init(ENS160 *device, I2C_Handle i2cHandle, uint8_t address);
ENS160_Status ENS160_setMode(ENS160 *device, uint8_t mode);
ENS160_Status ENS160_readStatus(ENS160 *device, uint8_t *status);
ENS160_Status ENS160_readAQI(ENS160 *device, uint8_t *aqi);
ENS160_Status ENS160_readTVOC(ENS160 *device, uint16_t *tvoc);
ENS160_Status ENS160_readECO2(ENS160 *device, uint16_t *eco2);

#ifdef __cplusplus
}
#endif

#endif /* ENS160_H_ */
