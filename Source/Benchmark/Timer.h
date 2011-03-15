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