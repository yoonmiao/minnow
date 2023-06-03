#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  route_table_.push_back( { route_prefix, prefix_length, next_hop, interface_num } );
}

void Router::route()
{
  for (auto& interface : interfaces_ ) {
    std::optional<InternetDatagram> t;
    while ( ( t = interface.maybe_receive() ).has_value() ) {
      route_one_datagram( t.value() );
    }
  }
}

void Router::route_one_datagram( InternetDatagram& dgram )
{
  if ( dgram.header.ttl >= 1 ) {
    dgram.header.ttl -= 1;
    dgram.header.compute_checksum();//payload: bad ipv4 datagram
  }

  if ( dgram.header.ttl == 0 ) {
    return;
  }

  auto dst = dgram.header.dst;
  uint8_t cur_max = 0;
  RouteEntry* best_match_route = nullptr;
  for ( auto& route : route_table_ ) {
    // 如果前缀匹配匹配长度为 0 (通用路径)，或者前缀匹配相同
    if ( route.prefix_length == 0 || ( ( route.route_prefix ^ dst ) >> ( 32 - route.prefix_length ) ) == 0 ) {
      if ( route.prefix_length >= cur_max ) {
        cur_max = route.prefix_length;
        best_match_route = &route;
      }
    }
  }

  if (best_match_route ) {
    const optional<Address> next_hop = best_match_route->next_hop;
    AsyncNetworkInterface& interface = interfaces_[best_match_route->interface_num];

    if ( next_hop.has_value() ) {
      interface.send_datagram( dgram, next_hop.value() );
    } else {
      interface.send_datagram( dgram, Address::from_ipv4_numeric( dst ) );
    }
  }
}
