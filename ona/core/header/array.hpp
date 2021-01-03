
namespace Ona::Core {
	template<typename Type> class Array : public Object {
		public:
		/**
		 * Retrieves the value by reference at `index`.
		 *
		 * Specifying an invalid `index` will result in a runtime error.
		 */
		virtual Type & At(size_t index) = 0;

		/**
		 * Retrieves the value by reference at `index`.
		 *
		 * Specifying an invalid `index` will result in a runtime error.
		 */
		virtual Type const & At(size_t index) const = 0;

		/**
		 * Gets the number of elements in the `Array`.
		 */
		virtual size_t Length() const = 0;

		/**
		 * Gets a pointer to the head of the `Array`.
		 */
		virtual Type * Pointer() = 0;

		/**
		 * Gets a pointer to the head of the `Array`.
		 */
		virtual Type const * Pointer() const = 0;

		/**
		 * Creates a `Slice` of the `Array` from element `a` of `b` elements long.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type> Sliced(size_t a, size_t b) = 0;

		/**
		 * Creates a `Slice` of the `Array` from element `a` of `b` elements long.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type const> Sliced(size_t a, size_t b) const = 0;

		/**
		 * Creates a `Slice` of all elements in the `Array`.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type> Values() = 0;

		/**
		 * Creates a `Slice` of all elements in the `Array`.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type const> Values() const = 0;
	};

	template<typename Type, size_t Len> class InlineArray final : public Array<Type> {
		Type buffer[Len];

		public:
		/**
		 * Initializes the contents with the default values of `Type`.
		 */
		InlineArray() = default;

		/**
		 * Initializes the contents with the contents of `values`.
		 */
		InlineArray(Type (&values)[Len]) {
			for (size_t i = 0; i < Len; i += 1) this->buffer[i] = values[i];
		}

		/**
		 * Assigns the contents of `values` to the `InlineArray`.
		 */
		InlineArray(Type (&&values)[Len]) {
			for (size_t i = 0; i < Len; i += 1) this->buffer[i] = std::move(values[i]);
		}

		Type & At(size_t index) override {
			assert(index < Len);

			return this->buffer[index];
		}

		Type const & At(size_t index) const override {
			assert(index < Len);

			return this->buffer[index];
		}

		size_t Length() const override {
			return Len;
		}

		Type * Pointer() override {
			return this->buffer;
		}

		Type const * Pointer() const override {
			return this->buffer;
		}

		Slice<Type> Sliced(size_t a, size_t b) override {
			return Slice<Type>{
				.length = b,
				.pointer = (this->buffer + a)
			};
		}

		Slice<Type const> Sliced(size_t a, size_t b) const override {
			return Slice<Type const>{
				.length = b,
				.pointer = (this->buffer + a)
			};
		}

		Slice<Type> Values() override {
			return Slice<Type>{
				.length = Len,
				.pointer = this->buffer
			};
		}

		Slice<Type const> Values() const override {
			return Slice<Type const>{
				.length = Len,
				.pointer = this->buffer
			};
		}
	};

	template<typename Type> class DynamicArray final : public Array<Type> {
		Allocator * allocator;

		Slice<Type> buffer;

		public:
		/**
		 * Allocates a buffer using `allocator` of `length` elements and initializes it with the
		 * default contents of `Type`.
		 */
		DynamicArray(Allocator * allocator, size_t length) : allocator{allocator} {
			this->buffer = Slice<Type>{.pointer = reinterpret_cast<Type *>(
				this->allocator->Allocate(sizeof(Type) * length)
			)};

			if (this->buffer.pointer != nullptr) {
				this->buffer.length = length;

				// Initialize array contents.
				for (size_t i = 0; i < length; i += 1) this->buffer.At(i) = Type{};
			}
		}

		~DynamicArray() override {
			if (this->buffer.pointer) {
				this->allocator->Deallocate(this->buffer.pointer);

				// Destruct array contents.
				for (size_t i = 0; i < this->buffer.length; i += 1) this->buffer.At(i).~Type();
			}
		}

		Type & At(size_t index) override {
			return this->buffer.At(index);
		}

		Type const & At(size_t index) const override {
			return this->buffer.At(index);
		}

		size_t Length() const override {
			return this->buffer.length;
		}

		Type * Pointer() override {
			return this->buffer.pointer;
		}

		Type const * Pointer() const override {
			return this->buffer.pointer;
		}

		/**
		 * Releases ownership of the allocated buffer from the `DynamicArray` and passes it to the
		 * caller wrapped in a `Slice`.
		 *
		 * As this releases dynamic memory, the responsibility of lifetime management becomes the
		 * programmer's, as the `DynamicArray` no longer knows about it.
		 *
		 * If the `DynamicArray` contains zero elements, an empty `Slice` is returned.
		 */
		Slice<Type> Release() {
			Slice<Type> slice = this->buffer;
			this->buffer = Slice<Type>{};

			return slice;
		}

		Slice<Type> Sliced(size_t a, size_t b) override {
			return this->buffer.Sliced(a, b);
		}

		Slice<Type const> Sliced(size_t a, size_t b) const override {
			return this->buffer.Sliced(a, b);
		}

		Slice<Type> Values() override {
			return this->buffer;
		}

		Slice<Type const> Values() const override {
			return this->buffer;
		}
	};
}
