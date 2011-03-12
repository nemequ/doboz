#include <iostream>
#include <Doboz/Encoder.h>
#include <Doboz/Decoder.h>
#include "Timer.h"

using namespace afra;
using namespace std;

int main(int argc, char* argv[])
{
	cout << "Doboz " << doboz::VERSION << ".0" << endl;
	cout << "Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>" << endl;
	cout << endl;

	if (argc != 4 || (argv[1][0] != 'c' && argv[1][0] != 'd' && argv[1][0] != 'D'))
	{
		cout << "Usage: doboz c|d input output" << endl;
		return 0;
	}

	// Read the input file
	FILE* inputStream = fopen(argv[2], "rb");

	if (inputStream == 0)
	{
		cout << "ERROR: Could not open " << argv[2] << "!" << endl;
		return 1;
	}

	fseek(inputStream, 0, SEEK_END);
	unsigned long long inputSize = _ftelli64(inputStream);
	fseek(inputStream, 0, SEEK_SET);

	cout << "Reading input file...";

	char* inputBuffer = new char[inputSize];

	fread(inputBuffer, 1, inputSize, inputStream);
	fclose(inputStream);

	cout << " Done!" << endl;

	// Compress or decompress
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	size_t outputSize;
	char* outputBuffer;
	Timer timer;
	double time;
	bool benchmark = false;

	if (argv[1][0] == 'c')
	{
		cout << "Compressing...";

		doboz::Encoder encoder;
		doboz::Result result;
		size_t outputBufferSize = encoder.getMaxCompressedSize(inputSize);
		outputBuffer = new char[outputBufferSize];

		timer.reset();
		result = encoder.encode(inputBuffer, inputSize, outputBuffer, outputBufferSize, outputSize);
		time = timer.query();

		if (result != doboz::RESULT_OK)
		{
			cout << " ERROR!" << endl;
			delete[] inputBuffer;
			delete[] outputBuffer;
			return 1;
		}

		cout << " Done!" << endl;
		cout << endl;
		cout <<  static_cast<double>(inputSize) / (1024 * 1024) << " MB -> " << static_cast<double>(outputSize) / (1024 * 1024) << " MB" << endl;
		cout << static_cast<double>(outputSize) / inputSize * 100.0 << "% compression ratio" << endl;
		cout << "Speed: " << static_cast<double>(inputSize) / (1024 * 1024) / time << " MB/s" << endl;
	}
	else
	{
		benchmark = argv[1][0] == 'D';

		if (benchmark)
			cout << "Decompressing (benchmark)...";
		else
			cout << "Decompressing...";

		doboz::Decoder decoder;
		doboz::CompressionInfo info;
		if (decoder.getCompressionInfo(inputBuffer, inputSize, info) != doboz::RESULT_OK)
		{
			cout << " ERROR!" << endl;
			delete[] inputBuffer;
			return 1;
		}

		outputSize = info.uncompressedSize;
		outputBuffer = new char[outputSize];

		if (benchmark)
		{
			doboz::Result result;
			
			time = 10000000000.0f;
			for (int i = 0; i < 10; ++i)
			{
				timer.reset();
				result = decoder.decode(inputBuffer, inputSize, outputBuffer, info.uncompressedSize);
				double currentTime = timer.query();
				if (currentTime < time)
					time = currentTime;
			}

			if (result != doboz::RESULT_OK)
			{
				cout << " ERROR!" << endl;
				delete[] inputBuffer;
				delete[] outputBuffer;
				return 1;
			}
		}
		else
		{
			doboz::Result result;

			timer.reset();
			result = decoder.decode(inputBuffer, inputSize, outputBuffer, info.uncompressedSize);
			time = timer.query();

			if (result != doboz::RESULT_OK)
			{
				cout << " ERROR!" << endl;
				delete[] inputBuffer;
				delete[] outputBuffer;
				return 1;
			}
		}

		cout << " Done!" << endl;
		cout << endl;
		cout << "Speed: " << static_cast<double>(outputSize) / (1024 * 1024) / time << " MB/s" << endl;
	}

	cout << "Time : " << time << " s" << endl;
	cout << endl;

	delete[] inputBuffer;

	if (benchmark)
	{
		delete[] outputBuffer;
		return 0;
	}


	// Write the output file
	FILE* outputStream = fopen(argv[3], "wb");

	if (outputStream == 0)
	{
		cout << "ERROR: Could not open " << argv[3] << "!" << endl;
		delete[] outputBuffer;
		return 1;
	}

	cout << "Writing output file...";

	fwrite(outputBuffer, 1, outputSize, outputStream);
	fclose(outputStream);

	cout << " Done!" << endl;

	delete[] outputBuffer;

	return 0;
}