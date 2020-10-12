
#pragma once

#include <string>
#include <thread>
#include <vector>

#include <INIReader.h>

#include <log_thread.hh>

bool archive_loop(std::string archive_dir, PacketQueueP q);

int open_udp_socket_for_rx(uint16_t port, const std::string hostname, uint32_t timeout_us);

void udp_raw_thread(int sock, uint16_t recv_udp_port, std::shared_ptr<FECEncoder> enc,
                    WifiOptions opts, uint8_t priority, uint16_t blocksize,
                    SharedQueue<std::shared_ptr<Message> > &outqueue, uint8_t port,
                    bool do_fec, uint16_t rate_target, PacketQueueP archive_inqueue);

bool create_udp_to_raw_threads(SharedQueue<std::shared_ptr<Message> > &outqueue,
			       std::vector<std::shared_ptr<std::thread> > &thrs,
			       const INIReader &conf,
			       TransferStats &trans_stats,
			       TransferStats &trans_stats_other,
                               PacketQueues &log_out,
                               PacketQueues &packed_log_out,
			       const std::string &mode,
                               uint32_t max_queue_size);
