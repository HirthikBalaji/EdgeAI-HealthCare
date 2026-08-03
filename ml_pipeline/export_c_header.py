"""
VITALIS-Edge: TinyML C Header Exporter (model_data.h)
Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026

Generates production-grade C header file with:
  1. Flat static const int8_t weight arrays
  2. Floating/Fixed-point quantization scales
  3. Memory buffer sizing macros
  4. Layer dimension definitions
"""

import numpy as np
import os
import sys

def format_c_array(arr, name, dtype="int8_t", elements_per_line=12):
    flat = arr.flatten()
    lines = []
    lines.append(f"static const {dtype} {name}[{len(flat)}] = {{")
    
    current_line = "    "
    for i, val in enumerate(flat):
        if dtype == "int8_t":
            current_line += f"{int(val):4d}, "
        else:
            current_line += f"{float(val):.7ff}, "
        
        if (i + 1) % elements_per_line == 0 and (i + 1) < len(flat):
            lines.append(current_line)
            current_line = "    "
            
    if current_line.strip():
        lines.append(current_line)
    lines.append("};\n")
    return "\n".join(lines)

def export_model_to_c_header(weights_path, output_header_path):
    print(f"Loading weights from: {weights_path}")
    data = np.load(weights_path)

    os.makedirs(os.path.dirname(output_header_path), exist_ok=True)

    header_content = """/*
 * VITALIS-Edge: Autonomous Multi-Modal Edge AI Healthcare System
 * DigiKey Presents electronica India Tech Challenge 2026
 * 
 * Auto-generated 8-bit Quantized Neural Network Weights & Biases
 * Model Architecture: 1D-DSCNN-SE (Depthwise Separable Conv1D + Squeeze & Excitation)
 * Target Silicon: Nordic nRF5340 / STM32U585 / ARM Cortex-M33
 * Zero Heap Allocation - Pure Static Buffer Execution
 */

#ifndef VITALIS_MODEL_DATA_H
#define VITALIS_MODEL_DATA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Model Architecture Dimensions --- */
#define VITALIS_INPUT_CHANNELS      5
#define VITALIS_INPUT_SAMPLES       250
#define VITALIS_NUM_CLASSES         6

/* Layer 1: Conv1D (5 -> 32, kernel=15, stride=2, pad=7) */
#define CONV1D_FILTERS              32
#define CONV1D_KERNEL               15
#define CONV1D_STRIDE               2
#define CONV1D_PAD                  7
#define CONV1D_OUT_LEN              125 /* (250 + 2*7 - 15)/2 + 1 = 125 */

/* Layer 1b: MaxPool1D (kernel=2, stride=2) */
#define POOL1_OUT_LEN               62  /* 125 / 2 = 62 */

/* Layer 2: Depthwise Conv1D (32 ch, kernel=5, stride=1, pad=2) */
#define DWCONV1D_KERNEL             5
#define DWCONV1D_STRIDE             1
#define DWCONV1D_PAD                2
#define DWCONV1D_OUT_LEN            62  /* (62 + 2*2 - 5)/1 + 1 = 62 */

/* Layer 3: Pointwise Conv1D (32 -> 32, kernel=1, stride=1) */
#define PWCONV1D_FILTERS            32
#define PWCONV1D_OUT_LEN            62

/* Layer 4: Squeeze-and-Excitation (32 -> 8 -> 32) */
#define SE_REDUCTION_DIM            8

/* Class Index Enumerations */
typedef enum {
    VITALIS_CLASS_NSR = 0,               /* Normal Sinus Rhythm */
    VITALIS_CLASS_AFIB = 1,              /* Atrial Fibrillation */
    VITALIS_CLASS_VTACH = 2,             /* Ventricular Tachycardia */
    VITALIS_CLASS_PVC = 3,               /* Premature Ventricular Contraction */
    VITALIS_CLASS_HYPOXIA = 4,           /* Acute Hypoxia Decompensation */
    VITALIS_CLASS_FALL = 5               /* Patient Fall with Immobility */
} vitalis_class_t;

/* --- Quantized Weights & Biases (INT8) --- */

"""
    header_content += format_c_array(data['W_c1'], "VITALIS_W_CONV1D")
    header_content += format_c_array(data['b_c1'], "VITALIS_B_CONV1D")
    header_content += format_c_array(data['W_dw'], "VITALIS_W_DWCONV1D")
    header_content += format_c_array(data['b_dw'], "VITALIS_B_DWCONV1D")
    header_content += format_c_array(data['W_pw'], "VITALIS_W_PWCONV1D")
    header_content += format_c_array(data['b_pw'], "VITALIS_B_PWCONV1D")
    header_content += format_c_array(data['W_se1'], "VITALIS_W_SE1")
    header_content += format_c_array(data['b_se1'], "VITALIS_B_SE1")
    header_content += format_c_array(data['W_se2'], "VITALIS_W_SE2")
    header_content += format_c_array(data['b_se2'], "VITALIS_B_SE2")
    header_content += format_c_array(data['W_fc'], "VITALIS_W_FC")
    header_content += format_c_array(data['b_fc'], "VITALIS_B_FC")

    # Quantization Scale Factors
    header_content += "/* --- Dequantization / Fixed-Point Scale Multipliers --- */\n"
    for k in ['W_c1', 'b_c1', 'W_dw', 'b_dw', 'W_pw', 'b_pw', 'W_se1', 'b_se1', 'W_se2', 'b_se2', 'W_fc', 'b_fc']:
        scale_val = float(data[f'scale_{k}'])
        header_content += f"#define VITALIS_SCALE_{k.upper()} ({scale_val:.8f}f)\n"

    header_content += """
#ifdef __cplusplus
}
#endif

#endif /* VITALIS_MODEL_DATA_H */
"""

    with open(output_header_path, "w") as f:
        f.write(header_content)

    print(f"Successfully exported C model header to: {output_header_path}")

if __name__ == "__main__":
    script_dir = os.path.dirname(__file__)
    weights_p = os.path.join(script_dir, "trained_quantized_weights.npz")
    out_h = os.path.join(script_dir, "..", "firmware", "include", "tinyml", "model_data.h")
    out_h = os.path.abspath(out_h)
    export_model_to_c_header(weights_p, out_h)
