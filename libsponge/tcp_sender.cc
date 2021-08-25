#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _current_rto{retx_timeout} {}
    

uint64_t TCPSender::bytes_in_flight() const { 
	uint64_t bytes = 0;
    	for (auto p = _outstanding.begin(); p != _outstanding.end(); p++) {
        	bytes += 0 + p->second.length_in_sequence_space();
    	}
    	return bytes;
}

void TCPSender::fill_window() {
	// FIN_ACKED
	if (_finsent)
	{
		return;
	}
	// CLOSED
	if (_next_seqno == 0) 
	{
        	TCPSegment seg;
        	seg.header().seqno = _isn;
        	seg.header().syn = true;
        	if (_stream.eof()) {
            		seg.header().fin = true;
            		_finsent = true;
        	} 
        	_segments_out.push(seg);
            	_outstanding.push_back({_time, seg});
            	_next_seqno += seg.length_in_sequence_space();
            	return;
        }
        // SYN_ACKED STREAM ONGOING
        while (_last_ack + _window - _next_seqno > 0)
        {
        	TCPSegment seg;
        	seg.header().seqno = wrap(_next_seqno, _isn);
        	
        	size_t len = min(TCPConfig::MAX_PAYLOAD_SIZE, min(_stream.buffer_size(), _last_ack + _window - _next_seqno));
		if (len == 0) {
			// ONLY NEED TO SEND THE FIN
            		if (_stream.eof() && _last_ack + _window - _next_seqno > 0) {
            			seg.header().fin = true;
            			_finsent = true;
            			_segments_out.push(seg);
            			_outstanding.push_back({_time, seg});
            			_next_seqno += seg.length_in_sequence_space();
            		}
            		return;
            	}
        	
        	string data = _stream.read(len);
        	seg.payload() = string(data);
        	if (_stream.eof() && _last_ack + _window - _next_seqno > data.size()) {
            		seg.header().fin = true;
            		_finsent = true;
        	}
        	_segments_out.push(seg);
        	_outstanding.push_back({_time, seg});
        	_next_seqno += seg.length_in_sequence_space();
        	
        	if (_finsent) {
        		return;
        	}
        }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
	size_t ackno64 = unwrap(ackno, _isn, _last_ack);
    	
    	if (ackno64 >= _last_ack) {
    		bool isnew = ackno64 > _last_ack;
    		_last_ack = ackno64;
    		_window = max(1, static_cast<int>(window_size));
    		if (window_size == 0)
    		{
    			_zerowindow = true;
    		}
    		if (isnew) {
            		_current_rto = _initial_retransmission_timeout;
            	}
		for (auto p = _outstanding.begin(); p != _outstanding.end();) 
		{
		    size_t seq64 = unwrap(p->second.header().seqno, _isn, _last_ack);
		    if (seq64 + p->second.length_in_sequence_space() <= _last_ack) 
		    {
		        _outstanding.erase(p);
		    }
		    else if (isnew)
		    {
		    	p->first = _time;   // resetting the sent time i.e. resetting the timeout
		    	p++;
		    }
		    else 
		    {
		    	p++;
		    }
		}
		_consecutive_retransmissions = 0;
	}
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
	_time += ms_since_last_tick;
	if (_outstanding.empty()) {
		return;
	}
    	if (_time >= _outstanding[0].first + _current_rto)
    	{
    		TCPSegment seg = _outstanding[0].second;
    		_segments_out.push(seg);
    		_outstanding[0].first = _time;
    		if (!_zerowindow) 
    		{
    			_current_rto *= 2;
    		}
    		_consecutive_retransmissions++;
    		for (auto p = _outstanding.begin(); p != _outstanding.end(); p++) 
		{
            		p->first = _time;    // resetting the sent time i.e. resetting the timeout
        	}
    	}
    	
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() 
{
	TCPSegment seg;
    	seg.header().seqno = wrap(_next_seqno, _isn);
    	_segments_out.push(seg);
}
