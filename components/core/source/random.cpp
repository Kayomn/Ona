#include "components/core/exports.hpp"

namespace Ona::Core {
	uint32_t XorShifter::NextU32() {
		return static_cast<uint32_t>(this->NextU64());
	}

	uint64_t XorShifter::NextU64() {
		this->seed ^= (this->seed << 13);
		this->seed ^= (this->seed << 17);
		this->seed ^= (this->seed << 5);

		return this->seed;
	}
}
