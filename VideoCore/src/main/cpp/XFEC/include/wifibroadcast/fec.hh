#ifndef FEC_ENCODER_HH
#define FEC_ENCODER_HH

#include <stdint.h>
#include <memory.h>

#include <vector>
#include <memory>
#include <iostream>
#include <set>
#include <queue>

#include <wifibroadcast/fec.h>

typedef enum {
  FEC_PARTIAL,
  FEC_COMPLETE,
  FEC_ERROR
} FECStatus;

struct __attribute__((__packed__)) FECHeader {
  FECHeader() : seq_num(0), block(0), n_blocks(0), n_fec_blocks(0), length() {}
  uint8_t seq_num;
  uint8_t block;
  uint8_t n_blocks;
  uint8_t n_fec_blocks;
  uint16_t length;
};

class FECBlock {
public:
  FECBlock(uint8_t seq_num, uint8_t block, uint8_t nblocks, uint8_t nfec_blocks,
	   uint16_t max_block_len, uint16_t data_length) {
    m_data_length = max_block_len + sizeof(FECHeader) - 2;
    m_data.reset(new uint8_t[m_data_length]);
    std::fill(m_data.get(), m_data.get() + m_data_length, 0);
    FECHeader *h = header();
    h->seq_num = seq_num;
    h->block = block;
    h->n_blocks = nblocks;
    h->n_fec_blocks = nfec_blocks;
    h->length = data_length;
  }

  // Create a block from a packet buffer
  FECBlock(const uint8_t *buf, uint16_t pkt_length) {
    m_data_length = pkt_length;
    m_data.reset(new uint8_t[m_data_length]);
    std::fill(m_data.get(), m_data.get() + m_data_length, 0);
    std::copy(buf, buf + pkt_length, m_data.get());
  }

  // Create a block from an existing header
  FECBlock(const FECHeader &h, uint16_t block_length) {
    m_data_length = block_length + sizeof(FECHeader) - 2;
    m_data.reset(new uint8_t[m_data_length]);
    std::fill(m_data.get(), m_data.get() + m_data_length, 0);
    *header() = h;
    data_length(block_length - 2);
  }

  // The FEC block size, which includes the data and data length fields
  uint16_t block_size() const {
    return data_length() + 2;
  }

  // Change the block size to the desired FEC block size, which could be
  // smaller than the maximum block size.
  void adjust_block_size(uint16_t block_size) {
    m_data_length = block_size + sizeof(FECHeader) - 2;
  }

  // A pointer to the data (does not include the header or length fields)
  uint8_t *data() {
    return m_data.get() + sizeof(FECHeader);
  }
  const uint8_t *data() const {
    return data();
  }

  // The length of data (does not include the header or length fields)
  uint16_t data_length() const {
    return header()->length;
  }
  void data_length(uint16_t len) {
    header()->length = len;
  }

  // A pointer to the data that should be included in FEC (everything except the header)
  uint8_t *fec_data() {
    return m_data.get() + sizeof(FECHeader) - 2;
  }
  const uint8_t *fec_data() const {
    return fec_data();
  }

  // A pointer to the header
  FECHeader *header() {
    return reinterpret_cast<FECHeader*>(m_data.get());
  }
  const FECHeader *header() const {
    return reinterpret_cast<const FECHeader*>(m_data.get());
  }

  // A pointer to all packet data (header + length + data)
  uint8_t *pkt_data() {
    return m_data.get();
  }
  const uint8_t *pkt_data() const {
    return m_data.get();
  }

  // The length of the entire packet (header + length + data)
  uint16_t pkt_length() const {
    return m_data_length;
  }

  // What type of block is this, data or FEC?
  bool is_data_block() const {
    return header()->block < header()->n_blocks;
  }
  bool is_fec_block() const {
    return !is_data_block();
  }

