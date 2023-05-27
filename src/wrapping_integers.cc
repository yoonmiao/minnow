#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point.operator+(static_cast<uint32_t>(n));
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  //timeout 超时 这个真的太慢太慢了
  /*uint64_t min_absno = 0;
  if(raw_value_ >= zero_point.raw_value_){
    min_absno = static_cast<uint64_t>(raw_value_) - static_cast<uint64_t>(zero_point.raw_value_);
  }else{
    min_absno = (1ULL << 32) -  static_cast<uint64_t>(zero_point.raw_value_) + static_cast<uint64_t>(raw_value_);
  }

  if(checkpoint <= min_absno) return min_absno;

  while(checkpoint > min_absno){
    min_absno += (1ULL << 32);
  }
  if((min_absno - checkpoint) < (checkpoint - (min_absno - (1ULL << 32)))) return min_absno;
  return min_absno - (1ULL << 32);
  */
  constexpr uint64_t UINT32_RANGE = 1ULL << 32;

  auto chpt32 = wrap(checkpoint, zero_point);

  uint32_t offset = chpt32.raw_value_ - this -> raw_value_;

  //we have absolute number checkpoint to wrap to chpt32.raw_value_
  //there are two choices from chpt32 to reach raw_value_, left(minus) or right(plus)

  if(offset > (1UL << 31)) {//right is closer
    return checkpoint + (UINT32_RANGE - offset);
  }

  if(checkpoint < offset){ //left is closer
    return (checkpoint + UINT32_RANGE) - offset;
  }
  return checkpoint - offset;
}
