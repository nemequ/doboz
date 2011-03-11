#pragma once

#include "Common.h"

namespace doboz {
namespace detail {

class Dictionary
{
public:
	Dictionary();
	~Dictionary();

	void setBuffer(const uint8_t* buffer, size_t bufferLength);

	int findMatches(Match* matchCandidates);
	void skip();

	size_t position() const
	{
		return absolutePosition_;
	}

private:
	static const int HASH_TABLE_SIZE = 1 << 20;
	static const int CHILD_COUNT = DICTIONARY_SIZE * 2;
	static const int INVALID_POSITION = -1;
	static const int REBASE_THRESHOLD = (INT_MAX - DICTIONARY_SIZE + 1) / DICTIONARY_SIZE * DICTIONARY_SIZE; // must be a multiple of DICTIONARY_SIZE!

	// Buffer
	const uint8_t* buffer_;	// pointer to the beginning of the buffer inside which we look for matches
	const uint8_t* bufferBase_;	// bufferBase_ > buffer_, relative positions are necessary to support > 2 GB buffers
	size_t bufferLength_;
	size_t matchableBufferLength_;
	size_t absolutePosition_; // position from the beginning of buffer_

	// Cyclic dictionary
	int32_t* hashTable_; // relative match positions to bufferBase_
	int32_t* children_;	// children of the binary tree nodes (relative match positions to bufferBase_)

	void init();
	void destroy();

	int computeRelativePosition();
	uint32_t hash(const uint8_t* data);
};

} // namespace detail
} // namespace doboz