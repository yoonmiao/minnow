#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "tcp_config.hh"
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , retransmission_timeout_( initial_RTO_ms_ )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{

  return next_seqno_ - recv_ackno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{

  return consecutive_retransmissions_num_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if ( message_.empty() )
    return {};

  TCPSenderMessage msg = message_.front();
  message_.pop_front();
  return msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  if ( fin_flag_ )
    return;

  // special case
  window_size_ = window_size_ ? window_size_ : 1;

  uint64_t remaining_size = static_cast<uint64_t>( window_size_ ) + recv_ackno_ - next_seqno_;
  // 发送窗口是根据接收端返回的ackno向前移动的，如果recv_ackno不动，发送窗口的起点不能向前移动
  // 计算机网络微课堂讲得很好

  // 只要发送窗口仍有空间，并且FIN还未发送，就仍然可以读stream中数据push进发送窗口
  while ( remaining_size > 0 && !fin_flag_ ) {
    TCPSenderMessage msg;
    // deal with SYN
    if ( !syn_flag_ ) {
      msg.SYN = true;
      syn_flag_ = true;
      remaining_size -= 1;
    } // SYN occupies a sequence number

    // deal with payload
    msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
    Buffer& buffer = msg.payload;
    read( outbound_stream, min( remaining_size, TCPConfig::MAX_PAYLOAD_SIZE ), buffer.operator std::string&() );
    remaining_size -= buffer.size();

    // deal with FIN
    if ( outbound_stream.is_finished() && remaining_size > 0 ) {
      msg.FIN = true;
      fin_flag_ = true;
      remaining_size -= 1;
    }

    // empty segment，没有任何数据，停止数据包的发送
    if ( msg.sequence_length() == 0 )
      return;

    next_seqno_ += msg.sequence_length();
    outstanding_bytes_ += msg.sequence_length();

    // 加入发送队列
    message_.push_back( msg );
    if ( timer_.is_running() == false ) {
      timer_.start();
    }
    outstanding_message_.push_back( msg );
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // 没有ackno
  if ( msg.ackno.has_value() == false )
    return;

  // wrap, unwrap转换得到新的ackno
  auto new_recv_ackno_ = msg.ackno.value().unwrap( isn_, recv_ackno_ );
  window_size_ = msg.window_size;

  //test "Impossible ackno (beyond next seqno) is ignored"
  //invalid recv_ackno, next_seqno - recv_ackno is invalid, can't get 
  //correct bytes in flight
  if (new_recv_ackno_ > next_seqno_ ){
    return;
  }else{
    recv_ackno_ = new_recv_ackno_;
  }
  
  while ( !outstanding_message_.empty() ) {
    auto front_msg = outstanding_message_.front();
    uint64_t front_seqno = front_msg.seqno.unwrap( isn_, recv_ackno_ );
    if ( recv_ackno_ >= ( front_seqno + front_msg.sequence_length() ) ) {
      outstanding_message_.pop_front();
      retransmission_timeout_ = initial_RTO_ms_;
      timer_.start();
      consecutive_retransmissions_num_ = 0;
    } else {
      break;
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  if ( timer_.is_expired( ms_since_last_tick, retransmission_timeout_ ) ) {
    message_.push_back( outstanding_message_.front() );
    if ( window_size_ != 0 ) {
      retransmission_timeout_ <<= 1; // exponential backoff
      consecutive_retransmissions_num_++;
    }

    //maybe_send();面向测试编程
    timer_.start();
  } else {
    return;
  }
}
