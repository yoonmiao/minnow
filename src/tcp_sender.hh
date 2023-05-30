#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class Timer
{
private:
  bool is_running_ = false;

public:
  uint64_t ticks_ = 0;

  void start()
  {
    is_running_ = true;
    ticks_ = 0;
  }

  void stop()
  {
    is_running_ = false;
    ticks_ = 0;
  }

  bool is_running() const { return is_running_; }

  bool is_expired( uint64_t ms_since_last_tick, uint64_t retransmission_timeout )
  {
    ticks_ += ms_since_last_tick;
    return is_running_ && ticks_ >= retransmission_timeout;
  }
};

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t retransmission_timeout_; // for exponential backoff

  bool syn_flag_ = false;
  bool fin_flag_ = false;

  uint16_t window_size_ = 1;
  uint64_t outstanding_bytes_ = 0;

  uint64_t consecutive_retransmissions_num_ = 0;

  std::deque<TCPSenderMessage> message_ {};
  std::deque<TCPSenderMessage> outstanding_message_ {};

  // the (absolute) sequence number for the next byte to be sent
  uint64_t next_seqno_ = 0;

  // last ackno
  uint64_t recv_ackno_ = 0;

  // 重传定时器
  Timer timer_ {};

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
