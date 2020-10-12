#ifndef RAW_SOCKET_HH
#define RAW_SOCKET_HH

#include <string>
#include <vector>
#include <chrono>

#include <pcap.h>

enum LinkType {
	       DATA_LINK,
	       FEC_LINK,
	       WFB_LINK,
	       SHORT_DATA_LINK,
	       RTS_DATA_LINK
};

struct monitor_message_t {
  monitor_message_t(size_t data_size = 0) :
    data(data_size), port(0), link_type(DATA_LINK), rssi(0), rate(0), channel(0),
    channel_flag(0), radiotap_flags(0) {}
  std::vector<uint8_t> data;
  uint8_t port;
  LinkType link_type;
  uint8_t rate;
  uint16_t channel;
  uint16_t channel_flag;
  uint8_t radiotap_flags;
  int8_t rssi;
  uint16_t lock_quality;
  uint16_t latency_ms;
  double recv_time;
  std::vector<uint8_t> antennas;
  std::vector<int8_t> rssis;
};

// Get a list of all the network device names
bool detect_network_devices(std::vector<std::string> &ifnames);

bool set_wifi_up_down(const std::string &device, bool up);
inline bool set_wifi_up(const std::string &device) {
  return set_wifi_up_down(device, true);
}
inline bool set_wifi_down(const std::string &device) {
  return set_wifi_up_down(device, false);
}
bool set_wifi_monitor_mode(const std::string &device);
bool set_wifi_frequency(const std::string &device, uint32_t freq_mhz);
bool set_wifi_txpower_fixed(const std::string &device);
bool set_wifi_txpower(const std::string &device, uint32_t power_mbm);

class RawSendSocket {
public:
  RawSendSocket(bool ground, uint32_t buffer_size = 131072, uint32_t max_packet = 65535);

  bool error() const {
    return (m_sock < 0);
  }

  bool add_device(const std::string &device, bool silent = true,
                  bool mcs = false, bool stbc = false, bool ldpc = false);

  // Copy the message into the send bufer and send it.
  bool send(const uint8_t *msg, size_t msglen, uint8_t port, LinkType type,
	    uint8_t datarate = 18);
  bool send(const std::vector<uint8_t> &msg, uint8_t port, LinkType type,
	    uint8_t datarate = 18) {
    return send(msg.data(), msg.size(), port, type, datarate);
  }

private:
  bool m_ground;
  bool m_mcs;
  bool m_stbc;
  bool m_ldpc;
  uint32_t m_max_packet;
  uint32_t m_buffer_size;
  int m_sock;
  std::vector<uint8_t> m_send_buf;
};

class RawReceiveSocket {
public:
  
  RawReceiveSocket(bool ground, uint32_t max_packet = 65535);

  bool add_device(const std::string &device);

  bool receive(monitor_message_t &msg, std::chrono::duration<double> timeout) const;

private:
  bool m_ground;
  uint32_t m_max_packet;
  pcap_t *m_ppcap;
  int m_selectable_fd;
};

#endif // RAW_SOCKET_HH
