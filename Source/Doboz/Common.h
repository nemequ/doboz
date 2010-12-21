#pragma once

#include <cassert>
#include <climits>

namespace doboz {

const int VERSION = 0; // encoding format

enum Result
{
	RESULT_SUCCESS,
	RESULT_ERROR_BUFFER_TOO_SMALL,
	RESULT_ERROR_CORRUPTED_DATA,
	RESULT_ERROR_UNSUPPORTED_VERSION,
};

} // namespace doboz


namespace doboz {
namespace detail {

struct Match
{
	unsigned int length;
	unsigned int offset;
};

struct Header
{
	unsigned long long uncompressedSize;
	unsigned long long compressedSize;
	int                version;
	bool               isStored;
};

const int WORD_SIZE                 = 4;							// unsigned int (4 bytes)

const int MIN_MATCH_LENGTH          = 3;
const int MAX_MATCH_LENGTH          = 255 + MIN_MATCH_LENGTH;
const int MAX_MATCH_CANDIDATE_COUNT = 128;
const int DICTIONARY_SIZE           = 1 << 21;						// 2 MB, must be a power of 2!

const int TAIL_LENGTH               = 2 * WORD_SIZE;				// prevents fast write operations from writing beyond the end of the buffer during decoding
const int DUMMY_SIZE                = WORD_SIZE;					// safety trailing bytes which decrease the number of necessary buffer checks


// Reads up to 4 bytes and returns them in a word
// WARNING: May read more bytes than requested!
inline unsigned int fastRead(const void* source, size_t size)
{
	assert(size <= WORD_SIZE);

	switch (size)
	{
	case 4:
		return *reinterpret_cast<const unsigned int*>(source);

	case 3:
		return *reinterpret_cast<const unsigned int*>(source);

	case 2:
		return *reinterpret_cast<const unsigned short*>(source);

	case 1:
		return *reinterpret_cast<const unsigned char*>(source);

	default:
		return 0; // dummy
	}
}

// Writes up to 4 bytes specified in a word
// WARNING: May write more bytes than requested!
inline void fastWrite(void* destination, unsigned int word, size_t size)
{
	assert(size <= WORD_SIZE);

	switch (size)
	{
	case 4:
		*reinterpret_cast<unsigned int*>(destination) = word;
		break;

	case 3:
		*reinterpret_cast<unsigned int*>(destination) = word;
		break;

	case 2:
		*reinterpret_cast<unsigned short*>(destination) = static_cast<unsigned short>(word);
		break;

	case 1:
		*reinterpret_cast<unsigned char*>(destination) = static_cast<unsigned char>(word);
		break;
	}
}

} // namespace detail
} // namespace doboz