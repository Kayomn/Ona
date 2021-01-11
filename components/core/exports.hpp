#ifndef ONA_COMPONENT_CORE_H
#define ONA_COMPONENT_CORE_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include <initializer_list>
#include <new>

#define $packed __attribute__ ((packed))

#define $length(arr) (sizeof(arr) / sizeof(*arr))

#define $atomic _Atomic

#include "components/core/header/math.hpp"

namespace Ona::Core {
	/**
	 * A non-owning view, as defined by an address and length, into a particular region of memory.
	 *
	 * `Slice` can be considered a "fat pointer" of sorts.
	 */
	template<typename Type> struct Slice {
		size_t length;

		Type * pointer;

		template<typename CastType> using Casted = std::conditional_t<
			std::is_const<Type>::value,
			CastType const,
			CastType
		>;

		/**
		 * Reinterprets the `Slice` as a `Slice` of raw, unsigned bytes.
		 */
		constexpr Slice<Casted<uint8_t>> Bytes() const {
			return Slice<Casted<uint8_t>>{
				.length = (this->length * sizeof(Type)),
				.pointer = reinterpret_cast<Casted<uint8_t> *>(this->pointer)
			};
		}

		/**
		 * Provides mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type & At(size_t index) {
			assert((index < this->length));

			return this->pointer[index];
		}

		/**
		 * Provides non-mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type const & At(size_t index) const {
			assert((index < this->length));

			return this->pointer[index];
		}

		/**
		 * Checks for element-wise equality between the the `Slice` and `that`.
		 */
		constexpr bool Equals(Slice const & that) const {
			if (this->length != that.length) return false;

			for (size_t i = 0; i < this->length; i += 1) {
				if (this->pointer[i] != that.pointer[i]) return false;
			}

			return true;
		}

		/**
		 * Creates a new `Slice` of `const` `Type` from the `Slice`, granting access to elements
		 * from index `a` to position `b`.
		 */
		constexpr Slice<Type> Sliced(size_t a, size_t b) const {
			return Slice<Type>{
				.length = b,
				.pointer = (this->pointer + a)
			};
		}

		constexpr Type * begin() {
			return this->pointer;
		}

		constexpr Type const * begin() const {
			return this->pointer;
		}

		constexpr Type * end() {
			return (this->pointer + this->length);
		}

		constexpr Type const * end() const {
			return (this->pointer + this->length);
		}

		/**
		 * Provides casting to a version of the `Slice` with a `const` `Type`.
		 */
		constexpr operator Slice<Type const>() const {
			return (*reinterpret_cast<Slice<Type const> const *>(this));
		}
	};

	/**
	 * Creates a `Slice` from `pointer` that provides a view into the memory for `length` elements.
	 */
	template<typename Type> constexpr Slice<Type> SliceOf(Type * pointer, size_t const length) {
		return Slice<Type>{length, pointer};
	}

	using Chars = Slice<char const>;

	constexpr Chars CharsFrom(char const * pointer) {
		size_t length = 0;

		while (*(pointer + length)) length += 1;

		return SliceOf(pointer, length);
	}

	/**
	 * Checks if the error type is equal to `0` (no error).
	 */
	template<typename ErrorType> constexpr bool IsOk(ErrorType errorType) {
		return (errorType == static_cast<ErrorType>(0));
	}
}

#include "components/core/header/atomics.hpp"
#include "components/core/header/text.hpp"

namespace Ona::Core {
	class Object {
		public:
		Object() = default;

		Object(Object const &) = delete;

		virtual ~Object() {};

		virtual bool Equals(Object const * that) const {
			return (this == that);
		}

		virtual uint64_t ToHash() const {
			return reinterpret_cast<int64_t>(this);
		}

		virtual String ToString() const {
			return String{"{}"};
		}
	};
}

#include "components/core/header/memory.hpp"

namespace Ona::Core {
	template<typename> struct Callable;

	/**
	 * Type-erasing function-like wrapper for binding function pointers and functor structs alike.
	 */
	template<typename Return, typename... Args> struct Callable<Return(Args...)> {
		private:
		/**
		 * Dynamic dispatch table for `Callable`s.
		 *
		 * This table is necessary for the kind of type erasure that `Callable` uses, as things like
		 * copy-construction and proper destruction cannot happen without it.
		 */
		struct Operations {
			Return(* caller)(uint8_t const * userdata, Args... args);

			void(* copier)(uint8_t * destinationUserdata, uint8_t const * sourceUserdata);

			void(* destroyer)(uint8_t * userdata);
		};

		enum { BufferSize = 24 };

		Operations const * operations;

		uint8_t buffer[BufferSize];

		public:
		using Function = Return(*)(Args...);

		Callable() = default;

		Callable(Callable const & that) {
			if (that.HasValue()) {
				this->operations = that.operations;

				that.operations->copier(this->buffer, that.buffer);
			} else {
				this->operations = nullptr;

				ZeroMemory(Slice<uint8_t>{
					.length = BufferSize,
					.pointer = this->buffer,
				});
			}
		}

		~Callable() {
			if (this->operations) this->operations->destroyer(this->buffer);
		}

		/**
		 * Constructs a `Callable` from the function pointer `function`.
		 */
		Callable(Function function) {
			static const Operations pointerOperations = {
				.caller = [](uint8_t const * userdata, Args... args) -> Return {
					return (*reinterpret_cast<Function const *>(userdata))(args...);
				},

				.copier = [](uint8_t * destinationUserdata, uint8_t const * sourceUserdata) {
					new (destinationUserdata) Function{
						*reinterpret_cast<Function const *>(sourceUserdata)
					};
				},

				.destroyer = [](uint8_t * userdata) {
					// Does nothing because function pointers are just numbers.
				},
			};

			this->operations = &pointerOperations;

			new (this->buffer) Function{function};
		}

		/**
		 * Constructs a `Callable` from the functor struct `functor`.
		 */
		template<typename Type> Callable(Type const & functor) {
			static const Operations functorOperations = {
				.caller = [](uint8_t const * userdata, Args... args) -> Return {
					return (*reinterpret_cast<Type const *>(userdata))(args...);
				},

				.copier = [](uint8_t * destinationUserdata, uint8_t const * sourceUserdata) {
					new (destinationUserdata) Type{
						*reinterpret_cast<Type const *>(sourceUserdata)
					};
				},

				.destroyer = [](uint8_t * userdata) {
					reinterpret_cast<Type *>(userdata)->~Type();
				},
			};

			static_assert((sizeof(Type) <= BufferSize), "Functor cannot be larger than buffer");

			this->operations = &functorOperations;

			new (this->buffer) Type{functor};
		}

		/**
		 * Calls the `Callable` with `args` as the arguments.
		 *
		 * Calling `Callable::Invoke` on an empty `Callable` *will* result in a runtime error.
		 */
		Return Invoke(Args const &... args) const {
			assert(this->HasValue() && "Callable is empty");

			return this->operations->caller(this->buffer, args...);
		}

		/**
		 * Checks if the `Callable` contains a value.
		 */
		bool HasValue() const {
			return (this->operations != nullptr);
		}
	};
}

#include "components/core/header/array.hpp"
#include "components/core/header/image.hpp"
#include "components/core/header/random.hpp"
#include "components/core/header/os.hpp"
#include "components/core/header/path.hpp"

#endif
