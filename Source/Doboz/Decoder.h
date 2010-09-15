#pragma once

#include "Common.h"

namespace Doboz {

struct CompressionInfo
{
	unsigned long long uncompressedSize;
	unsigned long long compressedSize;
	int version;
};

class Decoder
{
public:
	Result decode(const void* source, size_t sourceSize, void* destination, size_t destinationSize);
	Result decodeSafe(const void* source, size_t sourceSize, void* destination, size_t destinationSize);
	Result getCompressionInfo(const void* source, size_t sourceSize, CompressionInfo& compressionInfo);

private:
	unsigned int decodeMatch(Detail::Match& match, const void* source);
	Result decodeHeader(Detail::Header& header, const void* source, size_t sourceSize, unsigned int& headerSize);
};

} // namespace Doboz