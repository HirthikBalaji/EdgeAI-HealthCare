/*
 * VITALIS-Edge: SPI NOR Flash Circular Blackbox Storage Implementation
 */

#include "triage/blackbox_storage.h"
#include <string.h>

#define SIMULATED_FLASH_SECTOR_COUNT 16
static uint8_t s_simulated_flash_sectors[SIMULATED_FLASH_SECTOR_COUNT][BLACKBOX_SECTOR_SIZE_BYTES];

static uint16_t compute_crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

vitalis_status_t blackbox_init(blackbox_manager_t *mgr) {
    if (!mgr) return VITALIS_ERR_INVALID_PARAM;
    memset(mgr, 0, sizeof(blackbox_manager_t));
    memset(s_simulated_flash_sectors, 0xFF, sizeof(s_simulated_flash_sectors));
    mgr->is_flash_ready = true;
    return VITALIS_OK;
}

vitalis_status_t blackbox_log_event(blackbox_manager_t *mgr, const blackbox_event_header_t *header,
                                    const float *waveform_data, uint16_t data_len) {
    if (!mgr || !mgr->is_flash_ready || !header) {
        return VITALIS_ERR_INVALID_PARAM;
    }

    uint32_t sector = mgr->write_sector_ptr % SIMULATED_FLASH_SECTOR_COUNT;
    uint8_t *sector_buf = s_simulated_flash_sectors[sector];

    // Copy header
    blackbox_event_header_t local_hdr = *header;
    local_hdr.magic_header = BLACKBOX_EVENT_MAGIC;
    local_hdr.event_id = mgr->total_events_logged + 1;
    local_hdr.sample_count = data_len;
    local_hdr.crc16 = compute_crc16((const uint8_t*)waveform_data, data_len * sizeof(float));

    memcpy(sector_buf, &local_hdr, sizeof(blackbox_event_header_t));

    // Copy payload
    if (waveform_data && data_len > 0) {
        uint32_t max_payload = BLACKBOX_SECTOR_SIZE_BYTES - sizeof(blackbox_event_header_t);
        uint32_t bytes_to_copy = data_len * sizeof(float);
        if (bytes_to_copy > max_payload) bytes_to_copy = max_payload;
        memcpy(sector_buf + sizeof(blackbox_event_header_t), waveform_data, bytes_to_copy);
    }

    mgr->write_sector_ptr = (mgr->write_sector_ptr + 1) % BLACKBOX_TOTAL_SECTORS;
    mgr->total_events_logged++;

    return VITALIS_OK;
}

vitalis_status_t blackbox_read_event(blackbox_manager_t *mgr, uint32_t event_index,
                                     blackbox_event_header_t *out_header, float *out_waveform_data) {
    if (!mgr || !mgr->is_flash_ready || !out_header) {
        return VITALIS_ERR_INVALID_PARAM;
    }

    uint32_t sector = event_index % SIMULATED_FLASH_SECTOR_COUNT;
    const uint8_t *sector_buf = s_simulated_flash_sectors[sector];

    memcpy(out_header, sector_buf, sizeof(blackbox_event_header_t));

    if (out_header->magic_header != BLACKBOX_EVENT_MAGIC) {
        return VITALIS_ERR_FLASH_WRITE;
    }

    if (out_waveform_data && out_header->sample_count > 0) {
        uint32_t max_payload = BLACKBOX_SECTOR_SIZE_BYTES - sizeof(blackbox_event_header_t);
        uint32_t bytes_to_copy = out_header->sample_count * sizeof(float);
        if (bytes_to_copy > max_payload) bytes_to_copy = max_payload;
        memcpy(out_waveform_data, sector_buf + sizeof(blackbox_event_header_t), bytes_to_copy);
    }

    return VITALIS_OK;
}

uint32_t blackbox_get_logged_count(const blackbox_manager_t *mgr) {
    return mgr ? mgr->total_events_logged : 0;
}
