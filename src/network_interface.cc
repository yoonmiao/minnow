#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();
  auto arp_iter = arp_table_.find( next_hop_ip );

  EthernetFrame eth_frame;

  if ( arp_table_.find( next_hop_ip ) != arp_table_.end() ) {
    eth_frame.header = { /* dst  */ arp_iter->second.eth_addr,
                         /* src  */ ethernet_address_,
                         /* type */ EthernetHeader::TYPE_IPv4 };
    eth_frame.payload = serialize( dgram );
    frames_out_.push_back( eth_frame );
  } else {
    // currently no same arp_request
    if ( outstanding_arp_requests_.find( next_hop_ip ) == outstanding_arp_requests_.end() ) {
      ARPMessage arp_request;
      arp_request.opcode = ARPMessage::OPCODE_REQUEST;
      arp_request.sender_ethernet_address = ethernet_address_;
      arp_request.sender_ip_address = ip_address_.ipv4_numeric();
      arp_request.target_ip_address = next_hop_ip;

      eth_frame.header = { /*dst */ ETHERNET_BROADCAST,
                           /*src */ ethernet_address_,
                           /*type */ EthernetHeader::TYPE_ARP };
      eth_frame.payload = serialize( arp_request );
      frames_out_.push_back( eth_frame );

      // add a new KV pair to outstanding_arp_requests_
      outstanding_arp_requests_[next_hop_ip] = initial_arp_request_ttl;
    }
    // we have the same arp request within 5 seconds;
    outstanding_datagrams_.push_back( make_pair( next_hop, dgram ) );
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST )
    return nullopt;

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram datagram;
    if ( parse( datagram, frame.payload ) ) {
      return datagram;
    } else {
      return nullopt;
    }
  }

  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_msg;
    if ( !parse( arp_msg, frame.payload ) ) {
      return nullopt;
    }

    const uint32_t src_ip_addr = arp_msg.sender_ip_address;
    const EthernetAddress src_eth_addr = arp_msg.sender_ethernet_address;
    const uint32_t dst_ip_addr = arp_msg.target_ip_address;
    const EthernetAddress dst_eth_addr = arp_msg.target_ethernet_address;

    arp_table_[src_ip_addr] = { src_eth_addr, initial_ARP_Entry_ttl };

    bool is_valid_arp_request
      = arp_msg.opcode == ARPMessage::OPCODE_REQUEST && dst_ip_addr == ip_address_.ipv4_numeric();

    if ( is_valid_arp_request ) { // we recevied an arp request, need to send a reply
      ARPMessage arp_reply;
      arp_reply.opcode = ARPMessage::OPCODE_REPLY;
      arp_reply.sender_ethernet_address = ethernet_address_;
      arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
      arp_reply.target_ip_address = src_ip_addr;
      arp_reply.target_ethernet_address = src_eth_addr;

      EthernetFrame eth_frame;
      eth_frame.header = { /*dst */ src_eth_addr,
                           /*src */ ethernet_address_,
                           /*type */ EthernetHeader::TYPE_ARP };
      eth_frame.payload = serialize( arp_reply );
      frames_out_.push_back( eth_frame );
      return nullopt;
    }

    bool is_valid_arp_reply = arp_msg.opcode == ARPMessage::OPCODE_REPLY && dst_eth_addr == ethernet_address_;

    if ( is_valid_arp_reply ) {
      outstanding_arp_requests_.erase( src_ip_addr );

      for ( auto it = outstanding_datagrams_.begin(); it != outstanding_datagrams_.end(); ) {
        if ( it->first.ipv4_numeric() == src_ip_addr ) {
          send_datagram( it->second, it->first );
          it = outstanding_datagrams_.erase( it );
        } else
          ++it;
      }
    }
  }

  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // delete outdated arp entry
  for ( auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if ( it->second.ttl > ms_since_last_tick ) {
      it->second.ttl -= ms_since_last_tick;
      ++it;
    } else {
      it = arp_table_.erase( it );
    }
  }

  // delete outdated arp_requests
  for ( auto it = outstanding_arp_requests_.begin(); it != outstanding_arp_requests_.end(); ) {
    if ( it->second > ms_since_last_tick ) {
      it->second -= ms_since_last_tick;
      ++it;
    } else {
      it = outstanding_arp_requests_.erase( it );
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( frames_out_.empty() )
    return nullopt;

  auto frame = frames_out_.front();
  frames_out_.pop_front();

  return frame;
}
