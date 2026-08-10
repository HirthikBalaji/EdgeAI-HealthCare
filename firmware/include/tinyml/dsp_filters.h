/*
 * VITALIS-Edge: Digital Signal Processing (DSP) & Feature Extraction Engine
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Implements:
 *   1. Cascaded Biquad IIR Bandpass (0.5Hz - 40Hz) & Notch (50Hz / 60Hz)
 *   2. Pan-Tompkins QRS Detection & Real-time RR-interval tracking
 *   3. Motion Artifact Adaptive Cancellation (LMS Filter with IMU reference)
 *   4. Heart Rate Variability (SDNN, RMSSD, pNN50) extraction
 */

#ifndef DSP_FILTERS_H
#define DSP_FILTERS_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Biquad Direct Form II Transposed State */
typedef struct {
    float b0, b1, b2;  /* Feedforward coefficients */
    float a1, a2;      /* Feedback coefficients */
    float d1, d2;      /* Delay state memory */
} biquad_iir_t;

/* Cascaded ECG Filter Chain (Notch 50Hz + Bandpass 0.5-40Hz) */
typedef struct {
    biquad_iir_t notch_50hz;
    biquad_iir_t bandpass_low;
    biquad_iir_t bandpass_high;
    bool initialized;
} ecg_dsp_pipeline_t;

/* Pan-Tompkins Real-Time QRS Peak Tracker */
typedef struct {
    float prev_samples[5];     /* 5-point derivative buffer */
    float integrate_buffer[32];/* Moving window integrator */
    uint8_t integrate_idx;
    float running_sum;
    float peak_signal;         /* Running estimate of QRS peak amplitude */
    float peak_noise;          /* Running estimate of noise peak amplitude */
    float threshold_i1;        /* Adaptive threshold 1 */
    float threshold_i2;        /* Adaptive threshold 2 */
    uint32_t last_qrs_time_ms; /* Timestamp of last detected R-peak */
    float rr_intervals_ms[16]; /* Circular buffer of last 16 RR intervals */
    uint8_t rr_idx;
    uint8_t rr_count;
    float heart_rate_bpm;
    float sdnn_ms;             /* Standard deviation of NN intervals */
    float rmssd_ms;            /* Root mean square of successive differences */
} pan_tompkins_detector_t;

/* Least Mean Squares (LMS) Adaptive Motion Filter */
typedef struct {
    float weights[8];          /* Adaptive filter taps */
    float delay_line[8];       /* IMU reference history */
    uint8_t idx;
    float mu;                  /* Learning rate step-size */
} lms_motion_canceller_t;

/* DSP Interface Functions */
void dsp_init_ecg_pipeline(ecg_dsp_pipeline_t *pipeline);
float dsp_process_ecg_sample(ecg_dsp_pipeline_t *pipeline, float raw_mv);

void dsp_init_pan_tompkins(pan_tompkins_detector_t *detector);
bool dsp_process_qrs_peak(pan_tompkins_detector_t *detector, float filtered_ecg, uint32_t timestamp_ms);

void dsp_init_lms_canceller(lms_motion_canceller_t *canceller, float mu);
float dsp_cancel_motion_artifact(lms_motion_canceller_t *canceller, float biosignal, float imu_ref);

void dsp_normalize_window(float *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* DSP_FILTERS_H */
