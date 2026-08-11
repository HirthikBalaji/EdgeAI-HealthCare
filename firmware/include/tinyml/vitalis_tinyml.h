/*
 * VITALIS-Edge: TinyML Embedded Neural Inference Engine
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Hardware Target: Nordic nRF5340 / STM32U585 / ARM Cortex-M33
 * Zero Heap Allocation - Fully Deterministic Static Memory Execution
 */

#ifndef VITALIS_TINYML_H
#define VITALIS_TINYML_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"
#include "tinyml/model_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    vitalis_class_t predicted_class;
    float confidence_scores[VITALIS_NUM_CLASSES];
    float max_confidence;
    uint32_t inference_time_us;
    uint32_t mac_operations;
} vitalis_inference_result_t;

/* Static Memory Arena for Ping-Pong Layer Activation Buffers */
typedef struct {
    float act_conv1d[CONV1D_FILTERS * CONV1D_OUT_LEN];  /* 32 * 125 = 4,000 floats */
    float act_pool1[CONV1D_FILTERS * POOL1_OUT_LEN];    /* 32 * 62 = 1,984 floats */
    float act_dwconv[CONV1D_FILTERS * DWCONV1D_OUT_LEN];/* 32 * 62 = 1,984 floats */
    float act_pwconv[PWCONV1D_FILTERS * PWCONV1D_OUT_LEN];/* 32 * 62 = 1,984 floats */
    float act_se_gap[PWCONV1D_FILTERS];                 /* 32 floats */
    float act_se_fc1[SE_REDUCTION_DIM];                 /* 8 floats */
    float act_se_fc2[PWCONV1D_FILTERS];                 /* 32 floats */
    float logits[VITALIS_NUM_CLASSES];                  /* 6 floats */
} vitalis_tinyml_arena_t;

/* TinyML Engine API */
vitalis_status_t tinyml_init_engine(void);
vitalis_status_t tinyml_run_inference(const float input_tensor[VITALIS_INPUT_SAMPLES][VITALIS_INPUT_CHANNELS],
                                     vitalis_inference_result_t *out_result);
const char* tinyml_get_class_name(vitalis_class_t cls);

#ifdef __cplusplus
}
#endif

#endif /* VITALIS_TINYML_H */
