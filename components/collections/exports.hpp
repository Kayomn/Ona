#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "components/core/exports.hpp"

namespace Ona::Collections {
	using namespace Ona::Core;

	template<typename ValueType> class Collection : public Object {
		public:
		virtual void Clear() = 0;

		virtual size_t Count() const = 0;

		virtual void ForValues(Callable<void(ValueType &)> const & action) = 0;

		virtual void ForValues(Callable<void(ValueType const &)> const & action) const = 0;
	};
}

#include "components/collections/header/table.hpp"
#include "components/collections/header/stack.hpp"
#include "components/collections/header/queue.hpp"

#endif
