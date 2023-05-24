#include <stdexcept>
#include <iostream>
#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  auto len = data.size();
  if(len > capacity_ - buf_.size()){
    len = capacity_ - buf_.size();
  }
  nwrite_ += len;
  for (size_t i = 0; i < len; i++) {
        buf_.push_back(data[i]);
  }
  return;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  error_ = true;
  return;
}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buf_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return nwrite_;
}

string_view Reader::peek() const
{
  return string_view(&buf_.front(), 1);
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && buf_.empty();
}

bool Reader::has_error() const
{
  // Your code here.
  return error_;
}

void Reader::pop( uint64_t len )
{
    if (len > buf_.size()) {
        len = buf_.size();
    }
    nread_ += len;

    for(size_t i = 0; i < len; i++){
      buf_.pop_front();
    }
    return;
}

uint64_t Reader::bytes_buffered() const
{
  
  return buf_.size();
}

uint64_t Reader::bytes_popped() const
{
  
  return nread_;
}
