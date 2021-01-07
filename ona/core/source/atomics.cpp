#include "ona/core/module.hpp"

#include <stdatomic.h>

namespace Ona::Core {
	AtomicU32::AtomicU32(AtomicU32 const & that) {
		this->Store(that.Load());
	}

	uint32_t AtomicU32::FetchAdd(uint32_t amount) {
		return atomic_fetch_add(&this->value, amount);
	}

	uint32_t AtomicU32::FetchSub(uint32_t amount) {
		return atomic_fetch_sub(&this->value, amount);
	}

	uint32_t AtomicU32::Load() const {
		return atomic_load(&this->value);
	}

	void AtomicU32::Store(uint32_t value) {
		atomic_store(&this->value, value);
	}
}
