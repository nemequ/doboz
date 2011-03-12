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
	const uint8_t* buffer_; // pointer to the beginning of the buffer inside which we look for matches
	const uint8_t* bufferBase_; // bufferBase_ > buffer_, relative positions are necessary to support > 2 GB buffers
	size_t bufferLength_;
	size_t matchableBufferLength_;
	size_t absolutePosition_; // position from the beginning of buffer_

	// Cyclic dictionary
	int* hashTable_; // relative match positions to bufferBase_
	int* children_; // children of the binary tree nodes (relative match positions to bufferBase_)

	void initialize();

	int computeRelativePosition();
	uint32_t hash(const uint8_t* data);
};

} // namespace detail
} // namespace doboz