  // The sequence number
  uint8_t seq_num() const {
    return header()->seq_num;
  }

private:
  std::unique_ptr<uint8_t> m_data;
  uint16_t m_data_length;
};

class FECEncoder {
public:

  FECEncoder(uint8_t num_blocks = 8, uint8_t num_fec_blocks = 4, uint16_t max_block_size = 1500,
	     uint8_t start_seq_num = 1);

  // Get a new data block
  std::shared_ptr<FECBlock> new_block() {
    return std::shared_ptr<FECBlock>(new FECBlock(0, 0, m_num_blocks, m_num_fec_blocks,
						  m_max_block_size, 0));
  }

  // Allocate and initialize the next data block.
  std::shared_ptr<FECBlock> get_next_block(uint16_t length = 0);

  // Add an incoming data block to be encoded
  void add_block(std::shared_ptr<FECBlock> block);

  // Retrieve the next data/fec block
  std::shared_ptr<FECBlock> get_block();
  size_t n_output_blocks() const {
    return m_out_blocks.size();
  }

  // Complete the sequence with the current set of blocks
  void flush();

private:
  uint8_t m_seq_num;
  uint8_t m_num_blocks;
  uint8_t m_num_fec_blocks;
  uint16_t m_max_block_size;
  std::vector<std::shared_ptr<FECBlock> > m_in_blocks;
  std::queue<std::shared_ptr<FECBlock> > m_out_blocks;

  void encode_blocks();
};


class FECBufferEncoder {
public:
  FECBufferEncoder(uint32_t maximum_block_size = 1460, float fec_ratio = 0.5) :
    m_max_block_size(maximum_block_size), m_fec_ratio(fec_ratio), m_seq_num(1) { }

  std::vector<std::shared_ptr<FECBlock> >
  encode_buffer(const uint8_t* buf, size_t length);
  std::vector<std::shared_ptr<FECBlock> >
  encode_buffer(const std::vector<uint8_t> &buf) {
    return encode_buffer(buf.data(), buf.size());
  }

  std::pair<uint32_t, double> test(uint32_t iterations);

private:
  uint32_t m_max_block_size;
  float m_fec_ratio;
  uint8_t m_seq_num;
};



struct FECDecoderStats {
  FECDecoderStats() : total_blocks(0), total_packets(0), dropped_blocks(0), dropped_packets(0),
		      lost_sync(0), bytes(0) {}
  size_t total_blocks;
  size_t total_packets;
  size_t dropped_blocks;
  size_t dropped_packets;
  size_t lost_sync;
  size_t bytes;
};

FECDecoderStats operator-(const FECDecoderStats& s1, const FECDecoderStats &s2);
FECDecoderStats operator+(const FECDecoderStats& s1, const FECDecoderStats &s2);

class FECDecoder {
public:

  FECDecoder();

  void add_block(const uint8_t *buf, uint16_t block_length);
  // Make sure to use pkt_data and pkt_length instead of data / data_length
  void add_block(std::shared_ptr<FECBlock> blk){
    add_block(blk->pkt_data(), blk->pkt_length());
  }

  // Retrieve the next data/fec block
  std::shared_ptr<FECBlock> get_block();

  const FECDecoderStats &stats() const {
    return m_stats;
  }

private:
  // The block size of the current sequence (0 on restart)
  uint16_t m_block_size;
  // The previous sequence number
  FECHeader m_prev_header;
  // The blocks that have been received previously for this sequence
  std::vector<std::shared_ptr<FECBlock> > m_blocks;
  // The FEC blocks that have been received previously for this sequence
  std::vector<std::shared_ptr<FECBlock> > m_fec_blocks;
  // The output queue of blocks
  std::queue<std::shared_ptr<FECBlock> > m_out_blocks;
  // The running total of the decoder status
  FECDecoderStats m_stats;
  // Used for tracking dropped packets/sequences
  std::vector<uint8_t> m_block_numbers;

  void decode();
};

#endif //FEC_ENCODER_HH
