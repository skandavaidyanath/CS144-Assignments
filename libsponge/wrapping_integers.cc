#include "wrapping_integers.hh"
#include <iostream>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
	    uint64_t round = 1ul << 32;
	    uint32_t ans = ((n%round) + isn.raw_value()) % round;   // n%round so that it stays in the limit. doesn't affect the answer
	    return WrappingInt32{ans};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
	uint64_t round = 1ul << 32;
	uint32_t limit = 1 << 31;
	
	WrappingInt32 wcp = wrap(checkpoint, isn);
	
	if (wcp.raw_value() == n.raw_value()) {
		return checkpoint;
	}
	
	if (wcp.raw_value() < n.raw_value()) {
		uint32_t diff = n.raw_value() - wcp.raw_value();
		if (n.raw_value() - wcp.raw_value() < limit) {
			return checkpoint + diff;  
		}
		else {
			if (checkpoint < round) {
				return checkpoint + diff;
			}
			else {
				return checkpoint - round + diff;
			}
		}	
	}
	
	else {
		uint32_t diff = wcp.raw_value() - n.raw_value();
		if (wcp.raw_value() - n.raw_value() < limit) {
			return (checkpoint < diff) ? checkpoint - diff + round : checkpoint - diff;   // first case is for wrapping around the isn
		}
		else {
			return checkpoint + round - diff;
		}
	}
	
}
