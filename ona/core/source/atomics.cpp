#include "ona/core/module.hpp"

namespace Ona::Core {
	AtomicU32::AtomicU32(AtomicU32 const & that) {
		this->Store(that.Load());
	}

	uint32_t AtomicU32::FetchAdd(uint32_t amount) {
		return __c11_atomic_fetch_add(&this->value, amount, __ATOMIC_SEQ_CST);
	}

	uint32_t AtomicU32::FetchSub(uint32_t amount) {
		return __c11_atomic_fetch_sub(&this->value, amount, __ATOMIC_SEQ_CST);
	}

	uint32_t AtomicU32::Load() const {
		return __c11_atomic_load(&this->value, __ATOMIC_SEQ_CST);
	}

	void AtomicU32::Store(uint32_t value) {
		__c11_atomic_store(&this->value, value, __ATOMIC_SEQ_CST);
	}
}
