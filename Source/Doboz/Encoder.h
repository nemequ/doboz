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

class Encoder
{
public:
	static size_t getMaxCompressedSize(size_t size);

	Result encode(const void* source, size_t sourceSize, void* destination, size_t destinationSize, size_t& compressedSize);

private:
	detail::Dictionary dictionary_;

	static int getSizeCodedSize(size_t size);
	static int getHeaderSize(size_t maxCompressedSize);

	Result encodeStored(const void* source, size_t sourceSize, void* destination, size_t& compressedSize);
	detail::Match getBestMatch(detail::Match* matchCandidates, int matchCandidateCount);
	int encodeMatch(const detail::Match& match, void* destination);
	int getMatchCodedSize(const detail::Match& match);
	void encodeHeader(const detail::Header& header, size_t maxCompressedSize, void* destination);
};

} // namespace doboz