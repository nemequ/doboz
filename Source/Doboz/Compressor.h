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
#include "Dictionary.h"

namespace doboz {

class Compressor
{
public:
	// Returns the maximum compressed size of any block of data with the specified size
	// This function should be used to determine the size of the compression destination buffer
	static uint64_t getMaxCompressedSize(uint64_t size);

	// Compresses a block of data
	// The source and destination buffers must not overlap and their size must be greater than 0
	// This operation is memory safe
	// On success, returns RESULT_OK and outputs the compressed size
	Result compress(const void* source, size_t sourceSize, void* destination, size_t destinationSize, size_t& compressedSize);

private:
	detail::Dictionary dictionary_;

	static int getSizeCodedSize(uint64_t size);
	static int getHeaderSize(uint64_t maxCompressedSize);

	Result store(const void* source, size_t sourceSize, void* destination, size_t& compressedSize);
	detail::Match getBestMatch(detail::Match* matchCandidates, int matchCandidateCount);
	int encodeMatch(const detail::Match& match, void* destination);
	int getMatchCodedSize(const detail::Match& match);
	void encodeHeader(const detail::Header& header, uint64_t maxCompressedSize, void* destination);
};

} // namespace doboz