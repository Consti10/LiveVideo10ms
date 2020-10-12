
#pragma once

#include <memory>

#include <udp_send.hh>
#include <wifibroadcast/transfer_stats.hh>

struct WifiOptions {
  WifiOptions(LinkType type = DATA_LINK, uint8_t rate = 18) :
    link_type(type), data_rate(rate) { }
  LinkType link_type;
  uint8_t data_rate;
};

struct Message {
  Message() : port(0), priority(0) {}
  Message(size_t max_packet, uint8_t p, uint8_t pri, WifiOptions opt,
	  std::shared_ptr<FECEncoder> e) :
    msg(max_packet), port(p), priority(pri), opts(opt), enc(e) { }
  std::shared_ptr<Message> create(const std::string &s) {
    std::shared_ptr<Message> ret(new Message(s.length(), port, priority, opts, enc));
    std::copy(s.begin(), s.end(), ret->msg.begin());
    return ret;
  }
  std::vector<uint8_t> msg;
  uint8_t port;
  uint8_t priority;
  WifiOptions opts;
  std::shared_ptr<FECEncoder> enc;
};

void log_thread(TransferStats &stats, TransferStats &stats_other, float syslog_period,
		float status_period, SharedQueue<std::shared_ptr<Message> > &outqueue,
		std::shared_ptr<Message> msg, PacketQueues &log_out, PacketQueues &packed_log_out);
