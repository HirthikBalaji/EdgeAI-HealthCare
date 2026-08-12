/*
 * VITALIS-Edge: Clinical Triage Decision Engine & NEWS2 Scoring Implementation
 */

#include "triage/triage_engine.h"
#include <string.h>

void triage_init(void) {
    // Initialize triage state machine
}

vitalis_news2_breakdown_t triage_compute_news2(float hr_bpm, float spo2_pct, float temp_c, bool immobility) {
    vitalis_news2_breakdown_t score;
    memset(&score, 0, sizeof(score));

    // 1. Respiration / SpO2 scoring
    if (spo2_pct <= 91.0f) {
        score.subscore_spo2 = 3;
    } else if (spo2_pct <= 93.0f) {
        score.subscore_spo2 = 2;
    } else if (spo2_pct <= 95.0f) {
        score.subscore_spo2 = 1;
    } else {
        score.subscore_spo2 = 0;
    }

    // 2. Heart Rate (HR) scoring
    if (hr_bpm <= 40.0f || hr_bpm >= 131.0f) {
        score.subscore_hr = 3;
    } else if (hr_bpm >= 111.0f && hr_bpm <= 130.0f) {
        score.subscore_hr = 2;
    } else if ((hr_bpm >= 41.0f && hr_bpm <= 50.0f) || (hr_bpm >= 91.0f && hr_bpm <= 110.0f)) {
        score.subscore_hr = 1;
    } else {
        score.subscore_hr = 0; // 51-90 bpm
    }

    // 3. Temperature scoring
    if (temp_c <= 35.0f) {
        score.subscore_temp = 3; // Severe hypothermia
    } else if (temp_c >= 39.1f) {
        score.subscore_temp = 2; // Severe pyrexia
    } else if ((temp_c >= 35.1f && temp_c <= 36.0f) || (temp_c >= 38.1f && temp_c <= 39.0f)) {
        score.subscore_temp = 1;
    } else {
        score.subscore_temp = 0; // 36.1 - 38.0°C
    }

    // 4. Level of consciousness / Immobility
    if (immobility) {
        score.subscore_loc = 3; // Unresponsive / collapse
    } else {
        score.subscore_loc = 0; // Alert
    }

    score.total_news2 = score.subscore_spo2 + score.subscore_hr + score.subscore_temp + score.subscore_loc;
    return score;
}

vitalis_status_t triage_evaluate(float hr_bpm, float spo2_pct, float temp_c, 
                                 bool fall_immobility, const vitalis_inference_result_t *ai_result,
                                 vitalis_triage_decision_t *out_decision) {
    if (!ai_result || !out_decision) {
        return VITALIS_ERR_INVALID_PARAM;
    }

    out_decision->heart_rate_bpm = hr_bpm;
    out_decision->spo2_percent = spo2_pct;
    out_decision->temperature_c = temp_c;
    out_decision->patient_immobile = fall_immobility;
    out_decision->ai_classification = ai_result->predicted_class;
    out_decision->ai_confidence = ai_result->max_confidence;

    // Approximate Shock Index (SI = HR / SBP_est), using baseline SBP 115
    out_decision->shock_index = hr_bpm / 115.0f;

    // Compute NEWS2 Score
    vitalis_news2_breakdown_t news2 = triage_compute_news2(hr_bpm, spo2_pct, temp_c, fall_immobility);
    out_decision->news2_score = news2.total_news2;

    // Deterministic Multi-Modal Fusion Logic
    if (ai_result->predicted_class == VITALIS_CLASS_VTACH) {
        out_decision->triage_level = TRIAGE_LEVEL_CRITICAL;
        out_decision->clinical_recommendation = "CRITICAL: Ventricular Tachycardia detected. Immediate defibrillation/resuscitation required.";
        out_decision->trigger_local_alarm = true;
        out_decision->trigger_ble_emergency_broadcast = true;
    }
    else if (ai_result->predicted_class == VITALIS_CLASS_FALL || fall_immobility) {
        out_decision->triage_level = TRIAGE_LEVEL_CRITICAL;
        out_decision->clinical_recommendation = "CRITICAL: Patient fall with post-impact immobility detected. Dispatch emergency response.";
        out_decision->trigger_local_alarm = true;
        out_decision->trigger_ble_emergency_broadcast = true;
    }
    else if (ai_result->predicted_class == VITALIS_CLASS_HYPOXIA || spo2_pct < VITALIS_CRITICAL_SPO2_MIN) {
        out_decision->triage_level = TRIAGE_LEVEL_CRITICAL;
        out_decision->clinical_recommendation = "CRITICAL: Severe Hypoxemic Decompensation (SpO2 < 90%). Administer supplemental O2 immediately.";
        out_decision->trigger_local_alarm = true;
        out_decision->trigger_ble_emergency_broadcast = true;
    }
    else if (ai_result->predicted_class == VITALIS_CLASS_AFIB) {
        if (news2.total_news2 >= 3) {
            out_decision->triage_level = TRIAGE_LEVEL_WARNING;
            out_decision->clinical_recommendation = "WARNING: Atrial Fibrillation with elevated physiological strain (NEWS2 >= 3). Urgent clinical review.";
            out_decision->trigger_local_alarm = true;
            out_decision->trigger_ble_emergency_broadcast = true;
        } else {
            out_decision->triage_level = TRIAGE_LEVEL_OBSERVATION;
            out_decision->clinical_recommendation = "OBSERVATION: Atrial Fibrillation detected with stable hemodynamics. Logged to flash blackbox.";
            out_decision->trigger_local_alarm = false;
            out_decision->trigger_ble_emergency_broadcast = false;
        }
    }
    else if (ai_result->predicted_class == VITALIS_CLASS_PVC) {
        out_decision->triage_level = TRIAGE_LEVEL_OBSERVATION;
        out_decision->clinical_recommendation = "OBSERVATION: Premature Ventricular Contractions recorded. Frequency monitoring active.";
        out_decision->trigger_local_alarm = false;
        out_decision->trigger_ble_emergency_broadcast = false;
    }
    else {
        // Normal Sinus Rhythm
        if (news2.total_news2 >= 5) {
            out_decision->triage_level = TRIAGE_LEVEL_WARNING;
            out_decision->clinical_recommendation = "WARNING: Elevated systemic NEWS2 score (fever/tachycardia). Medical assessment advised.";
            out_decision->trigger_local_alarm = true;
            out_decision->trigger_ble_emergency_broadcast = true;
        } else if (news2.total_news2 >= 1) {
            out_decision->triage_level = TRIAGE_LEVEL_OBSERVATION;
            out_decision->clinical_recommendation = "OBSERVATION: Low-risk vital sign variation. Continuous monitoring maintained.";
            out_decision->trigger_local_alarm = false;
            out_decision->trigger_ble_emergency_broadcast = false;
        } else {
            out_decision->triage_level = TRIAGE_LEVEL_NORMAL;
            out_decision->clinical_recommendation = "NORMAL: All multi-modal physiological & kinematic parameters within healthy baseline.";
            out_decision->trigger_local_alarm = false;
            out_decision->trigger_ble_emergency_broadcast = false;
        }
    }

    return VITALIS_OK;
}
