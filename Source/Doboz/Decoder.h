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

struct CompressionInfo
{
	uint64_t uncompressedSize;
	uint64_t compressedSize;
	int version;
};

class Decoder
{
public:
	Result decode(const void* source, size_t sourceSize, void* destination, size_t destinationSize);
	Result getCompressionInfo(const void* source, size_t sourceSize, CompressionInfo& compressionInfo);

private:
	int decodeMatch(detail::Match& match, const void* source);
	Result decodeHeader(detail::Header& header, const void* source, size_t sourceSize, int& headerSize);
};

} // namespace doboz