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
	uint32_t decodeMatch(detail::Match& match, const void* source);
	Result decodeHeader(detail::Header& header, const void* source, size_t sourceSize, int& headerSize);
};

} // namespace doboz