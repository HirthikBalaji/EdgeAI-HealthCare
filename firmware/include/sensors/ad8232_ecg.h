/*
 * Analog Devices AD8232 / MAX30003 Single-Lead ECG Front-End Driver
 * Part of VITALIS-Edge Biosensing Firmware
 */

#ifndef AD8232_ECG_H
#define AD8232_ECG_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float raw_voltage_mv;      /* Instantaneous ADC biopotential in mV */
    float filtered_ecg_mv;     /* Bandpass & notch filtered ECG in mV */
    bool leads_off_detected;   /* True if LO+ or LO- lead disconnect active */
    uint32_t timestamp_ms;     /* Millisecond timestamp */
} ad8232_sample_t;

typedef struct {
    uint8_t pin_sdn;           /* Shutdown enable pin */
    uint8_t pin_lop;           /* Leads-off Positive (LO+) pin */
    uint8_t pin_lon;           /* Leads-off Negative (LO-) pin */
    uint8_t adc_channel;       /* ADC channel assigned for ECG */
    float adc_vref_mv;         /* ADC reference voltage in mV (e.g. 3000.0f) */
    uint16_t adc_max_count;    /* ADC resolution count (e.g. 4095 for 12-bit) */
    bool is_initialized;
} ad8232_dev_t;

vitalis_status_t ad8232_init(ad8232_dev_t *dev);
vitalis_status_t ad8232_read_sample(ad8232_dev_t *dev, ad8232_sample_t *out_sample);
bool ad8232_is_leads_off(ad8232_dev_t *dev);
void ad8232_set_power_mode(ad8232_dev_t *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* AD8232_ECG_H */
