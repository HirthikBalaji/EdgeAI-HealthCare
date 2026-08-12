/*
 * VITALIS-Edge: Clinical Triage Decision Engine & NEWS2 Scoring
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Implements deterministic National Early Warning Score (NEWS2), Shock Index,
 * and Multi-Modal Decision Fusion with zero cloud dependency.
 */

#ifndef TRIAGE_ENGINE_H
#define TRIAGE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"
#include "tinyml/vitalis_tinyml.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float heart_rate_bpm;
    float spo2_percent;
    float temperature_c;
    float shock_index;
    bool patient_immobile;
    vitalis_class_t ai_classification;
    float ai_confidence;
    uint8_t news2_score;
    vitalis_triage_level_t triage_level;
    const char* clinical_recommendation;
    bool trigger_local_alarm;
    bool trigger_ble_emergency_broadcast;
    uint32_t timestamp_ms;
} vitalis_triage_decision_t;

typedef struct {
    uint8_t subscore_hr;
    uint8_t subscore_spo2;
    uint8_t subscore_temp;
    uint8_t subscore_loc; // Level of consciousness / immobility
    uint8_t total_news2;
} vitalis_news2_breakdown_t;

/* Triage Engine API */
void triage_init(void);
vitalis_status_t triage_evaluate(float hr_bpm, float spo2_pct, float temp_c, 
                                 bool fall_immobility, const vitalis_inference_result_t *ai_result,
                                 vitalis_triage_decision_t *out_decision);

vitalis_news2_breakdown_t triage_compute_news2(float hr_bpm, float spo2_pct, float temp_c, bool immobility);

#ifdef __cplusplus
}
#endif

#endif /* TRIAGE_ENGINE_H */
