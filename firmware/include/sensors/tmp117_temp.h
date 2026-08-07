/*
 * Texas Instruments TMP117 High-Precision Clinical Temperature Sensor Driver
 * Part of VITALIS-Edge Biosensing Firmware
 */

#ifndef TMP117_TEMP_H
#define TMP117_TEMP_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TMP117_I2C_ADDR             0x48
#define TMP117_REG_TEMP_RESULT      0x00
#define TMP117_REG_CONFIGURATION    0x01
#define TMP117_REG_TEMP_HIGH_LIMIT  0x02
#define TMP117_REG_TEMP_LOW_LIMIT   0x03
#define TMP117_REG_EEPROM_UNLOCK    0x04
#define TMP117_REG_DEVICE_ID        0x0F

#define TMP117_RESOLUTION_C         0.0078125f /* 7.8125 m°C per LSB */

typedef struct {
    float temperature_celsius;  /* Calibrated skin/body temp */
    bool fever_alert;           /* True if temp > 38.3°C */
    bool hypothermia_alert;     /* True if temp < 35.0°C */
    uint32_t timestamp_ms;
} tmp117_data_t;

typedef struct {
    uint8_t i2c_bus;
    uint8_t i2c_addr;
    float high_limit_c;
    float low_limit_c;
    bool is_initialized;
} tmp117_dev_t;

vitalis_status_t tmp117_init(tmp117_dev_t *dev);
vitalis_status_t tmp117_read_temperature(tmp117_dev_t *dev, tmp117_data_t *out_data);
void tmp117_set_shutdown(tmp117_dev_t *dev, bool shutdown);

#ifdef __cplusplus
}
#endif

#endif /* TMP117_TEMP_H */
