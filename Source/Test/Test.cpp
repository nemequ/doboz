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

#include <cstdio>
#include <iostream>
#include <algorithm>
#include "Doboz/Encoder.h"
#include "Doboz/Decoder.h"
#include "Utils/Timer.h"
#include "Utils/FastRng.h"

using namespace std;
using namespace afra;

const double MEGABYTE = 1024.0 * 1024.0;

char* originalBuffer = 0;
size_t originalSize;

char* compressedBuffer = 0;
char* tempCompressedBuffer = 0;
size_t compressedSize;
size_t compressedBufferSize;

char* decompressedBuffer = 0;


bool loadFile(char* filename)
{
	FILE* file = fopen(filename, "rb");

	if (file == 0)
	{
		cout << "ERROR: Could not open file \"" << filename << "\"" << endl;
		return 1;
	}

	fseek(file, 0, SEEK_END);
	originalSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	cout << "Loading file \"" << filename << "\"..." << endl;
	cout << "File size: " << static_cast<double>(originalSize) / MEGABYTE << " MB" << endl;

	originalBuffer = new char[originalSize];

	fread(originalBuffer, 1, originalSize, file);
	fclose(file);
	return true;
}

void initializeTests()
{
	compressedBufferSize = doboz::Encoder::getMaxCompressedSize(originalSize);
	compressedBuffer = new char[compressedBufferSize];
	tempCompressedBuffer = new char[compressedBufferSize];

	decompressedBuffer = new char[originalSize];
}

void cleanup()
{
	delete[] compressedBuffer;
	delete[] tempCompressedBuffer;
	delete[] decompressedBuffer;

	delete[] originalBuffer;
}

bool verifyDecompressed()
{	
	for (size_t i = 0; i < originalSize; ++i)
	{
		if (decompressedBuffer[i] != originalBuffer[i])
		{
			return false;
		}
	}

	return true;
}

void prepareDecompression()
{
	memcpy(tempCompressedBuffer, compressedBuffer, compressedSize);
	memset(decompressedBuffer, 0, originalSize);
}

// Decompresses from tempCompressedBuffer!
bool decode()
{
	doboz::Decoder decoder;

	doboz::Result result = decoder.decode(tempCompressedBuffer, compressedSize, decompressedBuffer, originalSize);
	if (result != doboz::RESULT_OK)
		return false;

	return verifyDecompressed();
}

bool basicTest()
{
	doboz::Encoder encoder;
	doboz::Result result;

	cout << "Basic test" << endl;
	cout << "Encoding..." << endl;
	memset(compressedBuffer, 0, compressedBufferSize);
	result = encoder.encode(originalBuffer, originalSize, compressedBuffer, compressedBufferSize, compressedSize);
	if (result != doboz::RESULT_OK)
	{
		cout << "Encoding FAILED" << endl;
		return false;
	}

	cout << "Decoding..." << endl;
	prepareDecompression();
	if (!decode())
	{
		cout << "Decoding/verification FAILED" << endl;
		return false;
	}

	cout << "Decoding and verification successful" << endl;
	return true;
}

bool corruptionTest()
{
	FastRng rng;

	cout << "Corruption test" << endl;

	int maxErrorCount = 100;

	int testCount = 1000;
	int failedDecodingCount = 0;
	for (int i = 0; i < testCount; ++i)
	{
		cout << "\r" << (i + 1) << "/" << testCount;
		prepareDecompression();

		// Corrupt compressed data
		int errorCount = (rng.getUint() % maxErrorCount) + 1;
		for (int j = 0; j < errorCount; ++j)
		{
			int errorPosition = rng.getUint() % static_cast<uint32_t>(originalSize);
			tempCompressedBuffer[errorPosition] = rng.getUint() % 256;
		}

		// Try to decode
		doboz::Decoder decoder;
		doboz::Result result = decoder.decode(tempCompressedBuffer, compressedSize, decompressedBuffer, originalSize);
		if (result != doboz::RESULT_OK)
		{
			++failedDecodingCount;
		}
	}

	cout << endl;
	cout << "Decoding errors: " << failedDecodingCount << "/" << testCount << endl;
	return true;
}

bool incrementalTest()
{
	doboz::Encoder encoder;
	doboz::Result result;

	cout << "Incremental input size test" << endl;
	size_t totalOriginalSize = originalSize;

	size_t testCount = min(static_cast<size_t>(512), originalSize);
	for (size_t i = 1; i <= testCount; ++i)
	{
		cout << "\r" << i << "/" << testCount;

		originalSize = i;
		memset(compressedBuffer, 0, compressedBufferSize);
		result = encoder.encode(originalBuffer, originalSize, compressedBuffer, compressedBufferSize, compressedSize);
		if (result != doboz::RESULT_OK)
		{
			cout << endl << "Encoding FAILED" << endl;
			originalSize = totalOriginalSize;
			return false;
		}

		prepareDecompression();
		if (!decode())
		{
			cout << endl << "Decoding/verification FAILED" << endl;
			originalSize = totalOriginalSize;
			return false;
		}
	}

	cout << endl;
	originalSize = totalOriginalSize;
	return true;
}

int main(int argc, char* argv[])
{
	cout << "Doboz Data Compression Library - TEST" << endl;
	cout << "Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>" << endl;
	cout << endl;

	if (argc != 2)
	{
		cout << "Usage: Benchmark file" << endl;
		return 0;
	}

	bool ok = loadFile(argv[1]);
	if (!ok)
	{
		return 1;
	}

	cout << endl;

	// Basic test
	initializeTests();
	cout << "1. ";
	ok = basicTest();
	if (!ok)
	{
		cleanup();
		return 1;
	}
	cout << endl;
	bool allOk = true;

	// Corruption test
	cout << "2. ";
	allOk = allOk && corruptionTest();
	cout << endl;

	// Incremental input size test
	cout << "3. ";
	allOk = allOk && incrementalTest();

	cleanup();
	return 0;
}