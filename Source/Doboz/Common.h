/*
 * Doboz Data Compression Library
 * Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <cassert>

#if defined(_MSC_VER)
#define DOBOZ_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define DOBOZ_FORCEINLINE inline __attribute__ ((always_inline))
#else
#define DOBOZ_FORCEINLINE inline
#endif

namespace doboz {

const int VERSION = 0; // encoding format

enum Result
{
	RESULT_OK,
	RESULT_ERROR_BUFFER_TOO_SMALL,
	RESULT_ERROR_CORRUPTED_DATA,
	RESULT_ERROR_UNSUPPORTED_VERSION,
};


namespace detail {

struct Match
{
	int length;
	int offset;
};

struct Header
{
	uint64_t uncompressedSize;
	uint64_t compressedSize;
	int version;
	bool isStored;
};


const int WORD_SIZE = 4; // uint32_t

const int MIN_MATCH_LENGTH = 3;
const int MAX_MATCH_LENGTH = 255 + MIN_MATCH_LENGTH;
const int MAX_MATCH_CANDIDATE_COUNT = 128;
const int DICTIONARY_SIZE = 1 << 21; // 2 MB, must be a power of 2!

const int TAIL_LENGTH = 2 * WORD_SIZE; // prevents fast write operations from writing beyond the end of the buffer during decoding
const int TRAILING_DUMMY_SIZE = WORD_SIZE; // safety trailing bytes which decrease the number of necessary buffer checks


// Reads up to 4 bytes and returns them in a word
// WARNING: May read more bytes than requested!
DOBOZ_FORCEINLINE uint32_t fastRead(const void* source, size_t size)
{
	assert(size <= WORD_SIZE);

	switch (size)
	{
	case 4:
		return *reinterpret_cast<const uint32_t*>(source);

	case 3:
		return *reinterpret_cast<const uint32_t*>(source);

	case 2:
		return *reinterpret_cast<const uint16_t*>(source);

	case 1:
		return *reinterpret_cast<const uint8_t*>(source);

	default:
		return 0; // dummy
	}
}

// Writes up to 4 bytes specified in a word
// WARNING: May write more bytes than requested!
DOBOZ_FORCEINLINE void fastWrite(void* destination, uint32_t word, size_t size)
{
	assert(size <= WORD_SIZE);

	switch (size)
	{
	case 4:
		*reinterpret_cast<uint32_t*>(destination) = word;
		break;

	case 3:
		*reinterpret_cast<uint32_t*>(destination) = word;
		break;

	case 2:
		*reinterpret_cast<uint16_t*>(destination) = static_cast<uint16_t>(word);
		break;

	case 1:
		*reinterpret_cast<uint8_t*>(destination) = static_cast<uint8_t>(word);
		break;
	}
}

} // namespace detail

} // namespace doboz