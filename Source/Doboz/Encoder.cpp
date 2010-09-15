#include <cstring>
#include <algorithm>
#include "Encoder.h"

using namespace std;

namespace Doboz {

using namespace Detail;

Result Encoder::encode(const void* source, size_t sourceSize, void* destination, size_t destinationSize, size_t& compressedSize)
{
	if (sourceSize == 0)
		return RESULT_ERROR_BUFFER_TOO_SMALL;

	size_t maxCompressedSize = getMaxCompressedSize(sourceSize);

	if (destinationSize < maxCompressedSize)
		return RESULT_ERROR_BUFFER_TOO_SMALL;

	const unsigned char* inputBuffer = static_cast<const unsigned char*>(source);
	unsigned char* outputBuffer = static_cast<unsigned char*>(destination);

	unsigned char* outputEnd = outputBuffer + destinationSize;

	// Compute the maximum output end pointer
	// We use this to determine whether we should store the data instead of compressing it
	unsigned char* maxOutputEnd = outputBuffer + maxCompressedSize;

	// Allocate the header
	unsigned char* outputIterator = outputBuffer;
	outputIterator += getHeaderSize(maxCompressedSize);

	// Initialize the dictionary
	dictionary_.setBuffer(inputBuffer, sourceSize);

	// Initialize the control word which contains the literal/match bits
	// The highest bit of a control word is a guard bit, which marks the end of the bit list
	// The guard bit simplifies and speeds up the decoding process, and it 
	const int controlWordBitCount = WORD_SIZE * 8 - 1;
	const unsigned int controlWordGuardBit = 1 << controlWordBitCount;
	unsigned int controlWord = controlWordGuardBit;
	int controlWordBit = 0;

	// Since we do not know the contents of the control words in advance, we allocate space for them and subsequently fill them with data as soon as we can
	// This is necessary because the decoder must encounter a control word *before* the literals and matches it refers to
	// We begin the compressed data with a control word
	unsigned char* controlWordPointer = outputIterator;
	outputIterator += WORD_SIZE;

	// The match located at the current inputIterator position
	Match match;

	// The match located at the next inputIterator position
	// Initialize it to 'no match', because we are at the beginning of the inputIterator buffer
	// A match with a length of 0 means that there is no match
	Match nextMatch;
	nextMatch.length = 0;

	// The dictionary matching look-ahead is 1 character, so set the dictionary position to 1
	// We don't have to worry about getting matches beyond the inputIterator, because the dictionary ignores such requests
	dictionary_.skip();

	// At each position, we select the best match to encode from a list of match candidates provided by the match finder
	Match matchCandidates[MAX_MATCH_CANDIDATE_COUNT];
	int matchCandidateCount;

	// Iterate while there is still data left
	while (dictionary_.position() - 1 < sourceSize)
	{
		// Check whether the output is too large
		// During each iteration, we may output up to 8 bytes (2 words), and the compressed stream ends with 4 dummy bytes
		if (outputIterator + 2 * WORD_SIZE + DUMMY_SIZE > maxOutputEnd)
		{
			// Stop the compression and instead store
			return encodeStored(source, sourceSize, destination, compressedSize);
		}

		// Check whether the control word must be flushed
		if (controlWordBit == controlWordBitCount)
		{
			// Flush current control word
			fastWrite(controlWordPointer, controlWord, WORD_SIZE);

			// New control word
			controlWord = controlWordGuardBit;
			controlWordBit = 0;

			controlWordPointer = outputIterator;
			outputIterator += WORD_SIZE;
		}

		// The current match is the previous 'next' match
		match = nextMatch;

		// Find the best match at the next position
		// The dictionary position is automatically incremented
		matchCandidateCount = dictionary_.findMatches(matchCandidates);
		nextMatch = getBestMatch(matchCandidates, matchCandidateCount);

		// If we have a match, do not immediately use it, because we may miss an even better match (lazy evaluation)
		// If encoding a literal and the next match has a higher compression ratio than encoding the current match, discard the current match
		if (match.length > 0 && (1 + nextMatch.length) * getMatchCodedSize(match) > match.length * (1 + getMatchCodedSize(nextMatch)))
			match.length = 0;
		
		// Check whether we must encode a literal or a match
		if (match.length == 0)
		{
			// Encode a literal (0 control word flag)
			// In order to efficiently decode literals in runs, the literal bit (0) must differ from the guard bit (1)

			// The current dictionary position is now two characters ahead of the literal to encode
			assert(outputIterator + 1 <= outputEnd);
			fastWrite(outputIterator, inputBuffer[dictionary_.position() - 2], 1);
			++outputIterator;
		}
		else
		{
			// Encode a match (1 control word flag)
			controlWord |= 1 << controlWordBit;

			assert(outputIterator + WORD_SIZE <= outputEnd);
			outputIterator += encodeMatch(match, outputIterator);
			
			// Skip the matched characters
			for (unsigned int i = 0; i < match.length - 2; ++i)
				dictionary_.skip();

			matchCandidateCount = dictionary_.findMatches(matchCandidates);
			nextMatch = getBestMatch(matchCandidates, matchCandidateCount);
		}

		// Next control word bit
		++controlWordBit;
	}

	// Flush the control word
	fastWrite(controlWordPointer, controlWord, WORD_SIZE);

	// Output trailing safety dummy bytes
	// This reduces the number of necessary buffer checks during decoding
	assert(outputIterator + DUMMY_SIZE <= outputEnd);
	fastWrite(outputIterator, 0, DUMMY_SIZE);
	outputIterator += DUMMY_SIZE;

	// Done, compute the compressed size
	compressedSize = outputIterator - outputBuffer;

	// Encode the header
	Header header;
	header.version = VERSION;
	header.isStored = false;
	header.uncompressedSize = sourceSize;
	header.compressedSize = compressedSize;

	encodeHeader(header, maxCompressedSize, outputBuffer);

	// Return the compressed size
	return RESULT_SUCCESS;
}

// Store the source
Result Encoder::encodeStored(const void* source, size_t sourceSize, void* destination, size_t& compressedSize)
{
	unsigned char* outputBuffer = static_cast<unsigned char*>(destination);
	unsigned char* outputIterator = outputBuffer;

	// Encode the header
	size_t maxCompressedSize = getMaxCompressedSize(sourceSize);
	unsigned int headerSize = getHeaderSize(maxCompressedSize);

	compressedSize = headerSize + sourceSize;

	Header header;

	header.version = VERSION;
	header.isStored = true;
	header.uncompressedSize = sourceSize;
	header.compressedSize = compressedSize;

	encodeHeader(header, maxCompressedSize, destination);
	outputIterator += headerSize;

	// Store the data
	memcpy(outputIterator, source, sourceSize);

	return RESULT_SUCCESS;
}

// Selects the best match from a list of match candidates provided by the match finder
Match Encoder::getBestMatch(Match* matchCandidates, int matchCandidateCount)
{
	Match bestMatch;
	bestMatch.length = 0;

	// Select the longest match which can be coded efficiently (coded size is less than the length)
	for (int i = matchCandidateCount - 1; i >= 0; --i)
	{
		if (matchCandidates[i].length > getMatchCodedSize(matchCandidates[i]))
		{
			bestMatch = matchCandidates[i];
			break;
		}
	}
	
	return bestMatch;
}

unsigned int Encoder::encodeMatch(const Match& match, void* destination)
{
	assert(match.length <= MAX_MATCH_LENGTH);
	assert(match.length == 0 || match.offset < DICTIONARY_SIZE);

	unsigned int word;
	unsigned int size;

	unsigned int lengthCode = match.length - MIN_MATCH_LENGTH;
	unsigned int offsetCode = match.offset;

	if (lengthCode == 0 && offsetCode < 64)
	{
		word = offsetCode << 2; // 000
		size = 1;
	}
	else if (lengthCode == 0 && offsetCode < 16384)
	{
		word = (offsetCode << 2) | 1; // 001
		size = 2;
	}
	else if (lengthCode < 16 && offsetCode < 1024)
	{
		word = (offsetCode << 6) | (lengthCode << 2) | 2; // 010
		size = 2;
	}
	else if (lengthCode < 32 && offsetCode < 65536)
	{
		word = (offsetCode << 8) | (lengthCode << 3) | 3; // 011
		size = 3;
	}
	else
	{
		word = (offsetCode << 11) | (lengthCode << 3) | 7; // 111
		size = 4;
	}

	if (destination != 0)
		fastWrite(destination, word, size);

	return size;
}

unsigned int Encoder::getMatchCodedSize(const Match& match)
{
	return encodeMatch(match, 0);
}

unsigned int Encoder::getSizeCodedSize(size_t size)
{
	if (size <= UCHAR_MAX)
		return 1;

	if (size <= USHRT_MAX)
		return 2;

	if (size <= UINT_MAX)
		return 4;
	
	return 8;
}

unsigned int Encoder::getHeaderSize(size_t maxCompressedSize)
{
	return 1 + 2 * getSizeCodedSize(maxCompressedSize);
}

void Encoder::encodeHeader(const Header& header, size_t maxCompressedSize, void* destination)
{
	assert(header.version < 8);

	unsigned char* outputIterator = static_cast<unsigned char*>(destination);

	// Encode the attribute byte
	unsigned int attributes = header.version;

	unsigned int sizeCodedSize = getSizeCodedSize(maxCompressedSize);
	attributes |= (sizeCodedSize - 1) << 3;

	if (header.isStored)
		attributes |= 128;

	*outputIterator++ = attributes;

	// Encode the uncompressed and compressed sizes
	switch (sizeCodedSize)
	{
	case 4:
		*reinterpret_cast<unsigned int*>(outputIterator)                 = static_cast<unsigned int>(header.uncompressedSize);
		*reinterpret_cast<unsigned int*>(outputIterator + sizeCodedSize) = static_cast<unsigned int>(header.compressedSize);
		break;

	case 2:
		*reinterpret_cast<unsigned short*>(outputIterator)                 = static_cast<unsigned short>(header.uncompressedSize);
		*reinterpret_cast<unsigned short*>(outputIterator + sizeCodedSize) = static_cast<unsigned short>(header.compressedSize);
		break;

	case 1:
		*reinterpret_cast<unsigned char*>(outputIterator)                 = static_cast<unsigned char>(header.uncompressedSize);
		*reinterpret_cast<unsigned char*>(outputIterator + sizeCodedSize) = static_cast<unsigned char>(header.compressedSize);
		break;

	case 8:
		*reinterpret_cast<unsigned long long*>(outputIterator)                 = header.uncompressedSize;;
		*reinterpret_cast<unsigned long long*>(outputIterator + sizeCodedSize) = header.compressedSize;
		break;
	}
}

size_t Encoder::getMaxCompressedSize(size_t size)
{
	// The header + the original uncompressed data
	return getHeaderSize(size) + size;
}

} // namespace Doboz