#include "common.hpp"

namespace Ona {
	float Floor(float const value) {
		return __builtin_floorf(value);
	}

	float Pow(float const value, float const exponent) {
		return __builtin_pow(value, exponent);
	}
}
