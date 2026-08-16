# VITALIS-Edge Pitch Deck
## DigiKey Presents electronica India Tech Challenge 2026

---

## SLIDE 1 — Title

### 🫀 VITALIS-Edge
**Autonomous Edge AI Healthcare Monitor**

> *Real-Time Clinical Triage. Zero Cloud. Zero Latency. On-Device.*

**DigiKey Presents electronica India Tech Challenge 2026**  
Theme: Edge AI & Healthcare | National Innovation Hackathon

---

## SLIDE 2 — The Problem

### Two Crises Converging

**🫀 17.9M cardiac deaths/year globally (WHO)**  
85% occur outside hospital settings, without warning

**🏥 India: 1 doctor per 1,457 patients**  
Rural areas: 1 per 25,000 — monitoring is impossible at scale

**☁️ Cloud-connected wearables fail when it matters most:**
- Unreliable 4G/WiFi in rural India, ICUs, ambulances
- 300–800ms cloud round-trip = too slow for cardiac events
- Patient health data uploaded to foreign servers = privacy violation (DPDPA 2023)
- Clinic-visit models catch problems **days too late**

**The gap: No autonomous, affordable, offline-capable continuous monitor exists for India.**

---

## SLIDE 3 — Our Solution

### VITALIS-Edge: Intelligence at the Point of Care

An ultra-compact **biocompatible wearable chest patch** that:

✅ **Monitors 5 biosignal channels continuously** at 250 Hz:  
ECG (Lead I), PPG SpO₂/HR, 6-DoF Motion, Skin Temperature

✅ **Detects 6 life-threatening conditions** in real time:  
AFib · V-Tach · PVC · Hypoxia · Fall · Normal Sinus

✅ **Runs a 4.33 KB INT8 neural network** on-device in **11.69 ms**

✅ **Dispatches BLE emergency alerts** within 50m — no internet needed

✅ **Operates for 16.8+ days** on a single 180 mAh LiPo cell

✅ **Zero cloud calls** — all inference, triage, and decision-making on the patch

---

## SLIDE 4 — Hardware Architecture

### Silicon Ecosystem (DigiKey-Sourced)

| Component | Manufacturer | Role |
|:--|:--|:--|
| **nRF5340** | Nordic Semiconductor | Dual ARM Cortex-M33 + BLE 5.3 SoC |
| **AD8232** | Analog Devices | ECG Analog Front-End |
| **MAX30102** | Maxim / ADI | Optical PPG + SpO₂ |
| **LSM6DSOX** | STMicroelectronics | 6-DoF IMU + ML Core |
| **TMP117** | Texas Instruments | ±0.1°C Clinical Temp |
| **SST26VF064B** | Microchip | 8 MB Blackbox Flash |
| **BQ25570** | Texas Instruments | Nano-Power PMIC |

**Form Factor:** 82 × 36 × 6.5 mm | 18.2g | IP67 | Biocompatible Silicone  
**BOM Cost:** ~$28 USD (all DigiKey-sourced)

---

## SLIDE 5 — AI Model Architecture

### 1D-DSCNN-SE: Designed for Cortex-M33

```
Input (250 samples × 5 channels)
    ↓
Conv1D (5→32, k=15, s=2) + BatchNorm + ReLU    → 125
    ↓
MaxPool1D (k=2, s=2)                            → 62
    ↓
DepthwiseConv1D (k=5) + BatchNorm + ReLU        → 62
    ↓
PointwiseConv1D (32→32, k=1) + BatchNorm + ReLU → 62
    ↓
Squeeze-and-Excitation (32→8→32)                → 62
    ↓
Global Average Pooling                          → 32
    ↓
Linear → Softmax (6 classes)
```

**Key innovation:** BatchNorm fusion + INT8 symmetric quantization  
**Result:** 4,526 parameters → **4.33 KB** Flash, **7.81 KB** SRAM

---

## SLIDE 6 — Clinical Performance

### 96.08% Accuracy · 19/19 Tests Passed

| Condition | Sensitivity | Specificity | F1-Score |
|:--|:--:|:--:|:--:|
| Normal Sinus Rhythm | 100% | 95.3% | 89.5% |
| Atrial Fibrillation | 76.5% | 100% | 86.7% |
| **Ventricular Tachycardia** | **100%** | **100%** | **100%** |
| Premature V. Contraction | 100% | 100% | 100% |
| **Acute Hypoxia** | **100%** | **100%** | **100%** |
| **Patient Fall** | **100%** | **100%** | **100%** |

**Critical conditions (VTACH, HYPOXIA, FALL) achieve 100% sensitivity — zero missed life-threatening events.**

---

## SLIDE 7 — Edge MCU Performance

### Benchmarked on ARM Cortex-M33

