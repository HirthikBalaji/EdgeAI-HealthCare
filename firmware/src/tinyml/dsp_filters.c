/*
 * VITALIS-Edge: DSP Filters & Feature Extraction Engine Implementation
 */

#include "tinyml/dsp_filters.h"
#include <math.h>
#include <string.h>

static inline float biquad_filter_process(biquad_iir_t *b, float in) {
    // Direct Form II Transposed implementation
    float out = b->b0 * in + b->d1;
    b->d1 = b->b1 * in - b->a1 * out + b->d2;
    b->d2 = b->b2 * in - b->a2 * out;
    return out;
}

void dsp_init_ecg_pipeline(ecg_dsp_pipeline_t *pipeline) {
    if (!pipeline) return;
    memset(pipeline, 0, sizeof(ecg_dsp_pipeline_t));

    // 50 Hz Notch filter (fs=250 Hz, Q=10)
    // Coeffs computed via bilinear transform
    pipeline->notch_50hz.b0 = 0.9512f;
    pipeline->notch_50hz.b1 = -0.5878f;
    pipeline->notch_50hz.b2 = 0.9512f;
    pipeline->notch_50hz.a1 = -0.5878f;
    pipeline->notch_50hz.a2 = 0.9024f;

    // Highpass 0.5 Hz (fs=250 Hz) - Baseline wander removal
    pipeline->bandpass_high.b0 = 0.9937f;
    pipeline->bandpass_high.b1 = -1.9874f;
    pipeline->bandpass_high.b2 = 0.9937f;
    pipeline->bandpass_high.a1 = -1.9874f;
    pipeline->bandpass_high.a2 = 0.9875f;

    // Lowpass 40 Hz (fs=250 Hz) - Muscle noise / EMG suppression
    pipeline->bandpass_low.b0 = 0.1583f;
    pipeline->bandpass_low.b1 = 0.3167f;
    pipeline->bandpass_low.b2 = 0.1583f;
    pipeline->bandpass_low.a1 = -0.6120f;
    pipeline->bandpass_low.a2 = 0.2454f;

    pipeline->initialized = true;
}

float dsp_process_ecg_sample(ecg_dsp_pipeline_t *pipeline, float raw_mv) {
    if (!pipeline || !pipeline->initialized) return raw_mv;

    float s1 = biquad_filter_process(&pipeline->notch_50hz, raw_mv);
    float s2 = biquad_filter_process(&pipeline->bandpass_high, s1);
    float s3 = biquad_filter_process(&pipeline->bandpass_low, s2);
    return s3;
}

void dsp_init_pan_tompkins(pan_tompkins_detector_t *detector) {
    if (!detector) return;
    memset(detector, 0, sizeof(pan_tompkins_detector_t));
    detector->peak_signal = 0.05f;
    detector->peak_noise = 0.005f;
    detector->threshold_i1 = 0.015f;
    detector->threshold_i2 = 0.008f;
    detector->heart_rate_bpm = 75.0f;
}

