#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
	size_t data_size = data.length();
	size_t write_size = min(data_size, _capacity - _buffer_size);
	if (!write_size) 
	{
		return 0;
	}
	_stream += data.substr(0, write_size);
    	_buffer_size += write_size;
    	_bytes_written += write_size;
    	return write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    	return _stream.substr(0, len);
    
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
	_stream.erase(0, len);
    	_buffer_size -= len;
    	_bytes_read += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
	string out = _stream.substr(0, len);
	_stream.erase(0, len);
	_buffer_size -= len;
	_bytes_read += len;
	return out;
}

void ByteStream::end_input() {_eof=true;}

bool ByteStream::input_ended() const { return _eof; }

size_t ByteStream::buffer_size() const { return _buffer_size; }

bool ByteStream::buffer_empty() const { return _buffer_size==0; }

bool ByteStream::eof() const { return _buffer_size==0 && _eof; }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buffer_size; }
