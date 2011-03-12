#include <iostream>
#include "../Doboz/Encoder.h"
#include "../Doboz/Decoder.h"
#include "Timer.h"
#include <time.h>
#include "Rng32.h"

extern "C" {
#include "qlz/quicklz.h"
}

using namespace Afra;
using namespace doboz;
using namespace std;

int main()
{
	//SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// Read input file
	//FILE* f = fopen("..\\..\\..\\Data\\funnyblock", "rb"); // 2982
	FILE* f = fopen("..\\..\\..\\Data\\gdb.exe", "rb"); // 2982
	//FILE* f = fopen("..\\..\\..\\Data\\sponza.obj", "rb"); // 2982
	//FILE* f = fopen("test\\blockdump", "rb");
	//FILE* f = fopen("test\\cine", "rb"); // 27081, 25716K!!!
	//FILE* f = fopen("..\\..\\..\\Data\\cine", "rb"); // 2982
	//FILE* f = fopen("test\\bunny.v3c1", "rb");
	//FILE* f = fopen("test\\mpi.wrl", "rb");
	//FILE* f = fopen("test\\NotTheMusic.mp4", "rb");
	//FILE* f = fopen("test\\book1", "rb");
	//FILE* f = fopen("test\\enwik8", "rb");
	//FILE* f = fopen("test\\calgary.tar", "rb");
	//FILE* f = fopen("test\\acrord32.exe", "rb");
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* inputBuffer = new char[size];
	fread(inputBuffer, size, 1, f);
	fclose(f);

	/*
	inputBuffer[0] = 1;
	inputBuffer[1] = 2;
	for (int i = 2; i < 65536; ++i) {
		inputBuffer[i] = 0;
	}
	*/

	//size = 47;


	// Encode with Doboz
	cout << "Encoding with Doboz..." << endl;

	Encoder encoder;

	size_t dbzBufferSize = encoder.getMaxCompressedSize(size);
	char* dbzBuffer = new char[dbzBufferSize];

	Timer timer;

	size_t dbzSize;
	if (encoder.encode(inputBuffer, size, dbzBuffer, dbzBufferSize, dbzSize) != RESULT_OK)
		cout << "ERROR" << endl;

	cout << timer.query() << " sec" << endl;
	cout << dbzSize / 1024 << " KB" << endl;
	cout << (float)dbzSize / size * 100 << " %" << endl;
	cout << endl;

	//return 0;

	//!CORRUPT
	//dbzBufferSize = dbzSize - 1;
	/*
	Rng32 rng(time(0));
	for (int i = 0; i < 4; ++i)
	{
		//int pos = rng.gen_uint() % dbzSize;
		int pos = 5;
		//dbzBuffer[pos] = rng.gen_uint();
		dbzBuffer[pos] = 0;
	}
	*/
	//!

	// Decode with Doboz
	cout << "Decoding with Doboz..." << endl;

	Decoder decoder;
	CompressionInfo cinfo;
	if (decoder.getCompressionInfo(dbzBuffer, dbzBufferSize, cinfo) != RESULT_OK)
	{
		cout << "ERROR" << endl;
		return 1;
	}

	cout << "Version: " << cinfo.version << ", Uncompressed: " << cinfo.uncompressedSize << ", Compressed: " << cinfo.compressedSize << endl;

	char* decodedBuffer = new char[size];

	double totalSpeed = 0;
	double maxSpeed = 0.0f;
	int decodeCount = 50;
	//int decodeCount = 1;
	for (int i = 0; i < decodeCount; ++i)
	{
		timer.reset();
		if (decoder.decode(dbzBuffer, dbzBufferSize, decodedBuffer, size) != RESULT_OK)
		{
			cout << "ERROR" << endl;
			return 1;
		}
		double speed = (double)size / (1024*1024) / timer.query();
		if (speed > maxSpeed)
			maxSpeed = speed;
		totalSpeed += speed;
	}

	double averageDbzSpeed = totalSpeed / decodeCount;
	double maxDbzSpeed = maxSpeed;
	cout << maxSpeed << " MB/sec (max)" << endl;


	bool ok = true;
	for (int i = 0; i < size; ++i)
	{
		if (inputBuffer[i] != decodedBuffer[i])
		{
			//cout << i << endl;
			ok = false;
			//break;
		}
	}

	if (ok)
		cout << "DECODING OK" << endl;
	else
		cout << "DECODING ERROR!" << endl;

	f = fopen("..\\..\\..\\Data\\output.dbz", "wb");
	fwrite(dbzBuffer, dbzSize, 1, f);
	fclose(f);

	//return 0;

	f = fopen("..\\..\\..\\Data\\decoded", "wb");
	fwrite(decodedBuffer, size, 1, f);
	fclose(f);

	
	// Encode with QuickLZ
	cout << endl << endl;
	cout << "Encoding with QuickLZ..." << endl;

	char* qlzCompressScratch = new char[QLZ_SCRATCH_COMPRESS];
	char* qlzBuffer = new char[size * 2];

	int qlzSize = qlz_compress(inputBuffer, qlzBuffer, size, qlzCompressScratch);

	cout << qlzSize / 1024 << " KB" << endl;
	cout << endl;

	// Decode with QuickLZ
	cout << "Decoding with QuickLZ..." << endl;

	char* qlzDecompressScratch = new char[QLZ_SCRATCH_DECOMPRESS];

	totalSpeed = 0;
	maxSpeed = 0.0f;
	for (int i = 0; i < decodeCount; ++i)
	{
		timer.reset();
		qlz_decompress(qlzBuffer, decodedBuffer, qlzDecompressScratch);
		double speed = (double)size / (1024*1024) / timer.query();
		if (speed > maxSpeed)
			maxSpeed = speed;
		totalSpeed += speed;
	}

	double averageQlzSpeed = totalSpeed / decodeCount;
	double maxQlzSpeed = maxSpeed;
	cout << maxSpeed << " MB/sec (max)" << endl;

	//cout << endl << "Doboz/QuickLZ: " << averageDbzSpeed / averageQlzSpeed << "x" << endl;
	cout << endl << "Doboz/QuickLZ: " << maxDbzSpeed / maxQlzSpeed << "x" << endl;

}