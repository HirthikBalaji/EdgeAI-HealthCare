/*
 * VITALIS-Edge: SPI NOR Flash Circular Blackbox Storage Driver
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Hardware Target: Microchip SST26VF064B (64 Mbit SPI Flash)
 * Logs continuous circular 60-second pre-event and post-event raw bio-waveforms.
 */

#ifndef BLACKBOX_STORAGE_H
#define BLACKBOX_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLACKBOX_SECTOR_SIZE_BYTES  4096
#define BLACKBOX_TOTAL_SECTORS      2048 /* 8 MB total Flash */
#define BLACKBOX_EVENT_MAGIC        0x56495441 /* "VITA" */

#pragma pack(push, 1)
typedef struct {
    uint32_t magic_header;      /* 0x56495441 */
    uint32_t event_id;
    uint32_t timestamp_ms;
    uint8_t triage_level;
    uint8_t ai_classification;
    float heart_rate;
    float spo2;
    float temperature;
    uint16_t sample_count;      /* Number of 5-channel samples logged */
    uint16_t crc16;
} blackbox_event_header_t;
#pragma pack(pop)

typedef struct {
    uint32_t write_sector_ptr;
    uint32_t read_sector_ptr;
    uint32_t total_events_logged;
    bool is_flash_ready;
} blackbox_manager_t;

vitalis_status_t blackbox_init(blackbox_manager_t *mgr);
vitalis_status_t blackbox_log_event(blackbox_manager_t *mgr, const blackbox_event_header_t *header,
                                    const float *waveform_data, uint16_t data_len);
vitalis_status_t blackbox_read_event(blackbox_manager_t *mgr, uint32_t event_index,
                                     blackbox_event_header_t *out_header, float *out_waveform_data);
uint32_t blackbox_get_logged_count(const blackbox_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* BLACKBOX_STORAGE_H */
