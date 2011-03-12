#include <cstring>
#include "Decoder.h"

namespace doboz {

using namespace detail;

Result Decoder::decode(const void* source, size_t sourceSize, void* destination, size_t destinationSize)
{
	const uint8_t* inputBuffer = static_cast<const uint8_t*>(source);
	const uint8_t* inputIterator = inputBuffer;

	uint8_t* outputBuffer = static_cast<uint8_t*>(destination);
	uint8_t* outputIterator = outputBuffer;

	// Decode the header
	Header header;
	int headerSize;
	Result decodeHeaderResult = decodeHeader(header, source, sourceSize, headerSize);

	if (decodeHeaderResult != RESULT_OK)
	{
		return decodeHeaderResult;
	}

	inputIterator += headerSize;

	if (header.version != VERSION)
	{
		return RESULT_ERROR_UNSUPPORTED_VERSION;
	}

	// Check whether the supplied buffers are large enough
	if (sourceSize < header.compressedSize || destinationSize < header.uncompressedSize)
	{
		return RESULT_ERROR_BUFFER_TOO_SMALL;
	}

	size_t uncompressedSize = static_cast<size_t>(header.uncompressedSize);

	// If the data is simply stored, copy it to the destination buffer and we're done
	if (header.isStored)
	{
		memcpy(outputBuffer, inputIterator, uncompressedSize);
		return RESULT_OK;
	}

	const uint8_t* inputEnd = inputBuffer + uncompressedSize;
	uint8_t* outputEnd = outputBuffer + uncompressedSize;

	// Compute pointer to the first byte of the output 'tail'
	// Fast write operations can be used only before the tail, because those may write beyond the end of the output buffer
	uint8_t* outputTail = (uncompressedSize > TAIL_LENGTH) ? (outputEnd - TAIL_LENGTH) : outputBuffer;

	// Initialize the control word to 'empty'
	uint32_t controlWord = 1;

	// Decoding loop
	for (; ;)
	{
		// Check whether there is enough data left in the input buffer
		// In order to decode the next literal/match, we have to read up to 8 bytes (2 words)
		if (inputIterator + 2 * WORD_SIZE > inputEnd)
		{
			return RESULT_ERROR_CORRUPTED_DATA;
		}

		// Check whether we must read a control word
		if (controlWord == 1)
		{
			assert(inputIterator + WORD_SIZE <= inputEnd);
			controlWord = fastRead(inputIterator, WORD_SIZE);
			inputIterator += WORD_SIZE;
		}

		// Detect whether it's a literal or a match
		if ((controlWord & 1) == 0)
		{
			// It's a literal

			// If we are before the tail, we can safely use fast writing operations
			if (outputIterator < outputTail)
			{
				// We copy literals in runs of up to 4, because it's faster than copying one by one

				// Copy implicitly 4 literals regardless of the run length
				assert(inputIterator + 4 <= inputEnd);
				assert(outputIterator + 4 <= outputEnd);
				fastWrite(outputIterator, fastRead(inputIterator, 4), 4);

				// Get the run length using a lookup table
				static const int8_t literalRunLengthTable[16] = {4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
				int runLength = literalRunLengthTable[controlWord & 0xf];

				// Advance the inputBuffer and outputBuffer pointers with the run length
				inputIterator  += runLength;
				outputIterator += runLength;

				// Consume as much control word bits as the run length
				controlWord >>= runLength;
			}
			else
			{
				// We have reached the tail, we cannot output literals in runs anymore
				// Output all remaining literals
				while (outputIterator < outputEnd)
				{
					// Check whether there is enough data left in the input buffer
					// In order to decode the next literal, we have to read up to 5 bytes
					if (inputIterator + WORD_SIZE + 1 > inputEnd)
					{
						return RESULT_ERROR_CORRUPTED_DATA;
					}

					// Check whether we must read a control word
					if (controlWord == 1)
					{
						assert(inputIterator + WORD_SIZE <= inputEnd);
						controlWord = fastRead(inputIterator, WORD_SIZE);
						inputIterator += WORD_SIZE;
					}

					// Output one literal
					// We cannot use fast read/write functions
					assert(inputIterator + 1 <= inputEnd);
					assert(outputIterator + 1 <= outputEnd);
					*outputIterator++ = *inputIterator++;

					// Next control word bit
					controlWord >>= 1;
				}

				// Done
				return RESULT_OK;
			}
		}
		else
		{
			// It's a match

			// Decode the match
			Match match;
			assert(inputIterator + WORD_SIZE <= inputEnd);
			inputIterator += decodeMatch(match, inputIterator);

			// Copy the matched string
			// In order to achieve high performance, we copy characters in groups of machine words
			// We have to correctly handle overlaps, so after copying a word, we can advance the pointers only by the minimum possible match offset
			// If the minimum match offset is equal to the word size, the copying has maximum efficiency
			uint8_t* matchString = outputIterator - match.offset;

			// Check whether the match is out of range
			if (matchString < outputBuffer || outputIterator + match.length > outputTail)
			{
				return RESULT_ERROR_CORRUPTED_DATA;
			}
			
			int i = 0;

			if (match.offset < WORD_SIZE)
			{
				// The match offset is less than the word size
				// In order to correctly handle the overlap, we have to copy the first three bytes one by one
				do
				{
					assert(matchString >= outputBuffer);
					assert(matchString + i + WORD_SIZE <= outputEnd);
					assert(outputIterator + i + WORD_SIZE <= outputEnd);

					fastWrite(outputIterator + i, fastRead(matchString + i, 1), 1);
					++i;
				}
				while (i < 3);

				// With this trick, we increase the distance between the source and destination pointers
				matchString -= 2 + (match.offset & 1);
			}

			assert(outputIterator - matchString >= WORD_SIZE);

			do
			{
				assert(matchString >= outputBuffer);
				assert(matchString + i + WORD_SIZE <= outputEnd);
				assert(outputIterator + i + WORD_SIZE <= outputEnd);

				fastWrite(outputIterator + i, fastRead(matchString + i, WORD_SIZE), WORD_SIZE);
				i += WORD_SIZE;
			}
			while (i < match.length);
			
			outputIterator += match.length;

			// Next control word bit
			controlWord >>= 1;
		}
	}	
}

Result Decoder::getCompressionInfo(const void* source, size_t sourceSize, CompressionInfo& compressionInfo)
{
	// Decode the header
	Header header;
	int headerSize;
	Result decodeHeaderResult = decodeHeader(header, source, sourceSize, headerSize);

	if (decodeHeaderResult != RESULT_OK)
	{
		return decodeHeaderResult;
	}

	// Return the requested info
	compressionInfo.uncompressedSize = header.uncompressedSize;
	compressionInfo.compressedSize = header.compressedSize;
	compressionInfo.version = header.version;

	return RESULT_OK;
}

// Decodes a match and returns its size in bytes
DOBOZ_FORCEINLINE int Decoder::decodeMatch(Match& match, const void* source)
{
	// Use a decoding lookup table in order to avoid expensive branches
	static const struct
	{
		uint32_t mask; // the mask for the entire encoded match
		uint8_t offsetShift;
		uint8_t lengthMask;
		uint8_t lengthShift;
		int8_t size; // the size of the encoded match in bytes
	}
	lut[] =
	{
		{0xff,        2,   0, 0, 1}, // 000
		{0xffff,      2,   0, 0, 2}, // 001
		{0xffff,      6,  15, 2, 2}, // 010
		{0xffffff,    8,  31, 3, 3}, // 011
		{0xff,        2,   0, 0, 1}, // 100 = 000
		{0xffff,      2,   0, 0, 2}, // 101 = 001
		{0xffff,      6,  15, 2, 2}, // 110 = 011
		{0xffffffff, 11, 255, 3, 4}, // 111
	};

	// Read the maximum number of bytes a match is coded in (the machine word size)
	uint32_t word = fastRead(source, WORD_SIZE);

	// Compute the decoding lookup table entry index: the lowest 3 bits of the encoded match
	uint32_t i = word & 7;

	// Compute the match offset and length using the lookup table entry
	match.offset = static_cast<int>((word & lut[i].mask) >> lut[i].offsetShift);
	match.length = static_cast<int>(((word >> lut[i].lengthShift) & lut[i].lengthMask) + MIN_MATCH_LENGTH);

	return lut[i].size;
}

// Decodes a header and returns its size in bytes
// If the header is not valid, the function returns 0
Result Decoder::decodeHeader(Header& header, const void* source, size_t sourceSize, int& headerSize)
{
	const uint8_t* inputIterator = static_cast<const uint8_t*>(source);

	// Decode the attribute bytes
	if (sourceSize < 1)
	{
		return RESULT_ERROR_BUFFER_TOO_SMALL;
	}

	uint32_t attributes = *inputIterator++;

	header.version = attributes & 7;
	int sizeCodedSize = ((attributes >> 3) & 7) + 1;

	// Compute the size of the header
	headerSize = 1 + 2 * sizeCodedSize;

	if (sourceSize < headerSize)
	{
		return RESULT_ERROR_BUFFER_TOO_SMALL;
	}

	header.isStored = (attributes & 128) != 0;

	// Decode the uncompressed and compressed sizes
	switch (sizeCodedSize)
	{
	case 1:
		header.uncompressedSize = *reinterpret_cast<const uint8_t*>(inputIterator);
		header.compressedSize = *reinterpret_cast<const uint8_t*>(inputIterator + sizeCodedSize);
		break;

	case 2:
		header.uncompressedSize = *reinterpret_cast<const uint16_t*>(inputIterator);
		header.compressedSize = *reinterpret_cast<const uint16_t*>(inputIterator + sizeCodedSize);
		break;

	case 4:
		header.uncompressedSize = *reinterpret_cast<const uint32_t*>(inputIterator);
		header.compressedSize = *reinterpret_cast<const uint32_t*>(inputIterator + sizeCodedSize);
		break;

	case 8:
		header.uncompressedSize = *reinterpret_cast<const uint64_t*>(inputIterator);
		header.compressedSize = *reinterpret_cast<const uint64_t*>(inputIterator + sizeCodedSize);
		break;

	default:
		return RESULT_ERROR_CORRUPTED_DATA;
	}

	return RESULT_OK;
}

}