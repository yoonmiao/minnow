#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  first_unread_ = output.reader().bytes_popped();
  first_unassembled_ = output.writer().bytes_pushed();
  first_unacceptable_ = output.writer().bytes_pushed() + output.writer().available_capacity();

  if(first_unassembled_ == first_unacceptable_) return; //no bytes can be pushed to the stream 

  if(is_last_substring){
    eof_flag_ = true;
    eof_idx_ = first_index + data.size();
  }

  if(eof_flag_ && first_unassembled_ == eof_idx_){
    map_.clear();
    output.writer().close();
    return;
  }

  if(eof_flag_ && first_index >= eof_idx_) return ;

  if(first_index >= first_unacceptable_)  return; //beyond capacity

  if(first_index + data.size() <= first_unassembled_ ) return; //already in Bytestream

  if(first_index + data.size() > first_unacceptable_){//去尾
    data = data.substr(0, first_unacceptable_ - first_index);
  }

  if (first_index < first_unassembled_ && first_index + data.size() == first_unassembled_){
    //extremely special case WHERE first_unaccepable equal to first_unassembled
    //test-based
    return;
  }
  if (first_index < first_unassembled_ && first_index + data.size() > first_unassembled_){//去头
        data = data.substr(first_unassembled_ - first_index);
        first_index = first_unassembled_;
  }

  if(first_index != first_unassembled_){//this data can't push into ByteStream since there are bytes missing
    for(auto i = first_index; i < first_index + data.size(); i++){
      map_[i] = data[i - first_index];
    }
  }

  size_t new_unassembled = first_unassembled_;
  if(first_index == first_unassembled_){
    //directly push into ByteStream
    //no need to store in Reassembler
    if(!eof_flag_){
      output.writer().push(data);
      new_unassembled = first_unassembled_ + data.size();
    }else{
      //consider reaching eof_idx_
      if(first_unassembled_ + data.size() <= eof_idx_){
        output.writer().push(data);
        new_unassembled = first_unassembled_ + data.size();
      }else{
        size_t len = eof_idx_ - first_unassembled_;
        output.writer().push(data.substr(0, len));
        map_.clear();
        output.writer().close();
        return;
      }
    }
  

    for(auto i = first_unassembled_; i < new_unassembled; i++){
      //this part of stream has been pushed into ByteStream
      //no need to keep corresponding char of these index
      if(map_.find(i) != map_.end()){
        map_.erase(i);
      }
    }

  //there could be some chars that become in order right now
  //since stream before new_unassembled are pushed.
    string s;
    size_t limit = eof_flag_ == true ? min(eof_idx_, first_unacceptable_) : first_unacceptable_; 
    for(auto i = new_unassembled; i < limit ; i++){
      if(map_.find(i) != map_.end()){
        s += map_[i];
        map_.erase(i);
      }else{// missing char, there are still some unknown index
        break;
      }
    }
    if(!s.empty()){
      output.writer().push(s);
    }
  
    if(eof_flag_ && new_unassembled + s.size() >= eof_idx_){
      map_.clear();
      output.writer().close();
    }
  }
  return;
}

uint64_t Reassembler::bytes_pending() const
{
  return map_.size();
}
