#pragma once

namespace Afra {

class Rng32 {
public:
	Rng32(unsigned int seed = 1) : value_(seed) {}

	unsigned int gen_uint() {
		next();
		return value_;
	}

	int gen_int() {
		next();
		return value_;
	}

	float gen_float() {
		const unsigned int sign_exponent = 0x3f800000;
		const unsigned int mask = 0x007fffff;
		const float offset = 1.0f;

		next();

		union {
			unsigned int ui;
			float        f;
		} result;

		result.ui = (value_ & mask) | sign_exponent;
		return result.f - offset;
	}

private:
	unsigned int value_;

	void next() {
		const unsigned int multiplier = 1664525;
		const unsigned int increment = 1013904223;
		value_ = multiplier * value_ + increment;
	}
};

}