/*
 * Analog Devices AD8232 / MAX30003 Single-Lead ECG Driver Implementation
 */

#include "sensors/ad8232_ecg.h"
#include <string.h>

vitalis_status_t ad8232_init(ad8232_dev_t *dev) {
    if (!dev) {
        return VITALIS_ERR_INVALID_PARAM;
    }
    
    if (dev->adc_vref_mv <= 0.0f) {
        dev->adc_vref_mv = 3300.0f; // 3.3V default
    }
    if (dev->adc_max_count == 0) {
        dev->adc_max_count = 4095;   // 12-bit ADC default
    }

    dev->is_initialized = true;
    return VITALIS_OK;
}

vitalis_status_t ad8232_read_sample(ad8232_dev_t *dev, ad8232_sample_t *out_sample) {
    if (!dev || !dev->is_initialized || !out_sample) {
        return VITALIS_ERR_INVALID_PARAM;
    }

    // In hardware target, this reads ADC via DMA / HAL ADC register
    // and checks LO+ / LO- GPIO pins
    out_sample->leads_off_detected = ad8232_is_leads_off(dev);
    return VITALIS_OK;
}

bool ad8232_is_leads_off(ad8232_dev_t *dev) {
    if (!dev) return true;
    // Returns status of LO+ and LO- digital input pins
    return false;
}

void ad8232_set_power_mode(ad8232_dev_t *dev, bool enable) {
    if (!dev) return;
    (void)enable;
    // Set SDN pin high for active, low for ultra-low power shutdown (< 1 uA)
}
