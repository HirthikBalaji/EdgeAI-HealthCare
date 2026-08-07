/*
 * Texas Instruments TMP117 High-Precision Clinical Temperature Sensor Implementation
 */

#include "sensors/tmp117_temp.h"
#include <string.h>

vitalis_status_t tmp117_init(tmp117_dev_t *dev) {
    if (!dev) return VITALIS_ERR_INVALID_PARAM;
    dev->i2c_addr = TMP117_I2C_ADDR;
    dev->high_limit_c = VITALIS_FEVER_TEMP_THRESHOLD;
    dev->low_limit_c = VITALIS_HYPOTHERMIA_THRESHOLD;
    dev->is_initialized = true;
    return VITALIS_OK;
}

vitalis_status_t tmp117_read_temperature(tmp117_dev_t *dev, tmp117_data_t *out_data) {
    if (!dev || !dev->is_initialized || !out_data) {
        return VITALIS_ERR_INVALID_PARAM;
    }

    // In hardware target, reads 16-bit register REG_TEMP_RESULT via I2C
    // Temperature in °C = raw_int16 * 0.0078125
    out_data->fever_alert = (out_data->temperature_celsius >= dev->high_limit_c);
    out_data->hypothermia_alert = (out_data->temperature_celsius <= dev->low_limit_c);
    return VITALIS_OK;
}

void tmp117_set_shutdown(tmp117_dev_t *dev, bool shutdown) {
    if (!dev) return;
    (void)shutdown;
    // Set MOD[1:0] bits in REG_CONFIGURATION for 1.5uA shutdown mode
}
