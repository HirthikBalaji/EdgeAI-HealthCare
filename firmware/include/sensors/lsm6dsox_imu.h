/*
 * STMicroelectronics LSM6DSOX 6-DoF IMU Driver with Machine Learning Core (MLC)
 * Part of VITALIS-Edge Biosensing Firmware
 */

#ifndef LSM6DSOX_IMU_H
#define LSM6DSOX_IMU_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LSM6DSOX_I2C_ADDR           0x6A
#define LSM6DSOX_WHO_AM_I           0x0F
#define LSM6DSOX_WHO_AM_I_VAL       0x6C

/* Activity / Posture Enums recognized by MLC */
typedef enum {
    IMU_ACT_RESTING_SUPINE = 0,
    IMU_ACT_RESTING_PRONE = 1,
    IMU_ACT_SITTING = 2,
    IMU_ACT_WALKING = 3,
    IMU_ACT_RUNNING = 4,
    IMU_ACT_FALL_IMPACT = 5,
    IMU_ACT_SEIZURE_TREMOR = 6
} lsm6dsox_activity_t;

typedef struct {
    float accel_x_g;            /* Linear acceleration X (g) */
    float accel_y_g;            /* Linear acceleration Y (g) */
    float accel_z_g;            /* Linear acceleration Z (g) */
    float accel_mag_g;          /* Vector Magnitude sqrt(x^2 + y^2 + z^2) */
    float gyro_x_dps;           /* Angular rate X (degrees per second) */
    float gyro_y_dps;           /* Angular rate Y (degrees per second) */
    float gyro_z_dps;           /* Angular rate Z (degrees per second) */
    lsm6dsox_activity_t mlc_activity; /* On-sensor MLC classified state */
    bool free_fall_detected;    /* Hardware interrupt status */
    bool wake_up_event;
} lsm6dsox_data_t;

typedef struct {
    uint8_t i2c_bus;
    uint8_t i2c_addr;
    uint8_t accel_fs_g;         /* Full Scale: 2, 4, 8, 16g */
    uint16_t gyro_fs_dps;       /* Full Scale: 125, 250, 500, 1000, 2000 dps */
    bool is_initialized;
} lsm6dsox_dev_t;

vitalis_status_t lsm6dsox_init(lsm6dsox_dev_t *dev);
vitalis_status_t lsm6dsox_read_data(lsm6dsox_dev_t *dev, lsm6dsox_data_t *out_data);
void lsm6dsox_configure_mlc(lsm6dsox_dev_t *dev);
bool lsm6dsox_check_fall_event(const lsm6dsox_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* LSM6DSOX_IMU_H */
