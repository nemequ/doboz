#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <ctime>
#endif

#include <cassert>

namespace afra {

class Timer
{
public:
	Timer()
	{
#ifdef _WIN32
		LARGE_INTEGER frequency;

		BOOL result = QueryPerformanceFrequency(&frequency);
		assert(result != 0 && "Timer is not supported.");
		
		invCountsPerSecond_ = 1.0 / static_cast<double>(frequency.QuadPart);
#endif

		reset();
	}

	void reset()
	{
#ifdef _WIN32
		BOOL result = QueryPerformanceCounter(&startCount_);
		assert(result != 0 && "Could not query counter.");
		
#else
		int result = clock_gettime(CLOCK_MONOTONIC, &startTime_);
		assert(result == 0 && "Could not get time.");
#endif
	}

	double query() const
	{
#ifdef _WIN32
		LARGE_INTEGER currentCount;

		BOOL result = QueryPerformanceCounter(&currentCount);
		assert(result != 0 && "Could not query counter.");
		
		return (currentCount.QuadPart - startCount_.QuadPart) * invCountsPerSecond_;
#else
		timespec currentTime;

		int result = clock_gettime(CLOCK_MONOTONIC, &currentTime);
		assert(result == 0 && "Could not get time.");
		
		return static_cast<double>(currentTime.tv_sec - startTime_.tv_sec) + (currentTime.tv_nsec - startTime_.tv_nsec) * 1e-9;
#endif
	}

private:
#ifdef _WIN32
	double        invCountsPerSecond_;
	LARGE_INTEGER startCount_;
#else
	timespec      startTime_;
#endif
};

} // namespace afra