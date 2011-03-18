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

#include <stdint.h>

namespace afra {

class FastRng
{
public:
	FastRng(uint32_t seed = 1) : value_(seed) {}

	uint32_t getUint()
	{
		next();
		return value_;
	}

	int32_t getInt()
	{
		next();
		return static_cast<int32_t>(value_);
	}

	float getFloat()
	{
		const uint32_t signExponent = 0x3f800000;
		const uint32_t mask = 0x007fffff;
		const float offset = 1.0f;

		next();

		union
		{
			uint32_t ui;
			float f;
		}
		result;

		result.ui = (value_ & mask) | signExponent;
		return result.f - offset;
	}

private:
	uint32_t value_;

	void next()
	{
		const uint32_t multiplier = 1664525;
		const uint32_t increment = 1013904223;
		value_ = multiplier * value_ + increment;
	}
};

} // namespace afra
