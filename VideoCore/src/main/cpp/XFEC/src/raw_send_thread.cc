
#include <memory>

#include <wfb_bridge.hh>
#include <raw_send_thread.hh>

void raw_send_thread(SharedQueue<std::shared_ptr<Message> > &outqueue,
		     RawSendSocket raw_send_sock, uint16_t max_queue_size,
		     TransferStats &trans_stats, bool &terminate) {

  // Send message out of the send queue
  while(!terminate) {

    // Pull the next packet off the queue
    std::shared_ptr<Message> msg = outqueue.pop();
    bool flush = (msg->msg.size() == 0);

    // FEC encode the packet if requested.
    double loop_start = cur_time();
    auto enc = msg->enc;
    // Flush the encoder if necessary.
    if (flush) {
      if (!enc) {
	continue;
      }
      enc->flush();
    } else {
      double enc_start = cur_time();
      // Get a FEC encoder block
      std::shared_ptr<FECBlock> block = enc->get_next_block(msg->msg.size());
      // Copy the data into the block
      std::copy(msg->msg.data(), msg->msg.data() + msg->msg.size(), block->data());
      // Pass it off to the FEC encoder.
      enc->add_block(block);
      trans_stats.add_encode_time(cur_time() - enc_start);
    }

    // Transmit any packets that are finished in the encoder.
    size_t queue_size = outqueue.size() + enc->n_output_blocks();
    uint16_t dropped_blocks = 0;
    size_t count = 0;
    size_t nblocks = 0;
    for (std::shared_ptr<FECBlock> block = enc->get_block(); block;
	 block = enc->get_block()) {
      double send_start = cur_time();
      // If the link is slower than the data rate we need to drop some packets.
      if (block->is_fec_block() &
	  ((outqueue.size() + enc->n_output_blocks()) > max_queue_size)) {
	++dropped_blocks;
	continue;
      }
      raw_send_sock.send(block->pkt_data(), block->pkt_length(), msg->port,
			 msg->opts.link_type, msg->opts.data_rate);
      count += block->pkt_length();
      ++nblocks;
      trans_stats.add_send_time(cur_time() - send_start);
    }

    // Add stats to the accumulator.
    double cur = cur_time();
    double loop_time = cur - loop_start;
    trans_stats.add_send_stats(count, nblocks, dropped_blocks, queue_size, flush, loop_time);
  }
}

void raw_relay_thread(std::shared_ptr<SharedQueue<std::shared_ptr<monitor_message_t> > > relay_q,
                      RawSendSocket raw_send_sock, uint16_t max_queue_size,
                      bool &terminate) {

  // Send message out of the send queue
  while(!terminate) {

    // Pull the next packet off the queue
    std::shared_ptr<monitor_message_t> msg = relay_q->pop();
    if (msg->data.size() > 0) {
      raw_send_sock.send(msg->data.data(), msg->data.size(), msg->port,
                         msg->link_type, msg->rate);
    }
  }
}
