#pragma once

#include "Common.h"
#include "Dictionary.h"

namespace Doboz {

class Encoder
{
public:
	Result encode(const void* source, size_t sourceSize, void* destination, size_t destinationSize, size_t& compressedSize);
	size_t getMaxCompressedSize(size_t size);

private:
	Detail::Dictionary dictionary_;

	Result encodeStored(const void* source, size_t sourceSize, void* destination, size_t& compressedSize);
	Detail::Match getBestMatch(Detail::Match* matchCandidates, int matchCandidateCount);
	unsigned int encodeMatch(const Detail::Match& match, void* destination);
	unsigned int getMatchCodedSize(const Detail::Match& match);
	unsigned int getSizeCodedSize(size_t size);
	unsigned int getHeaderSize(size_t maxCompressedSize);
	void encodeHeader(const Detail::Header& header, size_t maxCompressedSize, void* destination);
};

} // namespace Doboz