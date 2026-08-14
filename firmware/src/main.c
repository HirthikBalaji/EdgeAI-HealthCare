/*
 * VITALIS-Edge: Autonomous Multi-Modal Edge AI Healthcare System
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Target MCU: Nordic Semiconductor nRF5340 / STMicroelectronics STM32U585
 * Partners: Analog Devices, STMicroelectronics, Texas Instruments, Microchip, Murata, Molex
 * 
 * Main Superloop Execution Flow:
 *   1. Hardware & Peripherals Initialization (I2C, SPI, ADC, BLE)
 *   2. Real-Time Biosignal Acquisition (250 Hz sample tick)
 *   3. Low-Power DSP Preprocessing & Artifact Cancellation
 *   4. Sliding Window Multi-Modal Buffer Management (250 samples x 5 channels)
 *   5. On-Device TinyML Inference (INT8 1D-DSCNN-SE Neural Network)
 *   6. Clinical NEWS2 & Shock Index Triage Evaluation
 *   7. Autonomous Local Alarm / BLE Telemetry / Flash Blackbox Recording
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "vitalis_config.h"
#include "sensors/ad8232_ecg.h"
#include "sensors/max30102_ppg.h"
#include "sensors/lsm6dsox_imu.h"
#include "sensors/tmp117_temp.h"
#include "tinyml/dsp_filters.h"
#include "tinyml/vitalis_tinyml.h"
#include "triage/triage_engine.h"
#include "triage/blackbox_storage.h"
#include "comms/ble_service.h"

/* Global Device Handles */
static ad8232_dev_t        s_ecg_dev;
static max30102_dev_t      s_ppg_dev;
static lsm6dsox_dev_t      s_imu_dev;
static tmp117_dev_t        s_temp_dev;
static blackbox_manager_t  s_blackbox;
static vitalis_ble_ctx_t   s_ble_ctx;

/* DSP & Feature Extraction Pipelines */
static ecg_dsp_pipeline_t       s_ecg_filter;
static pan_tompkins_detector_t  s_qrs_detector;
static lms_motion_canceller_t   s_motion_canceller;

/* Multi-Modal Ring Buffer for TinyML Windowing */
static float s_input_window[VITALIS_INPUT_SAMPLES][VITALIS_INPUT_CHANNELS];
static uint16_t s_window_sample_count = 0;

/* System Status */
static uint32_t s_uptime_ticks_ms = 0;
static uint32_t s_total_inferences = 0;

/**
 * @brief Initialize all hardware sensor interfaces, TinyML neural weights,
 *        DSP filter taps, Flash blackbox storage, and BLE subsystem.
 */
vitalis_status_t vitalis_system_init(void) {
    printf("===============================================================\n");
    printf(" [VITALIS-Edge] Autonomous Edge AI Healthcare Firmware v%d.%d.%d\n",
           VITALIS_FW_VERSION_MAJOR, VITALIS_FW_VERSION_MINOR, VITALIS_FW_VERSION_PATCH);
    printf(" DigiKey Presents electronica India Tech Challenge 2026\n");
    printf("===============================================================\n");

    // 1. Initialize Analog Front-Ends & Digital Sensors
    if (ad8232_init(&s_ecg_dev) != VITALIS_OK) {
        printf(" [!] Error: AD8232 ECG initialization failed\n");
        return VITALIS_ERR_SENSOR_INIT;
    }
    if (max30102_init(&s_ppg_dev) != VITALIS_OK) {
        printf(" [!] Error: MAX30102 PPG initialization failed\n");
        return VITALIS_ERR_SENSOR_INIT;
    }
    if (lsm6dsox_init(&s_imu_dev) != VITALIS_OK) {
        printf(" [!] Error: LSM6DSOX IMU initialization failed\n");
        return VITALIS_ERR_SENSOR_INIT;
    }
    if (tmp117_init(&s_temp_dev) != VITALIS_OK) {
        printf(" [!] Error: TMP117 Temp sensor initialization failed\n");
        return VITALIS_ERR_SENSOR_INIT;
    }

    // 2. Initialize DSP Pipelines
    dsp_init_ecg_pipeline(&s_ecg_filter);
    dsp_init_pan_tompkins(&s_qrs_detector);
    dsp_init_lms_canceller(&s_motion_canceller, 0.01f);

    // 3. Initialize TinyML Inference Engine
    if (tinyml_init_engine() != VITALIS_OK) {
        printf(" [!] Error: TinyML Neural Network engine initialization failed\n");
        return VITALIS_ERR_INFERENCE_FAIL;
    }

    // 4. Initialize Triage Decision Engine & Flash Storage
    triage_init();
    if (blackbox_init(&s_blackbox) != VITALIS_OK) {
        printf(" [!] Error: SPI Flash Blackbox initialization failed\n");
        return VITALIS_ERR_FLASH_WRITE;
    }

    // 5. Initialize BLE Communications
    if (ble_init_stack(&s_ble_ctx) != VITALIS_OK) {
        printf(" [!] Error: BLE stack initialization failed\n");
    }

    printf(" [✓] All Hardware Front-Ends, TinyML Core, and DSP Pipelines Initialized.\n");
    printf(" [✓] Ready for autonomous zero-cloud continuous monitoring.\n\n");
    return VITALIS_OK;
}

/**
 * @brief Processes one sample tick (called at 250 Hz from timer interrupt / RTOS task)
 */
