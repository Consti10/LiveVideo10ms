
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <fstream>

#include <logging.hh>
#include <udp_send.hh>
#include <udp_receive.hh>

extern double last_packet_time;

int open_udp_socket_for_rx(uint16_t port, const std::string hostname, uint32_t timeout_us) {

  // Try to open a UDP socket.
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    LOG_ERROR << "Error opening the UDP receive socket.";
    return -1;
  }

  // Set the socket options.
  int optval = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
  setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const void *)&optval, sizeof(optval));

  // Set a timeout to ensure that the end of a frame gets flushed
  if (timeout_us > 0) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout_us;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
  }

  // Find to the receive port
  struct sockaddr_in saddr;
  bzero((char *)&saddr, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);

  // Lookup the IP address from the hostname
  std::string ip;
  if (hostname != "") {
    ip = hostname_to_ip(hostname);
    saddr.sin_addr.s_addr = inet_addr(ip.c_str());
  } else {
    saddr.sin_addr.s_addr = INADDR_ANY;
  }

  if (bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    LOG_ERROR << "Error binding to the UDP receive socket: " << port;
    return -1;
  }

  return fd;
}

bool archive_loop(std::string archive_dir, PacketQueueP q) {

  // Try to create the archive directory
  if (!mkpath(archive_dir)) {
    return false;
  }

  std::string archive_file = archive_dir + "/" + datetime() + ".dat";
  LOG_INFO << "Archiving to " << archive_file;
  std::ofstream of(archive_file, std::fstream::out | std::fstream::binary);
  if (!of.good()) {
    LOG_ERROR << "Error opening archive file: " << archive_file;
    return false;
  }

  while (1) {
    Packet msg = q->pop();
    of.write(reinterpret_cast<const char*>(msg->data()), msg->size());
  }

  return true;
}

void udp_raw_thread(int sock, uint16_t recv_udp_port, std::shared_ptr<FECEncoder> enc,
                    WifiOptions opts, uint8_t priority, uint16_t blocksize,
                    SharedQueue<std::shared_ptr<Message> > &outqueue, uint8_t port,
                    bool do_fec, uint16_t rate_target, PacketQueueP archive_inqueue) {
  bool flushed = true;
  double last_send_time = 0;
  double send_rate = static_cast<double>(rate_target) / 1000.0;
  std::shared_ptr<Message> send_msg;

  while (1) {

    // Receive the next message.
    std::shared_ptr<Message> msg(new Message(blocksize, port, priority, opts, enc));
    ssize_t count = recv(sock, msg->msg.data(), blocksize, 0);

    // Did we receive a message to send?
    if (count > 0) {
      msg->msg.resize(count);

      // Add the mesage to the output queue
      outqueue.push(msg);

      // send the data to the archiver if requested.
      if (archive_inqueue) {
        archive_inqueue->push(mkpacket(msg->msg));
      }

      // Indicate that we have put data in the queue, so should flush if necessary
      flushed = false;

    } else {
      msg->msg.resize(0);
      outqueue.push(msg);
      flushed = true;
    }
  }
}

