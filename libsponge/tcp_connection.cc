#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity();; }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_now - _time_last_rec; }

void TCPConnection::segment_received(const TCPSegment &seg) {
	if (!_active) 
	{
        	return;
    	}
    	
    	// WE HAVEN'T RECEIVED A SYN YET! LISTENING
    	if (_sender.bytes_in_flight() == 0 && !_sender.stream_in().eof() && !_receiver.ackno().has_value() && !seg.header().syn) 
    	{
        	return;
    	}
    	
    	// RST CONDITION
    	if (seg.header().rst) 
    	{
        	_sender.stream_in().set_error();
        	_receiver.stream_out().set_error();
        	_active = false;
        	return;
    	}
    	
    	// FIN RECEIVED. WE NO LONGER NEED TO LINGER
    	if (seg.header().fin) 
    	{
        	_linger_after_streams_finish = false;
    	}
    	
    	// PASSIVE CLOSE
    	if (!_linger_after_streams_finish && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0) 
    	{
        	if (seg.header().ack && seg.header().ackno == _sender.next_seqno()) 
        	{
        		// WE HAVE NOTHING TO SEND AND WE HAVE ALREADY RECEIVED THE FIN FROM PEER
            		_active = false;
        	}
    	}
    
    	_time_last_rec = _time_now;
    	_receiver.segment_received(seg);
    	
    	// ACK IS SET, GIVE SENDER THE INFO IT NEEDS
    	if (seg.header().ack) 
    	{
        	_sender.ack_received(seg.header().ackno, seg.header().win);
    	}
    	
    	bool has_seqnos = seg.length_in_sequence_space() > 0;
    	
    	if (fill_window() == 0 && has_seqnos) 
    	{
    		// IF WE HAVE NOTHING TO SEND, WE AT LEAST SEND AN EMPTY SEGMENT
        	_sender.send_empty_segment();
        	fill_window();
    	}
    	
    	// CHECK IF WE'RE STILL ACTIVE
    	if (!_linger_after_streams_finish && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0) 
    	{
        	_active = false;
    	}
}

bool TCPConnection::active() const {
	return _active; 
}

size_t TCPConnection::fill_window() {
    	_sender.fill_window();
    	size_t num_packets = 0;
    	while (!_sender.segments_out().empty()) {
        	TCPSegment seg = _sender.segments_out().front();
        	if (_receiver.ackno().has_value()) 
        	{
        	    seg.header().ack = true;
        	    seg.header().ackno = _receiver.ackno().value();
        	}
        	
        	if (_cfg.recv_capacity > UINT16_MAX) 
        	{
            		seg.header().win = UINT16_MAX;
        	} 
        	else 
        	{
            		seg.header().win = _cfg.recv_capacity;
        	}
        
        	_segments_out.push(seg);
        	_sender.segments_out().pop();
        	_time_last_sent = _time_now;
        
        	num_packets++;
        
        	if (seg.header().fin) 
        	{
        	    _finsent = true;
        	}
    	}
    	return num_packets;
}

void TCPConnection::send_RST() {
	_active = false;
	_sender.send_empty_segment();
	TCPSegment seg = _sender.segments_out().front();
	_sender.segments_out().pop();
	seg.header().rst = true;
	_segments_out.push(seg);
	_sender.stream_in().set_error();
	_receiver.stream_out().set_error();
}

size_t TCPConnection::write(const string &data) {
    	if (!_active) {
        	return 0;
    	}
    	size_t write_size = _sender.stream_in().write(data);
    	fill_window();
    	return write_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
	if (!_active) {
        	return;
    	}
    	
    	_time_now += ms_since_last_tick;
    	_sender.tick(ms_since_last_tick);
    	
    	if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        	send_RST();
        	return;
    	}

    	if (_sender.bytes_in_flight() == 0 && _sender.stream_in().eof()) {
        	if (_time_now - _time_last_sent >= 10 * _cfg.rt_timeout) 
        	{
            		_active = false;
        	}
    	}
}

void TCPConnection::end_input_stream() { 
	if (!_active) {
        	return;
    	}
    	
    	_sender.stream_in().end_input();
	fill_window();
}

void TCPConnection::connect() { fill_window(); /* sends a SYN*/}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
