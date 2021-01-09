#ifndef ONA_CORE_H
#define ONA_CORE_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include <initializer_list>
#include <new>

#define $packed __attribute__ ((packed))

#define $length(arr) (sizeof(arr) / sizeof(*arr))

#define $atomic _Atomic

#include "ona/core/header/math.hpp"

namespace Ona::Core {
	/**
	 * A non-owning view, as defined by an address and length, into a particular region of memory.
	 *
	 * `Slice` can be considered a "fat pointer" of sorts.
	 */
	template<typename Type> struct Slice {
		size_t length;

		Type * pointer;

		/**
		 * Reinterprets the `Slice` as a `Slice` of raw, unsigned bytes.
		 */
		constexpr Slice<uint8_t> Bytes() {
			return Slice<uint8_t>{
				.length = (this->length * sizeof(Type)),
				.pointer = reinterpret_cast<uint8_t *>(this->pointer)
			};
		}

		/**
		 * Reinterprets the `Slice` as a `Slice` of read-only, raw, unsigned bytes.
		 */
		constexpr Slice<uint8_t const> Bytes() const {
			return Slice<uint8_t const>{
				.length = (this->length * sizeof(Type)),
				.pointer = reinterpret_cast<uint8_t const *>(this->pointer)
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
		 * Creates a new `Slice` from the `Slice`, granting access to elements from index `a` to
		 * position `b`.
		 */
		constexpr Slice Sliced(size_t a, size_t b) {
			return Slice{
				.length = b,
				.pointer = (this->pointer + a)
			};
		}

		/**
		 * Creates a new `Slice` of `const` `Type` from the `Slice`, granting access to elements
		 * from index `a` to position `b`.
		 */
		constexpr Slice<Type const> Sliced(size_t a, size_t b) const {
			return Slice<Type const>{
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

	template<typename> struct Callable;

	template<typename Return, typename... Args> struct Callable<Return(Args...)> {
		private:
		class Context {
			public:
			virtual Return Invoke(Args const &... args) = 0;
		};

		enum { BufferSize = 24 };

		Context * context;

		uint8_t buffer[BufferSize];

		using FunctionType = Return(*)(Args...);

		public:
		Callable() = default;

		Callable(Callable const & that) {
			for (size_t i = 0; i < BufferSize; i += 1) this->buffer[i] = that.buffer[i];

			this->context = reinterpret_cast<Context *>(this->buffer);
		}

		Callable(FunctionType function) {
			class Pointer : public Context {
				private:
				FunctionType function;

				public:
				Pointer(FunctionType function) : function{function} { }

				Return Invoke(Args const &... args) override {
					return this->function(args...);
				};
			};

			this->context = new (this->buffer) Pointer{function};
		}

		template<typename Type> Callable(Type const & functor) {
			class Functor : public Context {
				private:
				Type functor;

				public:
				Functor(Type const & functor) : functor{functor} { }

				Return Invoke(Args const &... args) override {
					return this->functor(args...);
				};
			};

			static_assert((sizeof(Functor) <= BufferSize), "Functor cannot be larger than buffer");

			this->context = new (this->buffer) Functor{functor};
		}

		Return Invoke(Args const &... args) const {
			return this->context->Invoke(args...);
		}

		bool IsEmpty() const {
			return (this->context == nullptr);
		}
	};

	using Chars = Slice<char const>;

	constexpr Chars CharsFrom(char const * pointer) {
		size_t length = 0;

		while (*(pointer + length)) length += 1;

		return SliceOf(pointer, length);
	}

	/**
	 * Error code enum wrapper, with a built-in state for flagging the lack of an error.
	 */
	template<typename ErrorType> struct Error {
		private:
		ErrorType value;

		uint32_t hasError;

		public:
		Error() = default;

		Error(ErrorType error) : value{error}, hasError{true} {

		}

		constexpr ErrorType Value() const {
			assert(this->hasError);

			return this->value;
		}

		constexpr bool IsOk() const {
			return (!this->hasError);
		}
	};
}

#include "ona/core/header/atomics.hpp"
#include "ona/core/header/text.hpp"
#include "ona/core/header/object.hpp"
#include "ona/core/header/memory.hpp"
#include "ona/core/header/array.hpp"
#include "ona/core/header/image.hpp"
#include "ona/core/header/random.hpp"

#endif
