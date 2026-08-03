/*
 * VITALIS-Edge: System Global Configuration & Tuning Parameters
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Hardware Target: Nordic nRF5340 / STMicroelectronics STM32U585
 * Ecosystem: Analog Devices, STMicroelectronics, Texas Instruments, Microchip, Murata, Molex
 */

#ifndef VITALIS_CONFIG_H
#define VITALIS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Firmware Metadata --- */
#define VITALIS_FW_VERSION_MAJOR    1
#define VITALIS_FW_VERSION_MINOR    0
#define VITALIS_FW_VERSION_PATCH    0
#define VITALIS_DEVICE_NAME         "VITALIS-Edge-01"

/* --- Biosignal Sampling Frequencies --- */
#define VITALIS_ECG_SAMPLE_RATE_HZ      250  /* Analog Devices AD8232 / MAX30003 */
#define VITALIS_PPG_SAMPLE_RATE_HZ      250  /* Maxim/ADI MAX30102 */
#define VITALIS_IMU_SAMPLE_RATE_HZ      250  /* STMicroelectronics LSM6DSOX */
#define VITALIS_TEMP_SAMPLE_RATE_HZ     1    /* Texas Instruments TMP117 */

/* --- TinyML Windowing --- */
#define VITALIS_WINDOW_SAMPLES          250  /* 1.0 second sliding window */
#define VITALIS_WINDOW_HOP_SAMPLES      125  /* 50% overlap (every 500ms) */
#define VITALIS_NUM_CHANNELS            5    /* ECG, PPG-IR, PPG-Red, AccelMag, Temp */

/* --- Clinical Thresholds --- */
#define VITALIS_CRITICAL_HR_HIGH        160  /* bpm - Tachycardia alert */
#define VITALIS_CRITICAL_HR_LOW         40   /* bpm - Severe Bradycardia alert */
#define VITALIS_CRITICAL_SPO2_MIN       90.0f/* % SpO2 - Hypoxia threshold */
#define VITALIS_FEVER_TEMP_THRESHOLD    38.3f/* °C - Pyrexia threshold */
#define VITALIS_HYPOTHERMIA_THRESHOLD   35.0f/* °C - Hypothermia */
#define VITALIS_FALL_IMPACT_G_THRESH    3.2f /* g - Impact acceleration spike */
#define VITALIS_FALL_IMMOBILITY_SEC     5    /* seconds of stillness following impact */

/* --- Triage Severity Levels --- */
typedef enum {
    TRIAGE_LEVEL_NORMAL = 0,        /* Green: All vitals normal */
    TRIAGE_LEVEL_OBSERVATION = 1,   /* Yellow: Mild anomaly (NEWS2 score 1-4) */
    TRIAGE_LEVEL_WARNING = 2,       /* Orange: High risk decompensation (NEWS2 score 5-6) */
    TRIAGE_LEVEL_CRITICAL = 3       /* Red: Life threat (Arrhythmia / Hypoxia / Fall / NEWS2 >= 7) */
} vitalis_triage_level_t;

/* --- System Return Codes --- */
typedef enum {
    VITALIS_OK = 0,
    VITALIS_ERR_SENSOR_INIT = -1,
    VITALIS_ERR_I2C_COMM = -2,
    VITALIS_ERR_SPI_COMM = -3,
    VITALIS_ERR_BUFFER_OVERFLOW = -4,
    VITALIS_ERR_INFERENCE_FAIL = -5,
    VITALIS_ERR_FLASH_WRITE = -6,
    VITALIS_ERR_INVALID_PARAM = -7
} vitalis_status_t;

#ifdef __cplusplus
}
#endif

#endif /* VITALIS_CONFIG_H */
