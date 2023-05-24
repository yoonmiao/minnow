#include <stdexcept>
#include <iostream>
#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {
  buf_.resize(capacity_); //vector实现循环队列
}

void Writer::push( string data )
{
  //push的数据长度不能超过stream当前剩余的容量
  size_t len = min(data.size(), available_capacity());

  for(size_t i = 0; i < len; i++){
    buf_[nwrite_++ % capacity_] = data[i];
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
  //当前剩余容量为总capacity减去已使用容量(Number of bytes currently buffered)
  return capacity_ - (nwrite_ - nread_);
}

uint64_t Writer::bytes_pushed() const
{
  return nwrite_;
}

string_view Reader::peek() const
{//如果当前stream中仍有字节留存，那么就可以peek，从nread % capacity_开始向后循环
  if(bytes_buffered() == 0){
    return string_view{};
  }

  size_t wpos = nwrite_ % capacity_;
  size_t rpos = nread_ % capacity_;
  int diff = (int)wpos - (int)rpos; //if wpos < rpos, size_t will be a quite large number
  if(diff > 0){
    return string_view(&buf_[rpos], diff);
  }
  //循环队列，如果此时在buf_中的仍未读取的内容从数组末尾循环到了头部，那么我们只截取末尾部分。
  return string_view(&buf_[rpos], capacity_ - rpos);
}

bool Reader::is_finished() const
{
  return closed_ && bytes_buffered() == 0;
}

bool Reader::has_error() const
{
  return error_;
}

void Reader::pop( uint64_t len )
{
    //最多pop出当前stream中留存的字节数
    size_t n = min(len, bytes_buffered());
    nread_ += n;
}

uint64_t Reader::bytes_buffered() const
{
  //当前stream中留存的字节数, 写进来的字节数减去读走的字节数
  return nwrite_ - nread_;
}

uint64_t Reader::bytes_popped() const
{
  
  return nread_;
}
