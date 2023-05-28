#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if(!syn_flag_){
    if(!message.SYN) return;
    syn_flag_ = true;
    isn_ = message.seqno;
  }

   if (message.FIN) {
        fin_flag_ = true;
    }

  //convert seqno to absolute seqno
  //first_unassembled index + 1 is first_unassembled corresponding absolute number
  uint64_t checkpont = inbound_stream.writer().bytes_pushed() + 1; 
  //given unrap isn_ is zero_point, unwrap seqno to get absolute seqno
  uint64_t abs_seqno = message.seqno.unwrap(isn_, checkpont);

  //convert absolute seqno to first_index
  uint64_t first_index = message.SYN ? 0 : abs_seqno - 1; 
  reassembler.insert(first_index, message.payload.release(), message.FIN, inbound_stream);
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  TCPReceiverMessage msg {};
  uint64_t w =  inbound_stream.writer().available_capacity();
  w = w > UINT16_MAX ? UINT16_MAX : w;
  msg.window_size = static_cast<uint16_t>(w);

  if(!syn_flag_) {
    return msg;
  }

  uint64_t first_index = inbound_stream.writer().bytes_pushed();
  uint64_t abs_ackno = first_index + 1;
  //如果Bytestream关闭了，也就是说接收到了一个FIN,而FIN是会占据seqno的，所以我们需要给checkpoint加一
  if(fin_flag_ && inbound_stream.writer().is_closed()) abs_ackno += 1;
  msg.ackno = Wrap32::wrap(abs_ackno, isn_);
  return msg; 
}
