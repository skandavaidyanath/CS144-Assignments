#include "stream_reassembler.hh"

#include <map>
#include <vector>
#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
	size_t idx = max(_first_unasb, index);    // the first non-duplicate index
	size_t endidx = index + data.length();
	    
	if (eof) {
		_last_idx = index + data.length();
	}
	    
	if (endidx <= idx) {
		// No data; check if we're done, if not, nothing to do
		if (_first_unasb == _last_idx) {
		    _output.end_input();
		}
		return;
	}
	
	vector<pair<pair<size_t, size_t>, string>> to_insert;
    	for (auto p = _buffer.begin(); p != _buffer.end(); p++) {
        	if (idx < p->first.first) {
        	    size_t start = idx, end = min(p->first.first, endidx);
        	    size_t len = end - start;
        	    to_insert.push_back({{start,end}, data.substr(idx - index, len)});
        	}
        	
        	idx = max(idx, p->first.second);
        	
        	if (p->first.second >= endidx) {
        	    break;
        	}
    	}
    	
    	if (idx < endidx) {
        	to_insert.push_back({{idx, endidx}, data.substr(idx - index, endidx - idx)});
    	}
    	
    	for (auto p : to_insert) {
        	_buffer[p.first] = p.second;
        	_num_unasb += p.second.length();
    	}
    	
    	for (auto p = _buffer.begin(); p != _buffer.end() && p->first.first == _first_unasb; _buffer.erase(p++)) {
    	    size_t write_size = _output.write(p->second);
    	    if (write_size == 0) {
    	    	break;
    	    }
    	    _first_unasb += write_size;
    	    _num_unasb -= write_size;
    	}
    	
    	if (_first_unasb == _last_idx) {
    	    _output.end_input();
    	}
}

size_t StreamReassembler::unassembled_bytes() const { return _num_unasb; }

bool StreamReassembler::empty() const { return _buffer.empty(); }
