#pragma once

#include "Common.h"

namespace Doboz {
namespace Detail {

class Dictionary
{
public:
	Dictionary();
	~Dictionary();

	void setBuffer(const unsigned char* buffer, size_t bufferLength);

	int findMatches(Match* matchCandidates);
	void skip();

	size_t position() const
	{
		return absolutePosition_;
	}

private:
	static const int HASH_TABLE_SIZE  = 1 << 20;
	static const int CHILD_COUNT      = DICTIONARY_SIZE * 2;
	static const int INVALID_POSITION = -1;
	static const int REBASE_THRESHOLD = (INT_MAX - DICTIONARY_SIZE + 1) / DICTIONARY_SIZE * DICTIONARY_SIZE; // must be a multiple of DICTIONARY_SIZE!

	// Buffer
	const unsigned char* buffer_;					// pointer to the beginning of the buffer inside which we look for matches
	const unsigned char* bufferBase_;				// bufferBase_ > buffer_, relative positions are necessary to support > 2 GB buffers
	size_t		         bufferLength_;
	size_t		         matchableBufferLength_;
	size_t               absolutePosition_;			// position from the beginning of buffer_

	// Cyclic dictionary
	int* hashTable_;		// relative match positions to bufferBase_
	int* children_;			// children of the binary tree nodes (relative match positions to bufferBase_)

	void initialize();
	void cleanup();

	int computeRelativePosition();
	unsigned int hash(const unsigned char* data);
};

} // namespace Detail
} // namespace Doboz