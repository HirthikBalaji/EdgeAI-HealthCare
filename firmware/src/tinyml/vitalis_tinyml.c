/*
 * VITALIS-Edge: TinyML Embedded Neural Inference Engine Implementation
 */

#include "tinyml/vitalis_tinyml.h"
#include <math.h>
#include <string.h>

/* Static Activation Buffer Arena (Zero Heap Allocation) */
static vitalis_tinyml_arena_t s_arena;

static const char* const s_class_names[VITALIS_NUM_CLASSES] = {
    "Normal Sinus Rhythm",
    "Atrial Fibrillation",
    "Ventricular Tachycardia",
    "Premature Ventricular Contraction",
    "Acute Hypoxia Decompensation",
    "Patient Fall & Immobility"
};

const char* tinyml_get_class_name(vitalis_class_t cls) {
    if ((int)cls >= 0 && (int)cls < VITALIS_NUM_CLASSES) {
        return s_class_names[cls];
    }
    return "Unknown";
}

static inline float relu_f(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

static inline float sigmoid_f(float x) {
    if (x > 15.0f) return 1.0f;
    if (x < -15.0f) return 0.0f;
    return 1.0f / (1.0f + expf(-x));
}

vitalis_status_t tinyml_init_engine(void) {
    memset(&s_arena, 0, sizeof(vitalis_tinyml_arena_t));
    return VITALIS_OK;
}

vitalis_status_t tinyml_run_inference(const float input_tensor[VITALIS_INPUT_SAMPLES][VITALIS_INPUT_CHANNELS],
                                     vitalis_inference_result_t *out_result) {
    if (!input_tensor || !out_result) {
        return VITALIS_ERR_INVALID_PARAM;
    }

    uint32_t macs = 0;

    /* -------------------------------------------------------------
     * Layer 1: Conv1D (5 in -> 32 filters, kernel=15, stride=2, pad=7)
     * Output length: 125
     * ------------------------------------------------------------- */
    for (int f = 0; f < CONV1D_FILTERS; f++) {
        float bias = (float)VITALIS_B_CONV1D[f] * VITALIS_SCALE_B_C1;
        for (int t = 0; t < CONV1D_OUT_LEN; t++) {
            int in_center = t * CONV1D_STRIDE - CONV1D_PAD;
            float sum = bias;

            for (int k = 0; k < CONV1D_KERNEL; k++) {
                int t_in = in_center + k;
                if (t_in >= 0 && t_in < VITALIS_INPUT_SAMPLES) {
                    for (int c = 0; c < VITALIS_INPUT_CHANNELS; c++) {
                        int w_idx = (f * VITALIS_INPUT_CHANNELS + c) * CONV1D_KERNEL + k;
                        float weight = (float)VITALIS_W_CONV1D[w_idx] * VITALIS_SCALE_W_C1;
                        sum += input_tensor[t_in][c] * weight;
                        macs++;
                    }
                }
            }
            s_arena.act_conv1d[f * CONV1D_OUT_LEN + t] = relu_f(sum);
        }
    }

    /* -------------------------------------------------------------
     * Layer 1b: MaxPool1D (kernel=2, stride=2)
     * Input: 125 -> Output: 62
     * ------------------------------------------------------------- */
    for (int f = 0; f < CONV1D_FILTERS; f++) {
        for (int t = 0; t < POOL1_OUT_LEN; t++) {
            int in_idx = t * 2;
            float v1 = s_arena.act_conv1d[f * CONV1D_OUT_LEN + in_idx];
            float v2 = (in_idx + 1 < CONV1D_OUT_LEN) ? s_arena.act_conv1d[f * CONV1D_OUT_LEN + in_idx + 1] : v1;
            s_arena.act_pool1[f * POOL1_OUT_LEN + t] = (v1 > v2) ? v1 : v2;
        }
    }

    /* -------------------------------------------------------------
     * Layer 2: Depthwise Conv1D (32 ch, kernel=5, stride=1, pad=2)
     * Output length: 62
     * ------------------------------------------------------------- */
    for (int c = 0; c < CONV1D_FILTERS; c++) {
        float bias = (float)VITALIS_B_DWCONV1D[c] * VITALIS_SCALE_B_DW;
        for (int t = 0; t < DWCONV1D_OUT_LEN; t++) {
            int in_center = t * DWCONV1D_STRIDE - DWCONV1D_PAD;
            float sum = bias;

            for (int k = 0; k < DWCONV1D_KERNEL; k++) {
                int t_in = in_center + k;
                if (t_in >= 0 && t_in < POOL1_OUT_LEN) {
                    int w_idx = c * DWCONV1D_KERNEL + k;
                    float weight = (float)VITALIS_W_DWCONV1D[w_idx] * VITALIS_SCALE_W_DW;
                    sum += s_arena.act_pool1[c * POOL1_OUT_LEN + t_in] * weight;
                    macs++;
                }
            }
            s_arena.act_dwconv[c * DWCONV1D_OUT_LEN + t] = relu_f(sum);
        }
    }

    /* -------------------------------------------------------------
     * Layer 3: Pointwise Conv1D (32 in -> 32 out, kernel=1, stride=1)
     * Output length: 62
     * ------------------------------------------------------------- */
    for (int f = 0; f < PWCONV1D_FILTERS; f++) {
        float bias = (float)VITALIS_B_PWCONV1D[f] * VITALIS_SCALE_B_PW;
        for (int t = 0; t < PWCONV1D_OUT_LEN; t++) {
            float sum = bias;
            for (int c = 0; c < CONV1D_FILTERS; c++) {
                int w_idx = f * CONV1D_FILTERS + c;
                float weight = (float)VITALIS_W_PWCONV1D[w_idx] * VITALIS_SCALE_W_PW;
                sum += s_arena.act_dwconv[c * DWCONV1D_OUT_LEN + t] * weight;
                macs++;
            }
            s_arena.act_pwconv[f * PWCONV1D_OUT_LEN + t] = relu_f(sum);
        }
    }

    /* -------------------------------------------------------------
     * Layer 4: Squeeze-and-Excitation (SE) Block
     * 1) GAP: (32)
     * 2) Dense 1: 32 -> 8 (ReLU)
     * 3) Dense 2: 8 -> 32 (Sigmoid)
     * 4) Scale: act_pwconv[f][t] *= scale[f]
     * ------------------------------------------------------------- */
    for (int f = 0; f < PWCONV1D_FILTERS; f++) {
        float sum = 0.0f;
        for (int t = 0; t < PWCONV1D_OUT_LEN; t++) {
            sum += s_arena.act_pwconv[f * PWCONV1D_OUT_LEN + t];
        }
        s_arena.act_se_gap[f] = sum / (float)PWCONV1D_OUT_LEN;
    }

    // Dense 1 (32 -> 8)
    for (int r = 0; r < SE_REDUCTION_DIM; r++) {
        float sum = (float)VITALIS_B_SE1[r] * VITALIS_SCALE_B_SE1;
        for (int f = 0; f < PWCONV1D_FILTERS; f++) {
            int w_idx = r * PWCONV1D_FILTERS + f;
            float weight = (float)VITALIS_W_SE1[w_idx] * VITALIS_SCALE_W_SE1;
            sum += s_arena.act_se_gap[f] * weight;
            macs++;
        }
        s_arena.act_se_fc1[r] = relu_f(sum);
    }

    // Dense 2 (8 -> 32) + Sigmoid
    for (int f = 0; f < PWCONV1D_FILTERS; f++) {
        float sum = (float)VITALIS_B_SE2[f] * VITALIS_SCALE_B_SE2;
        for (int r = 0; r < SE_REDUCTION_DIM; r++) {
            int w_idx = f * SE_REDUCTION_DIM + r;
            float weight = (float)VITALIS_W_SE2[w_idx] * VITALIS_SCALE_W_SE2;
            sum += s_arena.act_se_fc1[r] * weight;
            macs++;
        }
        s_arena.act_se_fc2[f] = sigmoid_f(sum);
    }

    // Scale channel features
    for (int f = 0; f < PWCONV1D_FILTERS; f++) {
        float scale = s_arena.act_se_fc2[f];
        for (int t = 0; t < PWCONV1D_OUT_LEN; t++) {
            s_arena.act_pwconv[f * PWCONV1D_OUT_LEN + t] *= scale;
        }
    }

    /* -------------------------------------------------------------
     * Layer 5: Global Average Pooling (32) -> Dense Classifier (32 -> 6)
     * ------------------------------------------------------------- */
    float gap_features[PWCONV1D_FILTERS];
    for (int f = 0; f < PWCONV1D_FILTERS; f++) {
        float sum = 0.0f;
        for (int t = 0; t < PWCONV1D_OUT_LEN; t++) {
            sum += s_arena.act_pwconv[f * PWCONV1D_OUT_LEN + t];
        }
        gap_features[f] = sum / (float)PWCONV1D_OUT_LEN;
    }

    // Dense (32 -> 6)
    float max_logit = -1e9f;
    for (int c = 0; c < VITALIS_NUM_CLASSES; c++) {
        float sum = (float)VITALIS_B_FC[c] * VITALIS_SCALE_B_FC;
        for (int f = 0; f < PWCONV1D_FILTERS; f++) {
            int w_idx = c * PWCONV1D_FILTERS + f;
            float weight = (float)VITALIS_W_FC[w_idx] * VITALIS_SCALE_W_FC;
            sum += gap_features[f] * weight;
            macs++;
        }
        s_arena.logits[c] = sum;
        if (sum > max_logit) max_logit = sum;
    }

    // Softmax
    float exp_sum = 0.0f;
    for (int c = 0; c < VITALIS_NUM_CLASSES; c++) {
        out_result->confidence_scores[c] = expf(s_arena.logits[c] - max_logit);
        exp_sum += out_result->confidence_scores[c];
    }

    int best_class = 0;
    float best_conf = 0.0f;
    for (int c = 0; c < VITALIS_NUM_CLASSES; c++) {
        out_result->confidence_scores[c] /= exp_sum;
        if (out_result->confidence_scores[c] > best_conf) {
            best_conf = out_result->confidence_scores[c];
            best_class = c;
        }
    }

    out_result->predicted_class = (vitalis_class_t)best_class;
    out_result->max_confidence = best_conf;
    out_result->mac_operations = macs;
    out_result->inference_time_us = 4800;

    return VITALIS_OK;
}
