/*
 * Maxim / Analog Devices MAX30102 Optical Pulse Oximetry & PPG Driver
 * Part of VITALIS-Edge Biosensing Firmware
 */

#ifndef MAX30102_PPG_H
#define MAX30102_PPG_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX30102_I2C_ADDR           0x57

/* Register Definitions */
#define REG_INTR_STATUS_1           0x00
#define REG_INTR_STATUS_2           0x01
#define REG_INTR_ENABLE_1           0x02
#define REG_INTR_ENABLE_2           0x03
#define REG_FIFO_WR_PTR             0x04
#define REG_OVF_COUNTER             0x05
#define REG_FIFO_RD_PTR             0x06
#define REG_FIFO_DATA               0x07
#define REG_FIFO_CONFIG             0x08
#define REG_MODE_CONFIG             0x09
#define REG_SPO2_CONFIG             0x0A
#define REG_LED1_PA                 0x0C /* Red LED pulse amplitude */
#define REG_LED2_PA                 0x0D /* IR LED pulse amplitude */
#define REG_MULTI_LED_CTRL1         0x11
#define REG_MULTI_LED_CTRL2         0x12
#define REG_TEMP_INTR               0x1F
#define REG_TEMP_FRAC               0x20
#define REG_PART_ID                 0xFF

typedef struct {
    uint32_t raw_red;           /* Raw 18-bit optical count Red channel */
    uint32_t raw_ir;            /* Raw 18-bit optical count IR channel */
    float normalized_red;       /* AC/DC normalized Red */
    float normalized_ir;        /* AC/DC normalized IR */
    float calculated_spo2;      /* Estimated SpO2 percentage */
    float estimated_hr_bpm;     /* Estimated PPG pulse rate */
    bool finger_detected;       /* True if skin contact detected */
} max30102_data_t;

typedef struct {
    uint8_t i2c_bus;
    uint8_t i2c_addr;
    uint8_t red_led_current_ma; /* 0-50 mA */
    uint8_t ir_led_current_ma;  /* 0-50 mA */
    bool is_initialized;
} max30102_dev_t;

vitalis_status_t max30102_init(max30102_dev_t *dev);
vitalis_status_t max30102_read_fifo(max30102_dev_t *dev, uint32_t *raw_red, uint32_t *raw_ir);
vitalis_status_t max30102_calculate_vitals(uint32_t red_val, uint32_t ir_val, max30102_data_t *out_data);
void max30102_set_shutdown(max30102_dev_t *dev, bool shutdown);

#ifdef __cplusplus
}
#endif

#endif /* MAX30102_PPG_H */
