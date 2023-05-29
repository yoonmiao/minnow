#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms ),
  retransmission_timeout_(initial_RTO_ms_)
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{

  return outstanding_bytes_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  
  return consecutive_retransmissions_num_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  return {};
}

void TCPSender::push( Reader& outbound_stream )
{
  if(fin_flag_) return;

  
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  return {};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
}