bool create_udp_to_raw_threads(SharedQueue<std::shared_ptr<Message> > &outqueue,
			       std::vector<std::shared_ptr<std::thread> > &thrs,
                               const INIReader &conf,
			       TransferStats &trans_stats,
			       TransferStats &trans_stats_other,
                               PacketQueues &log_out,
                               PacketQueues &packed_log_out,
			       const std::string &mode,
                               uint32_t max_queue_size) {

  // Extract a couple of global options.
  float syslog_period = conf.GetFloat("global", "syslogperiod", 5);
  float status_period = conf.GetFloat("global", "statusperiod", 0.2);

  // Create the the threads for receiving packets from UDP sockets
  // and relaying them to the raw socket interface.

  for (const auto &section : conf.Sections()) {

    // Ignore non-link sections
    if ((section.substr(0, 5) != "link-") || (section == "link-packed_status_down")) {
      continue;
    }

    // Only process uplink configuration entries.
    std::string direction = conf.Get(section, "direction", "");
    if (((direction == "down") && (mode == "air")) ||
	((direction == "up") && (mode == "ground"))) {

      // Get the name.
      std::string name = conf.Get(section, "name", "");

      // Get the UDP port number (required except for status).
      uint16_t inport = static_cast<uint16_t>(conf.GetInteger(section, "inport", 0));
      if ((inport == 0) && (section != "link-status_down") && (section != "link-status_up")) {
	LOG_CRITICAL << "No inport specified for " << section;
	return false;
      }

      // Get the remote hostname/ip (optional)
      std::string hostname = conf.Get(section, "inhost", "127.0.0.1");

      // Get the port number (required).
      uint8_t port = static_cast<uint16_t>(conf.GetInteger(section, "port", 0));
      if (port == 0) {
	LOG_CRITICAL << "No port specified for " << section;
	return false;
      }

      // Get the link type
      std::string type = conf.Get(section, "type", "data");

      // Get the priority (optional).
      uint8_t priority = static_cast<uint8_t>(conf.GetInteger(section, "priority", 100));

      // Get the FEC stats (optional).
      uint16_t blocksize = static_cast<uint16_t>(conf.GetInteger(section, "blocksize", 1500));
      uint8_t nblocks = static_cast<uint8_t>(conf.GetInteger(section, "blocks", 0));
      uint8_t nfec_blocks = static_cast<uint8_t>(conf.GetInteger(section, "fec", 0));
      bool do_fec = ((nblocks > 0) && (nfec_blocks > 0));

      // Allocate the encoder (blocks contain a 16 bit, 2 byte size field)
      static const uint8_t length_len = 2;
      std::shared_ptr<FECEncoder> enc(new FECEncoder(nblocks, nfec_blocks, blocksize + length_len));

      // Create the FEC encoder if requested.
      WifiOptions opts;
      if (type == "data"){
	opts.link_type = DATA_LINK;
      } else if (type == "short") {
	opts.link_type = SHORT_DATA_LINK;
      } else if (type == "rts") {
	opts.link_type = RTS_DATA_LINK;
      } else {
	opts.link_type = DATA_LINK;
      }
      opts.data_rate = static_cast<uint8_t>(conf.GetInteger(section, "datarate", 18));

      // See if this link has a rate target specified.
      uint16_t rate_target = static_cast<uint16_t>(conf.GetInteger(section, "rate_target", 0));

      // Create the logging thread if this is a status down channel.
      if ((section == "link-status_down") || (section == "link-status_up")) {

	// Create the stats logging thread.
	std::shared_ptr<Message> msg(new Message(blocksize, port, priority, opts, enc));
	auto logth = [&trans_stats, &trans_stats_other, syslog_period, status_period,
		      &outqueue, msg, &log_out, &packed_log_out]() {
	  log_thread(trans_stats, trans_stats_other, syslog_period, status_period, outqueue, msg,
                     log_out, packed_log_out);
	};
	thrs.push_back(std::shared_ptr<std::thread>(new std::thread(logth)));

      } else {

	// Try to open the UDP socket.
        // 1ms timeout for FEC links to support flushing
        // Update rate target every 100 uS
	uint32_t timeout_us = (rate_target > 0) ? 100 : (do_fec ? 1000 : 0);
	int udp_sock = open_udp_socket_for_rx(inport, hostname, timeout_us);
	if (udp_sock < 0) {
	  LOG_CRITICAL << "Error opening the UDP socket for " << name << "  ("
		       << hostname << ":" << port << ")";
	  return false;
	}

        // Create a file logger if requested
        std::string archive_dir = conf.Get(section, "archive_indir", "");
        PacketQueueP archive_queue;
        if (!archive_dir.empty()) {
          archive_queue.reset(new PacketQueue(max_queue_size, true));
          // Spawn the archive thread.
          std::shared_ptr<std::thread> archive_thread
            (new std::thread
             ([archive_queue, archive_dir]() {
                archive_loop(archive_dir, archive_queue);
              }));
          thrs.push_back(archive_thread);
        }

	// Create the receive thread for this socket
	auto uth =
          [udp_sock, port, enc, opts, priority, blocksize, &outqueue, inport,
           do_fec, rate_target, archive_queue]() {
            bool flushed = true;
            double send_rate = static_cast<double>(rate_target) / 1000.0;

            while (1) {

              // Receive the next message.
              std::shared_ptr<Message> msg(new Message(blocksize, port, priority, opts, enc));
              ssize_t count = recv(udp_sock, msg->msg.data(), blocksize, 0);
              double t = cur_time();

              // Did we receive a message to send?
              if (count > 0) {
                msg->msg.resize(count);
                outqueue.push(msg);
                flushed = false;

                // send the data to the archiver if requested.
                if (archive_queue) {
                  archive_queue->push(mkpacket(msg->msg));
                }
              } else {
                msg->msg.resize(0);
                outqueue.push(msg);
                flushed = true;
              }
            }
          };
        thrs.push_back(std::shared_ptr<std::thread>(new std::thread(uth)));
      }
    }    
  }

  return true;
}
