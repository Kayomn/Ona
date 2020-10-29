#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "ona/core/module.hpp"

namespace Ona::Collections {
	using namespace Ona::Core;

	template<typename ValueType> class Collection {
		virtual void ForValues(Callable<void(ValueType &)> const & action) = 0;

		virtual void ForValues(Callable<void(ValueType const &)> const & action) const = 0;
	};
}

#include "ona/collections/header/array.hpp"
#include "ona/collections/header/table.hpp"
#include "ona/collections/header/slots.hpp"

#endif
