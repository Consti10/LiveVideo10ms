//
// Created by Consti10 on 25/04/2019.
//

#ifndef FPV_VR_PRIVATE_WFB_TELEMETRY_DATA_H
#define FPV_VR_PRIVATE_WFB_TELEMETRY_DATA_H

#include "stdio.h"
#include "stdbool.h"


typedef struct {
    uint32_t received_packet_cnt;
    int8_t current_signal_dbm;
    int8_t type; // 0 = Atheros, 1 = Ralink
} __attribute__((packed)) wifi_adapter_rx_status_forward_t;


typedef struct {
    uint32_t damaged_block_cnt; // number bad blocks video downstream
    uint32_t lost_packet_cnt; // lost packets video downstream
    uint32_t skipped_packet_cnt; // skipped packets video downstream
    uint32_t received_packet_cnt; // packets received video downstream
    uint32_t kbitrate; // live video kilobitrate per second video downstream
    uint32_t kbitrate_measured; // max measured kbitrate during tx startup
    uint32_t kbitrate_set; // set kilobitrate (measured * bitrate_percent) during tx startup
    uint32_t lost_packet_cnt_telemetry_up; // lost packets telemetry uplink
    uint32_t lost_packet_cnt_telemetry_down; // lost packets telemetry downlink
    uint32_t lost_packet_cnt_msp_up; // lost packets msp uplink (not used at the moment)
    uint32_t lost_packet_cnt_msp_down; // lost packets msp downlink (not used at the moment)
    uint32_t lost_packet_cnt_rc; // lost packets rc link
    int8_t current_signal_air; // signal strength in dbm at air pi (telemetry upstream and rc link)
    int8_t joystick_connected; // 0 = no joystick connected, 1 = joystick connected
    uint8_t cpuload_gnd; // CPU load Ground Pi
    uint8_t temp_gnd; // CPU temperature Ground Pi
    uint8_t cpuload_air; // CPU load Air Pi
    uint8_t temp_air; // CPU temperature Air Pi
    uint32_t wifi_adapter_cnt; // number of wifi adapters
    wifi_adapter_rx_status_forward_t adapter[6]; // see struct above
} __attribute__((packed)) wifibroadcast_rx_status_forward_t;


typedef struct {
    uint32_t received_packet_cnt;
    int8_t current_signal_dbm;
    int8_t type; // 0 = Atheros, 1 = Ralink
    int8_t signal_good;
} __attribute__((packed)) wifi_adapter_rx_status_forward_t2;

typedef struct {
    uint32_t damaged_block_cnt; // number bad blocks video downstream
    uint32_t lost_packet_cnt; // lost packets video downstream
    uint32_t skipped_packet_cnt; // skipped packets video downstream
    uint32_t injection_fail_cnt;  // Video injection failed downstream
    uint32_t received_packet_cnt; // packets received video downstream
    uint32_t kbitrate; // live video kilobitrate per second video downstream
    uint32_t kbitrate_measured; // max measured kbitrate during tx startup
    uint32_t kbitrate_set; // set kilobitrate (measured * bitrate_percent) during tx startup
    uint32_t lost_packet_cnt_telemetry_up; // lost packets telemetry uplink
    uint32_t lost_packet_cnt_telemetry_down; // lost packets telemetry downlink
    uint32_t lost_packet_cnt_msp_up; // lost packets msp uplink (not used at the moment)
    uint32_t lost_packet_cnt_msp_down; // lost packets msp downlink (not used at the moment)
    uint32_t lost_packet_cnt_rc; // lost packets rc link
    int8_t current_signal_joystick_uplink; // signal strength in dbm at air pi (telemetry upstream and rc link)
    int8_t current_signal_telemetry_uplink;
    int8_t joystick_connected; // 0 = no joystick connected, 1 = joystick connected
    float HomeLat;
    float HomeLon;
    uint8_t cpuload_gnd; // CPU load Ground Pi
    uint8_t temp_gnd; // CPU temperature Ground Pi
    uint8_t cpuload_air; // CPU load Air Pi
    uint8_t temp_air; // CPU temperature Air Pi
    uint32_t wifi_adapter_cnt; // number of wifi adapters
    wifi_adapter_rx_status_forward_t2 adapter[6]; // same struct as in wifibroadcast lib.h
} __attribute__((packed)) wifibroadcast_rx_status_forward_t2;

static constexpr const auto WIFIBROADCAST_RX_STATUS_FORWARD_SIZE_BYTES=sizeof(wifibroadcast_rx_status_forward_t);
static constexpr const auto WIFIBROADCAST_RX_STATUS_FORWARD_2_SIZE_BYTES=sizeof(wifibroadcast_rx_status_forward_t2);

#endif //FPV_VR_PRIVATE_WFB_TELEMETRY_DATA_H
