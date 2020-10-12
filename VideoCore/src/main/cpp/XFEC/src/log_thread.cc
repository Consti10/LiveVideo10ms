
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <iomanip>
#include <thread>
#include <cmath>

#include <tinyformat.h>

#include <logging.hh>
#include <log_thread.hh>

template <typename tmpl__T>
double mbps(tmpl__T v1, tmpl__T v2, double time) {
  double diff = static_cast<double>(v1) - static_cast<double>(v2);
  return diff * 8e-6 / time;
}
template <typename tmpl__T>
uint32_t kbps(tmpl__T v1, tmpl__T v2, double time) {
  double diff = static_cast<double>(v1) - static_cast<double>(v2);
  return static_cast<uint32_t>(std::round(diff * 8e-3 / time));
}

// Send status messages to the other radio and log the FEC stats periodically
void log_thread(TransferStats &stats, TransferStats &stats_other, float syslog_period,
		float status_period, SharedQueue<std::shared_ptr<Message> > &outqueue,
		std::shared_ptr<Message> msg, PacketQueues &log_out,
                PacketQueues &packed_log_out) {
  bool is_ground = (stats.name() == "ground");

  uint32_t loop_period =
    static_cast<uint32_t>(std::round(1000.0 * ((syslog_period == 0) ? status_period :
					       ((status_period == 0) ? syslog_period :
						std::min(syslog_period, status_period)))));
  transfer_stats_t ps = stats.get_stats();
  transfer_stats_t pso = stats_other.get_stats();
  transfer_stats_t pps = stats.get_stats();
  float rssi = 0;
  float orssi = 0;
  float krate = 0;
  double last_stat = cur_time();
  double last_log = cur_time();
  double last_status = cur_time();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(loop_period));
    double t = cur_time();
    transfer_stats_t s = stats.get_stats();
    transfer_stats_t os = stats_other.get_stats();
    double loop_time = t - last_status;
    rssi = rssi * 0.9 + 0.1 * s.rssi;
    orssi = orssi * 0.9 + 0.1 * os.rssi;
    krate = krate * 0.9 + 0.1 * kbps(s.bytes_in, pps.bytes_in, loop_time);

    // Send status if it's time
    double stat_dur = t - last_stat;
    if (stat_dur > status_period) {

      // I don't see any reason to send status upstream currently.
      if (!is_ground) {
	std::shared_ptr<Message> omsg = msg->create(stats.serialize());
	outqueue.push(omsg);
      }

      // Send the local status out the UDP port
      std::string outmsg = stats.serialize();
      {
        Packet pkt = mkpacket(outmsg.c_str(), outmsg.c_str() + outmsg.length());
        for (auto q : log_out) {
          q->push(pkt);
        }
      }

      // Create the packed status message and send it.
      {
	wifibroadcast_rx_status_forward_t rxs;
	rxs.damaged_block_cnt = s.sequence_errors;
	rxs.lost_packet_cnt = s.block_errors;
	rxs.skipped_packet_cnt = 0;
	rxs.injection_fail_cnt = os.inject_errors;
	rxs.received_packet_cnt = s.blocks_in;
	rxs.kbitrate = krate;
	rxs.kbitrate_measured = 0;
	rxs.kbitrate_set = 0;
	rxs.lost_packet_cnt_telemetry_up = os.block_errors;
	rxs.lost_packet_cnt_telemetry_down = 0;
	rxs.lost_packet_cnt_msp_up = 0;
	rxs.lost_packet_cnt_msp_down = 0;
	rxs.lost_packet_cnt_rc = 0;
	rxs.current_signal_joystick_uplink = orssi;
	rxs.current_signal_telemetry_uplink = orssi;
	rxs.joystick_connected = 0;
	rxs.HomeLat = 0;
	rxs.HomeLon = 0;
	rxs.cpuload_gnd = 0;
	rxs.temp_gnd = 0;
	rxs.cpuload_air = 0;
	rxs.temp_air = 0;
	rxs.wifi_adapter_cnt = 1;
	rxs.adapter[0].received_packet_cnt = s.blocks_in;
	rxs.adapter[0].current_signal_dbm = rssi;
	rxs.adapter[0].type = 1;
	rxs.adapter[0].signal_good = (rssi > -100);
        Packet pkt = mkpacket(reinterpret_cast<uint8_t*>(&rxs),
                              reinterpret_cast<uint8_t*>(&rxs) + sizeof(rxs));
        for (auto q : packed_log_out) {
          q->push(pkt);
        }
	pps = s;
      }

      last_stat = t;
    }

    // Post a log message if it's time
    double log_dur = t - last_log;
    if (log_dur >= syslog_period) {
      std::string blks = is_ground ?
        tfm::format("%4d %4d %4d",
		    (s.blocks_in - ps.blocks_in),
		    (s.blocks_out - ps.blocks_out),
		    (s.block_errors - ps.block_errors)) :
        tfm::format("%4d %4d %4d",
		    (s.blocks_out - ps.blocks_out),
		    (s.blocks_in - ps.blocks_in),
		    (s.block_errors - ps.block_errors));
      std::string times = tfm::format("%3d %3d %3d",
                                      static_cast<uint32_t>(std::round(s.encode_time)),
                                      static_cast<uint32_t>(std::round(s.send_time)),
                                      static_cast<uint32_t>(std::round(s.pkt_time)));
      std::string rate = is_ground ?
	tfm::format("%5.2f %5.2f",
                    mbps(s.bytes_in, ps.bytes_in, log_dur),
		    mbps(s.bytes_out, ps.bytes_out, log_dur)) :
	tfm::format("%5.2f %5.2f",
		    mbps(s.bytes_out, ps.bytes_out, log_dur),
		    mbps(s.bytes_in, ps.bytes_in, log_dur));
      LOG_INFO << "Name   Interval Seq(r/e)  Blocks(d/u/e)  Inj  Rate(d/u)   Times(e/s/t)Us  LatMs RSSI";
      LOG_INFO << tfm::format
	("%-6s %4.2f s %4d %4d   %-14s %3d %s  %14s  %3d   %4d",
         stats.name(),
         std::round(log_dur),
         (s.sequences - ps.sequences),
         (s.sequence_errors - ps.sequence_errors),
         blks,
         (s.inject_errors - ps.inject_errors),
         rate,
         times,
         static_cast<uint32_t>(std::round(s.latency)),
         static_cast<int16_t>(std::round(rssi)));
      ps = s;
      std::string oblks = is_ground ?
	tfm::format("%4d %4d %4d",
		    (os.blocks_out - pso.blocks_out),
		    (os.blocks_in - pso.blocks_in),
		    (os.block_errors - pso.block_errors)) :
	tfm::format("%4d %4d %4d",
		    (os.blocks_in - pso.blocks_in),
		    (os.blocks_out - pso.blocks_out),
		    (os.block_errors - pso.block_errors));
      std::string otimes = tfm::format("%3d %3d %3d",
                                       static_cast<uint32_t>(std::round(os.encode_time)),
                                       static_cast<uint32_t>(std::round(os.send_time)),
                                       static_cast<uint32_t>(std::round(os.pkt_time)));
      std::string orate = is_ground ?
	tfm::format("%5.2f %5.2f",
		    mbps(os.bytes_out, pso.bytes_out, log_dur),
		    mbps(os.bytes_in, pso.bytes_in, log_dur)) :
	tfm::format("%5.2f %5.2f",
		    mbps(os.bytes_in, pso.bytes_in, log_dur),
                    mbps(os.bytes_out, pso.bytes_out, log_dur));
      LOG_INFO << tfm::format
	("%-6s %4.2f s %4d %4d   %-14s %3d %s  %14s  %3d   %4d",
         stats_other.name(),
         std::round(log_dur),
         (os.sequences - pso.sequences),
         (os.sequence_errors - pso.sequence_errors),
         oblks,
         (os.inject_errors - pso.inject_errors),
         orate,
         otimes,
         static_cast<uint32_t>(std::round(os.latency)),
         static_cast<int16_t>(std::round(orssi)));
      pso = os;
      last_log = t;
    }
    last_status = t;
  }
}