bool dsp_process_qrs_peak(pan_tompkins_detector_t *detector, float filtered_ecg, uint32_t timestamp_ms) {
    if (!detector) return false;

    // 1. 5-point Derivative: y[n] = (2*x[n] + x[n-1] - x[n-3] - 2*x[n-4]) / 8
    detector->prev_samples[4] = detector->prev_samples[3];
    detector->prev_samples[3] = detector->prev_samples[2];
    detector->prev_samples[2] = detector->prev_samples[1];
    detector->prev_samples[1] = detector->prev_samples[0];
    detector->prev_samples[0] = filtered_ecg;

    float deriv = (2.0f * detector->prev_samples[0] + detector->prev_samples[1] -
                   detector->prev_samples[3] - 2.0f * detector->prev_samples[4]) / 8.0f;

    // 2. Squaring function (non-linear energy boost)
    float squared = deriv * deriv;

    // 3. Moving Window Integrator (32 samples = 128ms @ 250Hz)
    detector->running_sum -= detector->integrate_buffer[detector->integrate_idx];
    detector->integrate_buffer[detector->integrate_idx] = squared;
    detector->running_sum += squared;
    detector->integrate_idx = (detector->integrate_idx + 1) % 32;

    float integrated = detector->running_sum / 32.0f;

    // 4. Adaptive Thresholding & Refractory Period (200ms)
    bool is_qrs = false;
    uint32_t elapsed_ms = timestamp_ms - detector->last_qrs_time_ms;

    if (elapsed_ms > 200) { // Physiological refractory period
        if (integrated > detector->threshold_i1) {
            is_qrs = true;
            detector->peak_signal = 0.125f * integrated + 0.875f * detector->peak_signal;

            if (detector->last_qrs_time_ms > 0 && elapsed_ms < 3000) {
                // Record RR interval
                detector->rr_intervals_ms[detector->rr_idx] = (float)elapsed_ms;
                detector->rr_idx = (detector->rr_idx + 1) % 16;
                if (detector->rr_count < 16) detector->rr_count++;

                // Compute instantaneous HR
                float sum_rr = 0.0f;
                for (int i = 0; i < detector->rr_count; i++) {
                    sum_rr += detector->rr_intervals_ms[i];
                }
                float avg_rr = sum_rr / detector->rr_count;
                if (avg_rr > 10.0f) {
                    detector->heart_rate_bpm = 60000.0f / avg_rr;
                }

                // Compute SDNN (Standard deviation of RR)
                float var_sum = 0.0f;
                for (int i = 0; i < detector->rr_count; i++) {
                    float diff = detector->rr_intervals_ms[i] - avg_rr;
                    var_sum += diff * diff;
                }
                detector->sdnn_ms = sqrtf(var_sum / detector->rr_count);
            }
            detector->last_qrs_time_ms = timestamp_ms;
        } else {
            detector->peak_noise = 0.125f * integrated + 0.875f * detector->peak_noise;
        }

        // Update thresholds
        detector->threshold_i1 = detector->peak_noise + 0.25f * (detector->peak_signal - detector->peak_noise);
        detector->threshold_i2 = 0.5f * detector->threshold_i1;
    }

    return is_qrs;
}

void dsp_init_lms_canceller(lms_motion_canceller_t *canceller, float mu) {
    if (!canceller) return;
    memset(canceller, 0, sizeof(lms_motion_canceller_t));
    canceller->mu = (mu > 0.0f) ? mu : 0.01f;
}

float dsp_cancel_motion_artifact(lms_motion_canceller_t *canceller, float biosignal, float imu_ref) {
    if (!canceller) return biosignal;

    // Shift IMU reference delay line
    canceller->delay_line[canceller->idx] = imu_ref;
    canceller->idx = (canceller->idx + 1) % 8;

    // Compute estimated artifact
    float estimated_artifact = 0.0f;
    for (int i = 0; i < 8; i++) {
        uint8_t d_idx = (canceller->idx + i) % 8;
        estimated_artifact += canceller->weights[i] * canceller->delay_line[d_idx];
    }

    // Error = Cleaned Signal
    float error = biosignal - estimated_artifact;

    // Update weights: w[i] = w[i] + 2 * mu * error * x[n-i]
    for (int i = 0; i < 8; i++) {
        uint8_t d_idx = (canceller->idx + i) % 8;
        canceller->weights[i] += 2.0f * canceller->mu * error * canceller->delay_line[d_idx];
    }

    return error;
}

void dsp_normalize_window(float *buffer, uint16_t length) {
    if (!buffer || length == 0) return;

    float sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        sum += buffer[i];
    }
    float mean = sum / length;

    float sq_diff_sum = 0.0f;
    for (uint16_t i = 0; i < length; i++) {
        float diff = buffer[i] - mean;
        sq_diff_sum += diff * diff;
    }
    float std_dev = sqrtf(sq_diff_sum / length);
    if (std_dev < 1e-6f) std_dev = 1.0f;

    for (uint16_t i = 0; i < length; i++) {
        buffer[i] = (buffer[i] - mean) / std_dev;
    }
}