| Metric | Value |
|:--|:--|
| Neural Network Flash | **4.33 KB** (INT8) |
| SRAM Buffer | **7.81 KB** (static, no heap) |
| MACs / Inference | 374,112 |
| Inference @ 64 MHz | **11.69 ms** |
| Inference @ 160 MHz | **4.68 ms** |
| Energy / Inference | **18.4 µJ** |
| Average System Draw | **384 µA** |
| Battery Life | **> 16.8 Days** |

**Signal chain latency** (sensor sample → emergency alert): < **15 ms total**

---

## SLIDE 8 — Clinical Triage Engine

### NEWS2 + AI Fusion — All On-Device

**Three-stage deterministic fusion:**

1. **Neural Stage:** 1D-DSCNN-SE classifies the 1-second 5-channel biosignal window
2. **Scoring Stage:** NEWS2 (SpO₂ + HR + Temperature + Consciousness) computed locally
3. **Fusion Stage:** AI prediction × NEWS2 score × Shock Index → triage level

**Four triage levels:**
- 🟢 **NORMAL:** Routine monitoring, BLE telemetry 1×/sec
- 🟡 **OBSERVATION:** Waveform blackbox logging begins
- 🟠 **WARNING:** Haptic alert + BLE GATT notification
- 🔴 **CRITICAL:** Piezo alarm + BLE emergency broadcast (50m, no internet)

---

## SLIDE 9 — Privacy & Regulatory

### Privacy-by-Design

| Requirement | VITALIS-Edge Status |
|:--|:--|
| Cloud data egress | ✅ ZERO |
| DPDPA 2023 compliance | ✅ No personal data leaves device |
| BLE encryption | ✅ AES-CCM-128 |
| Flash encryption | ✅ nRF5340 CryptoCell-312 |
| Biocompatibility | ✅ USP Class VI Silicone (ISO 10993) |
| Ingress protection | ✅ IP67 (1m, 30 min) |
| Software structure | ✅ Aligned with IEC 62304 lifecycle |

---

## SLIDE 10 — Scalability & Impact

### Clinical & Commercial Pathway

**Use Case Targets:**
1. 🏥 **Hospital step-down units** — Wireless patient monitoring without bedside monitors
2. 🚑 **Ambulance / MICU** — Real-time triage during transport, zero connectivity needed
3. 🌾 **Rural India / Tier-3 clinics** — Offline-first cardiac screening
4. 🏠 **Home monitoring** — Post-discharge CHF / COPD / atrial fibrillation patients
5. 🚨 **Mass disaster response** — Field triage at scale (floods, earthquakes)

**Market potential:** India's remote patient monitoring market: ₹12,000 Crore by 2028 (CAGR 21%)

**VITALIS-Edge BOM cost: ~$28 USD** — vs. Holter monitors at $500–$2,000 and 12-lead ECG machines at $5,000–$30,000.

---

## SLIDE 11 — Deliverables Summary

### Complete Submission Package

| Deliverable | Status |
|:--|:--|
| Embedded C Firmware (sensors + DSP + TinyML + triage + BLE + flash) | ✅ Complete |
| Python TinyML Training Pipeline (PyTorch + INT8 quantization) | ✅ Complete |
| Auto-generated C Header (model_data.h with quantized weights) | ✅ Complete |
| DigiKey Bill of Materials (15 components, full part numbers) | ✅ Complete |
| Hardware Schematics & GPIO Pinout | ✅ Complete |
| Power Budget & Battery Life Analysis | ✅ Complete |
| Enclosure Mechanical Specifications | ✅ Complete |
| Live Web Dashboard (5-channel oscilloscope + TinyML visualization) | ✅ Complete |
| Technical Report (10-section, full mathematical derivations) | ✅ Complete |
| Firmware Test Suite (6 suites, 19/19 PASSED) | ✅ Complete |

---

## SLIDE 12 — Why VITALIS-Edge Wins

### Judging Criteria Alignment

| Judging Criterion | Our Score |
|:--|:--|
| **Innovation & Creativity** | 1D-DSCNN-SE + BN fusion + 5-modal fusion novel architecture |
| **Technical Excellence** | 96.08% accuracy · 4.33 KB · 11.69 ms · 19/19 tests passed |
| **Problem Understanding** | NEWS2 clinical standard · DPDPA privacy · India rural gap |
| **Feasibility & Scalability** | $28 BOM · all DigiKey PNs · IEC 62304 structure · IP67 |
| **User Experience** | Live dashboard · haptic + BLE alerts · 16.8 day battery |
| **Quality of Demonstration** | Live biosignal + TinyML simulator · compilable firmware |
| **Overall Impact** | Zero-cloud · 16.8 day · 50m BLE range · 96% F1 on critical |

---

*VITALIS-Edge — Intelligence at the Point of Care*  
*DigiKey Presents electronica India Tech Challenge 2026*
