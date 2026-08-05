/*
 * Maxim / Analog Devices MAX30102 Optical PPG Driver Implementation
 */

#include "sensors/max30102_ppg.h"
#include <math.h>
#include <string.h>

/* Running DC tracker for AC/DC extraction */
static float s_dc_red = 50000.0f;
static float s_dc_ir = 50000.0f;
static const float s_alpha = 0.95f; // Exponential moving average coefficient

vitalis_status_t max30102_init(max30102_dev_t *dev) {
    if (!dev) return VITALIS_ERR_INVALID_PARAM;
    dev->i2c_addr = MAX30102_I2C_ADDR;
    dev->is_initialized = true;
    s_dc_red = 50000.0f;
    s_dc_ir = 50000.0f;
    return VITALIS_OK;
}

vitalis_status_t max30102_read_fifo(max30102_dev_t *dev, uint32_t *raw_red, uint32_t *raw_ir) {
    if (!dev || !dev->is_initialized || !raw_red || !raw_ir) {
        return VITALIS_ERR_INVALID_PARAM;
    }
    // Hardware I2C burst read from REG_FIFO_DATA
    return VITALIS_OK;
}

vitalis_status_t max30102_calculate_vitals(uint32_t red_val, uint32_t ir_val, max30102_data_t *out_data) {
    if (!out_data) return VITALIS_ERR_INVALID_PARAM;

    out_data->raw_red = red_val;
    out_data->raw_ir = ir_val;

    // Skin contact check
    if (ir_val < 5000) {
        out_data->finger_detected = false;
        out_data->calculated_spo2 = 0.0f;
        out_data->normalized_red = 0.0f;
        out_data->normalized_ir = 0.0f;
        return VITALIS_OK;
    }

    out_data->finger_detected = true;

    // Track DC levels
    s_dc_red = s_alpha * s_dc_red + (1.0f - s_alpha) * (float)red_val;
    s_dc_ir = s_alpha * s_dc_ir + (1.0f - s_alpha) * (float)ir_val;

    float ac_red = (float)red_val - s_dc_red;
    float ac_ir = (float)ir_val - s_dc_ir;

    out_data->normalized_red = ac_red / (s_dc_red + 1e-5f);
    out_data->normalized_ir = ac_ir / (s_dc_ir + 1e-5f);

    // Compute Ratio-of-Ratios R = (AC_red / DC_red) / (AC_ir / DC_ir)
    float r_ratio = 1.0f;
    if (fabsf(out_data->normalized_ir) > 1e-5f) {
        r_ratio = fabsf(out_data->normalized_red / out_data->normalized_ir);
    }

    // Empirical calibration curve: SpO2 = 110.0 - 25.0 * R
    float spo2 = 110.0f - 25.0f * r_ratio;
    if (spo2 > 100.0f) spo2 = 100.0f;
    if (spo2 < 70.0f) spo2 = 70.0f;

    out_data->calculated_spo2 = spo2;
    return VITALIS_OK;
}

void max30102_set_shutdown(max30102_dev_t *dev, bool shutdown) {
    if (!dev) return;
    (void)shutdown;
    // Set SHDN bit in REG_MODE_CONFIG
}
