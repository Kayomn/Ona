#ifndef CORE_H
#define CORE_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include <initializer_list>
#include <memory>

#define $packed __attribute__ ((packed))

#define $length(arr) (sizeof(arr) / sizeof(*arr))

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

		template<typename CastType> using Casted = std::conditional_t<
			std::is_const<Type>::value,
			CastType const,
			CastType
		>;

		template<typename CastType> Slice<Casted<CastType>> As() const {
			using ToType = Casted<CastType>;

			return Slice<ToType>{
				(this->length / sizeof(ToType)),
				reinterpret_cast<ToType *>(this->pointer)
			};
		}

		/**
		 * Reinterprets the `Slice` as a `Slice` of raw, unsigned bytes.
		 */
		constexpr Slice<Casted<uint8_t>> AsBytes() const {
			return Slice<Casted<uint8_t>>{
				(this->length * sizeof(Type)),
				reinterpret_cast<Casted<uint8_t> *>(this->pointer)
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
		 * Provides casting to a version of `Slice` with a `const` `Type`.
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

		public:
		Callable() = default;

		Callable(Callable const & that) {
			for (size_t i = 0; i < BufferSize; i += 1) this->buffer[i] = that.buffer[i];

			this->context = reinterpret_cast<Context *>(this->buffer);
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

		bool IsEmpty() {
			return (this->context != nullptr);
		}
	};

	using Chars = Slice<char const>;

	constexpr Chars CharsFrom(char const * pointer) {
		size_t length = 0;

		while (*(pointer + length)) length += 1;

		return SliceOf(pointer, length);
	}

	template<typename ValueType, typename ErrorType> struct Result final {
		private:
		static constexpr size_t CalculateStoreSize() {
			if constexpr (std::is_same_v<ErrorType, void>) {
				return sizeof(ValueType);
			} else {
				return std::max(sizeof(ValueType), sizeof(ErrorType));
			}
		}

		enum { StoreSize = CalculateStoreSize() };

		uint8_t store[StoreSize + 1];

		public:
		Result() = default;

		Result(Result const & that) {
			this->store[StoreSize] = that.store[StoreSize];

			if constexpr (std::is_same_v<ErrorType, void>) {
				if (this->store[StoreSize]) new (this->store) ValueType{that.Value()};
			} else {
				if (this->store[StoreSize]) {
					new (this->store) ValueType{that.Value()};
				} else {
					new (this->store) ErrorType{that.Error()};
				}
			}
		}

		~Result() {
			if constexpr (std::is_destructible_v<ValueType>) {
				if (this->IsOk()) {
					this->Value().~ValueType();

					return;
				}
			}

			if constexpr (std::is_destructible_v<ErrorType>) {
				this->Error().~ErrorType();
			}
		}

		ErrorType const & Error() const {
			assert((!this->IsOk()) && "Result is ok");

			return (*reinterpret_cast<ErrorType const *>(this->store));
		}

		ValueType & Expect(Chars const & message) {
			assert(this->IsOk() && message.pointer);

			return (*reinterpret_cast<ValueType *>(this->store));
		}

		ValueType const & Expect(Chars const & message) const {
			assert(this->IsOk() && message.pointer);

			return (*reinterpret_cast<ValueType const *>(this->store));
		}

		bool IsOk() const {
			return static_cast<bool>(this->store[StoreSize]);
		}

		ValueType & Value() {
			return this->Expect(CharsFrom("Result is erroneous"));
		}

		ValueType const & Value() const {
			return this->Expect(CharsFrom("Result is erroneous"));
		}

		static Result Ok(ValueType const & value) {
			Result result = {};
			result.store[StoreSize] = 1;

			new (result.store) ValueType{value};

			return result;
		}

		static Result Fail(ErrorType const & error) {
			Result result = {};

			new (result.store) ErrorType{error};

			return result;
		}
	};
}

#include "ona/core/header/text.hpp"
#include "ona/core/header/object.hpp"
#include "ona/core/header/memory.hpp"
#include "ona/core/header/array.hpp"
#include "ona/core/header/image.hpp"

void * operator new(size_t count, Ona::Core::Allocator * allocator);

void operator delete(void * pointer, Ona::Core::Allocator * allocator);

#endif
