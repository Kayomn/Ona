#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "ona/core/module.hpp"

namespace Ona::Collections {
	using namespace Ona::Core;

	template<typename ValueType> class Collection {
		public:
		virtual void Clear() = 0;

		virtual size_t Count() const = 0;

		virtual void ForValues(Callable<void(ValueType &)> const & action) = 0;

		virtual void ForValues(Callable<void(ValueType const &)> const & action) const = 0;
	};
}

#include "ona/collections/header/table.hpp"
#include "ona/collections/header/stack.hpp"

#endif
