/*
 * VITALIS-Edge: Nordic BLE 5.3 GATT Telemetry & Emergency Alert Beacon Driver
 * Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026
 * 
 * Hardware Target: Nordic Semiconductor nRF5340 (Dual-Core BLE 5.3 SoC)
 * Directly advertises emergency alerts without internet connection.
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "vitalis_config.h"
#include "triage/triage_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 128-bit Custom UUIDs for VITALIS Health Service */
#define VITALIS_BLE_SVC_UUID_BASE   0x1B, 0xC5, 0xD5, 0xA5, 0x02, 0x00, 0x56, 0x5A, \
                                    0x45, 0x44, 0x47, 0x45, 0x01, 0x00, 0x00, 0x00

#define VITALIS_BLE_CHAR_TELEMETRY_UUID 0x0001
#define VITALIS_BLE_CHAR_ALERT_UUID     0x0002
#define VITALIS_BLE_CHAR_BLACKBOX_UUID  0x0003

#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp_ms;
    float heart_rate;
    float spo2;
    float temperature;
    uint8_t triage_level;
    uint8_t ai_class;
    float ai_confidence;
    uint8_t news2_score;
    uint8_t battery_percent;
} vitalis_ble_telemetry_packet_t;

typedef struct {
    uint32_t alert_timestamp_ms;
    uint8_t urgency_level;      /* 0: None, 1: Yellow, 2: Orange, 3: Red Emergency */
    uint8_t primary_condition;  /* vitalis_class_t */
    char alert_text[48];
} vitalis_ble_emergency_beacon_t;
#pragma pack(pop)

typedef struct {
    bool is_advertising;
    bool is_connected;
    uint16_t conn_handle;
    uint8_t battery_level;
} vitalis_ble_ctx_t;

vitalis_status_t ble_init_stack(vitalis_ble_ctx_t *ctx);
vitalis_status_t ble_send_telemetry_notification(vitalis_ble_ctx_t *ctx, const vitalis_ble_telemetry_packet_t *pkt);
vitalis_status_t ble_broadcast_emergency_beacon(vitalis_ble_ctx_t *ctx, const vitalis_ble_emergency_beacon_t *alert);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SERVICE_H */
