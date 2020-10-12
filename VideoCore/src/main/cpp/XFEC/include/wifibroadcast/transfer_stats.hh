
#pragma once

#include <string>
#include <mutex>

#include <wifibroadcast/fec.hh>

struct transfer_stats_t {
  transfer_stats_t(uint32_t _sequences = 0, uint32_t _blocks_in = 0, uint32_t _blocks_out = 0,
		   uint32_t _bytes_in = 0, uint32_t _bytes_out = 0, uint32_t _block_errors = 0,
		   uint32_t _sequence_errors = 0, uint32_t _inject_errors = 0,
		   float _encode_time = 0, float _send_time = 0, float _pkt_time = 0,
		   float _latency = 0, float _rssi= 0);
  uint32_t sequences;
  uint32_t blocks_in;
  uint32_t blocks_out;
  uint32_t sequence_errors;
  uint32_t block_errors;
  uint32_t inject_errors;
  uint32_t bytes_in;
  uint32_t bytes_out;
  float encode_time;
  float send_time;
  float pkt_time;
  float latency;
  float rssi;
};

class TransferStats {
public:

  TransferStats(const std::string &name);

  const std::string &name();

  void add(const FECDecoderStats &cur, const FECDecoderStats &prev);
  void add_rssi(int8_t rssi);
  void add_send_stats(uint32_t bytes, uint32_t nblocks, uint16_t inject_errors, uint32_t queue_size,
		      bool flush, float pkt_time);
  void add_encode_time(float t);
  void add_send_time(float t);
  void add_latency(uint8_t);
  transfer_stats_t get_stats();

  bool update(const std::string &s);

  void timeout();

  std::string serialize();

private:
  std::string m_name;
  float m_window;
  uint32_t m_seq;
  uint32_t m_blocks;
  uint32_t m_bytes;
  uint32_t m_block_errors;
  uint32_t m_seq_errors;
  uint32_t m_send_bytes;
  uint32_t m_send_blocks;
  uint32_t m_inject_errors;
  uint32_t m_flushes;
  float m_queue_size;
  float m_enc_time;
  float m_send_time;
  float m_pkt_time;
  float m_rssi;
  float m_latency;
  std::mutex m_mutex;
};

// Standard OpenHD stats structures.
typedef struct {
    uint32_t received_packet_cnt;
    int8_t current_signal_dbm;
    int8_t type; // 0 = Atheros, 1 = Ralink
    int8_t signal_good;
} __attribute__((packed)) wifi_adapter_rx_status_forward_t;

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
  wifi_adapter_rx_status_forward_t adapter[6]; // same struct as in wifibroadcast lib.h
} __attribute__((packed)) wifibroadcast_rx_status_forward_t;
