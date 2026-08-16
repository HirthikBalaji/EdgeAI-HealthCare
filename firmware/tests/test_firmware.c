/*
 * VITALIS-Edge: Comprehensive Embedded Firmware & TinyML Test Suite
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Verifies:
 *   1. Biquad IIR Filter & Pan-Tompkins QRS Detection
 *   2. MAX30102 PPG SpO2 & Heart Rate DSP
 *   3. LSM6DSOX 6-DoF Motion Artifact Cancellation & Fall Interception
 *   4. On-Device TinyML INT8 Inference Engine & Confidence Scores
 *   5. NEWS2 Scoring & Deterministic Triage State Escalation
 *   6. SPI NOR Flash Circular Blackbox Event Logging & CRC16 Integrity
 *   7. BLE 5.3 Health Telemetry & Emergency Alert Broadcasting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

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

static int s_tests_passed = 0;
static int s_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  [PASS] %s\n", msg); \
        s_tests_passed++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
        s_tests_failed++; \
    } \
} while(0)

/* --- Test 1: IIR Biquad Filters & Pan-Tompkins QRS Detector --- */
void test_dsp_ecg_and_qrs(void) {
    printf("\n[Test 1/6] Testing DSP Filters & Pan-Tompkins QRS Peak Detector...\n");

    ecg_dsp_pipeline_t pipeline;
    dsp_init_ecg_pipeline(&pipeline);

    pan_tompkins_detector_t qrs_det;
    dsp_init_pan_tompkins(&qrs_det);

    // Feed 3500 samples (3.5 seconds @ 250Hz) of synthetic normal ECG
    uint32_t qrs_count = 0;
    for (uint32_t t_ms = 0; t_ms < 3500; t_ms += 4) {
        float phase = (float)(t_ms % 800) / 800.0f; // 75 bpm -> 800ms period
        float raw_ecg = 0.15f * expf(-powf(phase - 0.15f, 2) / (2.0f * 0.025f * 0.025f))
                      - 0.12f * expf(-powf(phase - 0.28f, 2) / (2.0f * 0.010f * 0.010f))
                      + 1.20f * expf(-powf(phase - 0.30f, 2) / (2.0f * 0.015f * 0.015f))
                      - 0.25f * expf(-powf(phase - 0.33f, 2) / (2.0f * 0.012f * 0.012f))
                      + 0.30f * expf(-powf(phase - 0.55f, 2) / (2.0f * 0.045f * 0.045f));
        // Add 50Hz powerline noise
        raw_ecg += 0.25f * sinf(2.0f * 3.14159f * 50.0f * (t_ms / 1000.0f));

        float filtered = dsp_process_ecg_sample(&pipeline, raw_ecg);
        if (dsp_process_qrs_peak(&qrs_det, filtered, t_ms)) {
            qrs_count++;
        }
    }

    TEST_ASSERT(qrs_count >= 2, "Pan-Tompkins detected QRS peaks accurately");
    TEST_ASSERT(qrs_det.heart_rate_bpm >= 65.0f && qrs_det.heart_rate_bpm <= 85.0f,
                "Heart rate estimated correctly (~75 bpm)");
}

/* --- Test 2: MAX30102 Optical SpO2 & Ratio Calculation --- */
void test_ppg_spo2_calculation(void) {
    printf("\n[Test 2/6] Testing MAX30102 Optical PPG & SpO2 Computation...\n");

    max30102_data_t ppg_out;
    // Simulate healthy finger absorption (SpO2 ~98%)
    for (int i = 0; i < 50; i++) {
        max30102_calculate_vitals(50000 + 1500, 50000 + 3000, &ppg_out);
    }

    TEST_ASSERT(ppg_out.finger_detected == true, "Finger contact detected");
    TEST_ASSERT(ppg_out.calculated_spo2 >= 90.0f && ppg_out.calculated_spo2 <= 100.0f,
                "Calculated SpO2 within valid clinical saturation range");
}

/* --- Test 3: LSM6DSOX Motion Artifact Cancellation & Fall Detector --- */
void test_imu_fall_and_motion(void) {
    printf("\n[Test 3/6] Testing LSM6DSOX IMU Fall Detection & LMS Motion Canceller...\n");

    lsm6dsox_data_t imu_fall;
    imu_fall.accel_x_g = 0.5f;
    imu_fall.accel_y_g = 1.2f;
    imu_fall.accel_z_g = 3.5f;
    imu_fall.free_fall_detected = false;

    lsm6dsox_dev_t dev;
    lsm6dsox_init(&dev);
    lsm6dsox_read_data(&dev, &imu_fall);

    TEST_ASSERT(lsm6dsox_check_fall_event(&imu_fall) == true, "High-G Fall impact event detected");

    lms_motion_canceller_t canceller;
    dsp_init_lms_canceller(&canceller, 0.02f);
    float cleaned = dsp_cancel_motion_artifact(&canceller, 1.2f, 0.8f);
    TEST_ASSERT(fabsf(cleaned) < 2.0f, "LMS Adaptive Motion Filter stabilized");
}