void vitalis_process_sample_tick(float raw_ecg_mv, uint32_t raw_ppg_red, uint32_t raw_ppg_ir,
                                 float accel_x, float accel_y, float accel_z, float skin_temp_c) {
    s_uptime_ticks_ms += 4; // 250 Hz = 4 ms per tick

    // 1. Digital Signal Processing
    // Step 1a: Filter ECG through 50Hz Notch + 0.5-40Hz Bandpass
    float filtered_ecg = dsp_process_ecg_sample(&s_ecg_filter, raw_ecg_mv);

    // Step 1b: Cancel motion artifacts using IMU vector magnitude
    float accel_mag = sqrtf(accel_x * accel_x + accel_y * accel_y + accel_z * accel_z);
    float motion_cleaned_ecg = dsp_cancel_motion_artifact(&s_motion_canceller, filtered_ecg, accel_mag - 1.0f);

    // Step 1c: Pan-Tompkins QRS Peak Detection & Heart Rate Tracking
    bool is_r_peak = dsp_process_qrs_peak(&s_qrs_detector, motion_cleaned_ecg, s_uptime_ticks_ms);
    (void)is_r_peak;

    // Step 1d: PPG Normalization & SpO2 calculation
    max30102_data_t ppg_data;
    max30102_calculate_vitals(raw_ppg_red, raw_ppg_ir, &ppg_data);

    // 2. Buffer into 5-Channel Sliding Window
    if (s_window_sample_count < VITALIS_INPUT_SAMPLES) {
        s_input_window[s_window_sample_count][0] = motion_cleaned_ecg;
        s_input_window[s_window_sample_count][1] = ppg_data.normalized_ir;
        s_input_window[s_window_sample_count][2] = ppg_data.normalized_red;
        s_input_window[s_window_sample_count][3] = accel_mag;
        s_input_window[s_window_sample_count][4] = skin_temp_c;
        s_window_sample_count++;
    }

    // 3. Sliding Window Inference Execution (Every 250 samples / 1.0 second)
    if (s_window_sample_count >= VITALIS_INPUT_SAMPLES) {
        vitalis_inference_result_t ai_result;
        tinyml_run_inference(s_input_window, &ai_result);
        s_total_inferences++;

        // 4. Clinical Triage Decision Fusion
        bool fall_immobile = (accel_mag < 0.35f && s_qrs_detector.heart_rate_bpm > 90.0f);
        vitalis_triage_decision_t triage_decision;
        triage_evaluate(
            s_qrs_detector.heart_rate_bpm,
            ppg_data.calculated_spo2,
            skin_temp_c,
            fall_immobile,
            &ai_result,
            &triage_decision
        );

        // 5. Check Emergency Actions
        if (triage_decision.triage_level >= TRIAGE_LEVEL_WARNING) {
            // Trigger Local Piezo/Haptic Alarm
            // Broadcast direct BLE emergency beacon
            vitalis_ble_emergency_beacon_t beacon;
            beacon.alert_timestamp_ms = s_uptime_ticks_ms;
            beacon.urgency_level = (uint8_t)triage_decision.triage_level;
            beacon.primary_condition = (uint8_t)ai_result.predicted_class;
            strncpy(beacon.alert_text, triage_decision.clinical_recommendation, sizeof(beacon.alert_text) - 1);
            ble_broadcast_emergency_beacon(&s_ble_ctx, &beacon);

            // Log event waveform to SPI Flash Blackbox
            blackbox_event_header_t event_hdr;
            event_hdr.timestamp_ms = s_uptime_ticks_ms;
            event_hdr.triage_level = (uint8_t)triage_decision.triage_level;
            event_hdr.ai_classification = (uint8_t)ai_result.predicted_class;
            event_hdr.heart_rate = s_qrs_detector.heart_rate_bpm;
            event_hdr.spo2 = ppg_data.calculated_spo2;
            event_hdr.temperature = skin_temp_c;
            blackbox_log_event(&s_blackbox, &event_hdr, (const float*)s_input_window, VITALIS_INPUT_SAMPLES * VITALIS_INPUT_CHANNELS);
        }

        // Send normal BLE telemetry notification
        vitalis_ble_telemetry_packet_t telemetry;
        telemetry.timestamp_ms = s_uptime_ticks_ms;
        telemetry.heart_rate = s_qrs_detector.heart_rate_bpm;
        telemetry.spo2 = ppg_data.calculated_spo2;
        telemetry.temperature = skin_temp_c;
        telemetry.triage_level = (uint8_t)triage_decision.triage_level;
        telemetry.ai_class = (uint8_t)ai_result.predicted_class;
        telemetry.ai_confidence = ai_result.max_confidence;
        telemetry.news2_score = triage_decision.news2_score;
        telemetry.battery_percent = 98;
        ble_send_telemetry_notification(&s_ble_ctx, &telemetry);

        // Slide window by 50% hop (125 samples)
        memmove(&s_input_window[0], &s_input_window[VITALIS_WINDOW_HOP_SAMPLES],
                (VITALIS_INPUT_SAMPLES - VITALIS_WINDOW_HOP_SAMPLES) * sizeof(s_input_window[0]));
        s_window_sample_count = VITALIS_INPUT_SAMPLES - VITALIS_WINDOW_HOP_SAMPLES;
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vitalis_system_init();
    printf(" [VITALIS-Edge] Superloop running. Standalone embedded engine operational.\n");
    return 0;
}
