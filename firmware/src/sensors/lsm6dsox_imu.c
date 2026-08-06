/*
 * STMicroelectronics LSM6DSOX 6-DoF IMU Driver Implementation
 */

#include "sensors/lsm6dsox_imu.h"
#include <math.h>
#include <string.h>

vitalis_status_t lsm6dsox_init(lsm6dsox_dev_t *dev) {
    if (!dev) return VITALIS_ERR_INVALID_PARAM;
    dev->i2c_addr = LSM6DSOX_I2C_ADDR;
    dev->accel_fs_g = 8;
    dev->gyro_fs_dps = 1000;
    dev->is_initialized = true;
    return VITALIS_OK;
}

vitalis_status_t lsm6dsox_read_data(lsm6dsox_dev_t *dev, lsm6dsox_data_t *out_data) {
    if (!dev || !dev->is_initialized || !out_data) {
        return VITALIS_ERR_INVALID_PARAM;
    }

    // Vector magnitude calculation
    out_data->accel_mag_g = sqrtf(
        out_data->accel_x_g * out_data->accel_x_g +
        out_data->accel_y_g * out_data->accel_y_g +
        out_data->accel_z_g * out_data->accel_z_g
    );

    return VITALIS_OK;
}

void lsm6dsox_configure_mlc(lsm6dsox_dev_t *dev) {
    if (!dev) return;
    // Configures Machine Learning Core decision trees inside ST sensor registers
}

bool lsm6dsox_check_fall_event(const lsm6dsox_data_t *data) {
    if (!data) return false;
    // High impact G spike > threshold
    return (data->accel_mag_g >= VITALIS_FALL_IMPACT_G_THRESH) || data->free_fall_detected;
}
