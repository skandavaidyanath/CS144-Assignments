#include "tcp_receiver.hh"
#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    	TCPHeader h = seg.header();
    	if (h.syn) 
    	{
		_isn = h.seqno;
		_status = _SYN_RECV;
    	} 
    	else 
    	{
		_last_asn = unwrap(h.seqno, _isn, _last_asn) - 1;     // -1 for SYN
    	}
    
    	if (_status == _LISTEN) 
    	{
    		return;
    	}
   	
    	bool eof = false;
    	if (h.fin) 
    	{
		eof = true;
		_status = _FIN_RECV;
    	}
    	string payload = static_cast<string>(seg.payload().str());
    	_reassembler.push_substring(payload, _last_asn, eof);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
 	if (_status == _LISTEN) {
 		return {};
 	}
 	
 	size_t unasb = _reassembler.first_unasb();
 	
 	WrappingInt32 ack(0);
 	if (unassembled_bytes() == 0) {
 		ack = WrappingInt32(wrap(unasb+_status, _isn));  //either stream is over and everything is assembled (_FIN_RECV) or just waiting for next packet (_SYN_RECV)
 	}
 	else {
 		ack = WrappingInt32(wrap(unasb+_SYN_RECV, _isn));  //something unassembled left so cannot be _FIN_RECV
 	}
 	return {ack};
 }

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
