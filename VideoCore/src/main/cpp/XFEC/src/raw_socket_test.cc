
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <iostream>

#include <cxxopts.hpp>

#include <wifibroadcast/fec.hh>
#include <wifibroadcast/raw_socket.hh>
#include <logging.hh>

#define MAX_MESSAGE 65535

class USecTimer {
public:
  USecTimer() {
    struct timeval t;
    gettimeofday(&t, 0);
    m_start_second = t.tv_sec;
  }

  uint64_t usec() const {
    struct timeval t;
    gettimeofday(&t, 0);
    return static_cast<uint64_t>(t.tv_sec - m_start_second) * 1000000 +
      static_cast<uint64_t>(t.tv_usec);
  }

private:
  time_t m_start_second;
};


int main(int argc, char** argv) {

  // Ensure we've running with root privileges
  auto uid = getuid();
  if (uid != 0) {
    std::cerr << "This application must be run as root or with root privileges.\n";
    return EXIT_FAILURE;
  }

  uint8_t port;
  uint64_t period;
  uint16_t length;
  float frequency;
  uint8_t datarate;
  uint8_t num_data_blocks;
  uint8_t num_fec_blocks;
  std::string message;
  std::string device;
  cxxopts::Options options(argv[0], "Allowed options");
  options.add_options()
    ("h,help", "produce help message")
    ("v,verbose", "produce verbose logging")
    ("p,port", "the port number (0-16) to send to",
     cxxopts::value<uint8_t>(port)->default_value("0"))
    ("d,datarate", "the MCS datarate",
     cxxopts::value<uint8_t>(datarate)->default_value("3"))
    ("l,length", "the length of message to send",
     cxxopts::value<uint16_t>(length)->default_value("1024"))
    ("D,data_blocks", "the number of data blocks per FEC sequence",
     cxxopts::value<uint8_t>(num_data_blocks)->default_value("0"))
    ("F,fec_blocks", "the number of fec blocks per FEC sequence",
     cxxopts::value<uint8_t>(num_fec_blocks)->default_value("0"))
    ("r,receiver", "receive messages instead of sending them")
    ("m,message", "the test message to send",
     cxxopts::value<std::string>(message))
    ("P,period", "the time between messages in microseconds",
     cxxopts::value<uint64_t>(period)->default_value("0"))
    ("f,frequency", "change the frequency of the wifi channel",
     cxxopts::value<float>(frequency)->default_value("0"))
    ("device", "the wifi device to connect to",
     cxxopts::value<std::string>(device))
    ;
  options.parse_positional({"device"});

  auto args = options.parse(argc, argv);
  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    std::cout << "Positional parameters: <device>\n";
    return EXIT_SUCCESS;
  }
  bool receiver = args.count("receiver");

  // Configure logging
  log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
  appender1->setLayout(new log4cpp::BasicLayout());
  log4cpp::Category& root = log4cpp::Category::getRoot();
  root.setPriority(args.count("verbose") ? log4cpp::Priority::DEBUG : log4cpp::Priority::INFO);
  root.addAppender(appender1);

  // Make sure the adapter is in monitor mode.
  if (!set_wifi_monitor_mode(device)) {
    LOG_ERROR << "Error configuring the device in monitor mode";
    return EXIT_FAILURE;
  }

  // Set the frequency if the user requested it.
  if (args.count("frequency")) {
    if (!set_wifi_frequency(device, frequency)) {
      LOG_ERROR << "Error changing the frequency to: " << frequency;
      return EXIT_FAILURE;
    } else {
      LOG_DEBUG << "Frequency changed to: " << frequency;
    }
  }

  // Allocate a message buffer
  uint8_t msgbuf[MAX_MESSAGE];
  USecTimer time;

  if (receiver) {
    LOG_DEBUG << "Receiving test messages on port " << static_cast<int>(port);

    // Create the FEC decoder
    FECDecoder dec;

    // Try to open the device
    RawReceiveSocket raw_recv_sock(true);
    if (raw_recv_sock.add_device(device)) {
      LOG_DEBUG << "Receiving on interface: " << device;
    } else {
      LOG_DEBUG << "Error opening the raw socket for transmiting: " << device;
      return EXIT_FAILURE;
    }

    monitor_message_t msg;
    uint16_t prev_pktid = 0;
    uint64_t bytes = 0;
    uint32_t blocks = 0;
    uint32_t packets = 0;
    uint32_t dropped_blocks = 0;
    uint32_t dropped_packets = 0;
    uint64_t prev_bytes = 0;
    uint32_t prev_blocks = 0;
    uint32_t prev_packets = 0;
    uint32_t prev_dropped_blocks = 0;
    uint32_t prev_dropped_packets = 0;
    uint64_t last_status = time.usec();
    while (raw_recv_sock.receive(msg, std::chrono::milliseconds(200))) {
      size_t len = msg.data.size();
      if (len > 0) {
        dec.add_block(msg.data.data(), msg.data.size());
      }
      for (std::shared_ptr<FECBlock> block = dec.get_block(); block;
           block = dec.get_block()) {
      }
      uint64_t t = time.usec();
      if ((t - last_status) > 1000000) {
        bytes = dec.stats().bytes;
        blocks = dec.stats().total_blocks;
        packets = dec.stats().total_packets;
        dropped_packets = dec.stats().dropped_packets;
        dropped_blocks = dec.stats().dropped_blocks;
        float mbps = static_cast<float>(bytes - prev_bytes) * 8.0 / 1e6;
        LOG_INFO << mbps << " Mbps  Packets (eb/ep/tot): "
                 << (dropped_blocks - prev_dropped_blocks) << "/"
                 << (dropped_packets - prev_dropped_packets) << "/"
                 << (packets - prev_packets);
        prev_bytes = bytes;
        prev_dropped_blocks = dropped_blocks;
        prev_dropped_packets = dropped_packets;
        prev_packets = packets;
        last_status = t;
      }
    }

  } else {
    LOG_DEBUG << "Sending test messages to port " << static_cast<int>(port);

    // Create the FEC encoder
    FECEncoder enc(num_data_blocks, num_fec_blocks, length);

    // Try to open the device
    RawSendSocket raw_send_sock(false);
    if (raw_send_sock.add_device(device, true, true, true, true)) {
      LOG_DEBUG << "Transmitting on interface: " << device;
    } else {
      LOG_DEBUG << "Error opening the raw socket for transmiting: " << device;
      return EXIT_FAILURE;
    }

    uint64_t send_time = 0;
    while (1) {
      uint64_t t = time.usec();
      if ((t - send_time) >= period) {
        {
          std::shared_ptr<FECBlock> block = enc.get_next_block(length);
          if (!message.empty()) {
            memcpy(block->data(), reinterpret_cast<const uint8_t*>(message.c_str()), message.size());
            length = message.size();
          } else if (length > 0) {
            memset(block->data(), 0, length);
          } else {
            continue;
          }
          enc.add_block(block);
        }
        for (std::shared_ptr<FECBlock> block = enc.get_block(); block;
             block = enc.get_block()) {
          raw_send_sock.send(block->pkt_data(), block->pkt_length(), port, DATA_LINK,
                             datarate);
        }
        if (period == 0) {
          break;
        }
        send_time = t;
      }
    }
  }
}
