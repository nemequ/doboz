#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <ctime>
#endif

#include <stdexcept>

namespace Afra {

class Timer
{
public:
	Timer()
	{
#ifdef _WIN32
		LARGE_INTEGER frequency;

		if (QueryPerformanceFrequency(&frequency) == 0)
			throw std::runtime_error("timer is not supported");
		
		countsPerSecond_ = (double)frequency.QuadPart;
#endif

		reset();
	}

	void reset()
	{
#ifdef _WIN32
		if (QueryPerformanceCounter(&startCount_) == 0)
			throw std::runtime_error("could not query counter");
		
#else
		if (clock_gettime(CLOCK_MONOTONIC, &startTime_) != 0) {
			throw std::runtime_error("could not get time");
		}
#endif
	}

	double query() const
	{
#ifdef _WIN32
		LARGE_INTEGER currentCount;

		if (QueryPerformanceCounter(&currentCount) == 0)
			throw std::runtime_error("could not query counter");
		
		return (currentCount.QuadPart - startCount_.QuadPart) / countsPerSecond_;
#else
		timespec currentTime;

		if (clock_gettime(CLOCK_MONOTONIC, &currentTime) != 0)
			throw std::runtime_error("could not get time");
		
		return (double)(currentTime.tv_sec - startTime_.tv_sec) + (currentTime.tv_nsec - startTime_.tv_nsec) * 1e-9;
#endif
	}

private:
#ifdef _WIN32
	double        countsPerSecond_;
	LARGE_INTEGER startCount_;
#else
	timespec      startTime_;
#endif
};

} // namespace Afra