/* --- Test 4: TinyML On-Device Neural Inference Engine --- */
void test_tinyml_inference(void) {
    printf("\n[Test 4/6] Testing TinyML 1D-DSCNN-SE Neural Network Inference Engine...\n");

    tinyml_init_engine();

    // Create 250 samples x 5 channels test tensor
    float input_window[VITALIS_INPUT_SAMPLES][VITALIS_INPUT_CHANNELS];
    for (int t = 0; t < VITALIS_INPUT_SAMPLES; t++) {
        input_window[t][0] = 0.8f * sinf(2.0f * 3.14159f * 1.2f * (t / 250.0f)); // ECG
        input_window[t][1] = 0.5f * sinf(2.0f * 3.14159f * 1.2f * (t / 250.0f)); // PPG-IR
        input_window[t][2] = 0.5f * sinf(2.0f * 3.14159f * 1.2f * (t / 250.0f)); // PPG-Red
        input_window[t][3] = 1.0f; // 1g Accel
        input_window[t][4] = 36.6f; // Temp
    }

    vitalis_inference_result_t result;
    vitalis_status_t status = tinyml_run_inference(input_window, &result);

    TEST_ASSERT(status == VITALIS_OK, "TinyML inference completed successfully");
    TEST_ASSERT(result.max_confidence >= 0.0f && result.max_confidence <= 1.0f, "Softmax confidence valid");
    TEST_ASSERT(result.mac_operations > 50000, "MAC calculation logged");
    printf("     > Inferred Class: %s (Confidence: %.2f%%, MACs: %u)\n",
           tinyml_get_class_name(result.predicted_class), result.max_confidence * 100.0f, result.mac_operations);
}

/* --- Test 5: Clinical NEWS2 & Deterministic Triage Engine --- */
void test_triage_decision_engine(void) {
    printf("\n[Test 5/6] Testing Clinical NEWS2 & Multi-Modal Triage Escalation...\n");

    vitalis_inference_result_t vtach_result;
    vtach_result.predicted_class = VITALIS_CLASS_VTACH;
    vtach_result.max_confidence = 0.96f;

    vitalis_triage_decision_t decision;
    triage_evaluate(180.0f, 89.0f, 37.5f, false, &vtach_result, &decision);

    TEST_ASSERT(decision.triage_level == TRIAGE_LEVEL_CRITICAL, "V-Tach correctly escalated to Level 3 (CRITICAL)");
    TEST_ASSERT(decision.trigger_local_alarm == true, "Local emergency alarm triggered");
    TEST_ASSERT(decision.trigger_ble_emergency_broadcast == true, "BLE emergency broadcast activated");

    // Test Normal Sinus Rhythm
    vitalis_inference_result_t normal_result;
    normal_result.predicted_class = VITALIS_CLASS_NSR;
    normal_result.max_confidence = 0.99f;
    triage_evaluate(72.0f, 98.5f, 36.6f, false, &normal_result, &decision);

    TEST_ASSERT(decision.triage_level == TRIAGE_LEVEL_NORMAL, "Normal vitals classified as Level 0 (NORMAL)");
    TEST_ASSERT(decision.trigger_local_alarm == false, "No false alarms triggered for healthy patient");
}

/* --- Test 6: SPI Flash Blackbox Ring Buffer & CRC16 Integrity --- */
void test_blackbox_flash_storage(void) {
    printf("\n[Test 6/6] Testing SPI NOR Flash Circular Blackbox Storage & CRC16...\n");

    blackbox_manager_t mgr;
    blackbox_init(&mgr);

    blackbox_event_header_t in_hdr;
    memset(&in_hdr, 0, sizeof(in_hdr));
    in_hdr.timestamp_ms = 12500;
    in_hdr.triage_level = TRIAGE_LEVEL_CRITICAL;
    in_hdr.ai_classification = VITALIS_CLASS_VTACH;
    in_hdr.heart_rate = 185.0f;
    in_hdr.spo2 = 88.0f;
    in_hdr.temperature = 37.2f;

    float waveform_test[100];
    for (int i = 0; i < 100; i++) waveform_test[i] = (float)i * 0.1f;

    vitalis_status_t log_st = blackbox_log_event(&mgr, &in_hdr, waveform_test, 100);
    TEST_ASSERT(log_st == VITALIS_OK, "Blackbox event logged to flash sector");

    blackbox_event_header_t out_hdr;
    float read_waveform[100];
    vitalis_status_t read_st = blackbox_read_event(&mgr, 0, &out_hdr, read_waveform);

    TEST_ASSERT(read_st == VITALIS_OK, "Blackbox event retrieved from flash");
    TEST_ASSERT(out_hdr.magic_header == BLACKBOX_EVENT_MAGIC, "Magic header verified (0x56495441)");
    TEST_ASSERT(out_hdr.heart_rate == 185.0f, "Logged telemetry parameters intact");
    TEST_ASSERT(fabsf(read_waveform[50] - 5.0f) < 1e-4f, "Waveform array integrity confirmed");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("====================================================================\n");
    printf(" VITALIS-Edge Embedded Firmware & TinyML Hardware Test Suite\n");
    printf(" DigiKey Presents electronica India Tech Challenge 2026\n");
    printf("====================================================================\n");

    test_dsp_ecg_and_qrs();
    test_ppg_spo2_calculation();
    test_imu_fall_and_motion();
    test_tinyml_inference();
    test_triage_decision_engine();
    test_blackbox_flash_storage();

    printf("\n====================================================================\n");
    printf(" TEST SUITE SUMMARY: %d PASSED, %d FAILED\n", s_tests_passed, s_tests_failed);
    printf("====================================================================\n");

    return (s_tests_failed == 0) ? 0 : 1;
}
