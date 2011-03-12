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