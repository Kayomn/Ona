
namespace Ona {
	/**
	 * An `Array` with a compile-time known fixed length.
	 *
	 * `FixedArray` benefits from using aligned memory, and as such, cannot be resized in any way.
	 */
	template<typename Type, size_t Len> class FixedArray final : public Array<Type> {
		Type buffer[Len];

		public:
		enum {
			FixedLength = Len,
		};

		/**
		 * Initializes the contents with the default values of `Type`.
		 */
		FixedArray() {
			for (size_t i = 0; i < Len; i += 1) this->buffer[i] = Type{};
		}

		/**
		 * Initializes the contents with the contents of `values`.
		 */
		FixedArray(Type (&values)[Len]) {
			for (size_t i = 0; i < Len; i += 1) this->buffer[i] = values[i];
		}

		/**
		 * Assigns the contents of `values` to the `FixedArray`.
		 */
		FixedArray(Type (&&values)[Len]) {
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

	/**
	 * An `Array` with a dynamically-sized length.
	 *
	 * `DynamicArray` trades initialization speed and buffer locality for the ability to assign and
	 * change the number of elements at runtime.
	 */
	template<typename Type> class DynamicArray final : public Array<Type> {
		Allocator allocator;

		Slice<Type> buffer;

		public:
		/**
		 * Assigns `allocator` to a zero-length array of `Type` for use when resizing.
		 */
		DynamicArray(Allocator allocator) : allocator{allocator}, buffer{} {

		}

		/**
		 * Allocates a buffer using `allocator` of `length` elements and initializes it with the
		 * default contents of `Type`.
		 */
		DynamicArray(Allocator allocator, size_t length) : allocator{allocator} {
			this->buffer = Slice<Type>{.pointer = reinterpret_cast<Type *>(
				Allocate(this->allocator, sizeof(Type) * length)
			)};

			if (this->buffer.pointer != nullptr) {
				this->buffer.length = length;

				// Initialize array contents.
				for (size_t i = 0; i < length; i += 1) this->buffer.At(i) = Type{};
			}
		}

		~DynamicArray() override {
			if (this->buffer.pointer) {
				Deallocate(this->allocator, this->buffer.pointer);

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

		/**
		 * Attempts to resize the `DynamicArray` buffer at runtime.
		 *
		 * A return value of `false` means that the operation failed and the `DynamicArray` now has
		 * a length of `0`.
		 */
		bool Resize(size_t length) {
			this->buffer.pointer = reinterpret_cast<Type *>(
				Reallocate(this->allocator, this->buffer.pointer, sizeof(Type) * length)
			);

			bool const allocationSucceeded = (this->buffer.pointer != nullptr);
			this->buffer.length = (length * allocationSucceeded);

			return allocationSucceeded;
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

	/**
	 * Small-buffer optimized `Array` with a dynamically-sized length.
	 *
	 * `InlineArray` meets in-between the speed of `Ona::FixedArray` with the flexibility of
	 * `Ona::DynamicArray`. Allocations smaller than or equal to `Len` are stored in-line within
	 * internal buffer storage. Once a `Ona::InlineArray` instance exceeds `Len` elements of space,
	 * the buffer is re-allocated dynamically using the `Ona::Allocator` if any.
	 */
	template<typename Type, size_t Len> class InlineArray final : public Array<Type> {
		Allocator allocator;

		union {
			Slice<Type> dynamic;

			Type static_[Len];
		} buffer;

		public:
		/**
		 * Assigns `allocator` to a zero-length array of `Type` for use when resizing.
		 */
		InlineArray(Allocator allocator) : allocator{allocator}, buffer{} {
			// TODO(Kayomn): Implement.
		}

		/**
		 * Allocates a buffer using `allocator` of `length` elements and initializes it with the
		 * default contents of `Type`.
		 */
		InlineArray(Allocator allocator, size_t length) : allocator{allocator} {
			// TODO(Kayomn): Implement.
		}

		~InlineArray() override {
			// TODO(Kayomn): Implement.
		}

		Type & At(size_t index) override {
			// TODO(Kayomn): Implement.
		}

		Type const & At(size_t index) const override {
			// TODO(Kayomn): Implement.
		}

		size_t Length() const override {
			// TODO(Kayomn): Implement.
		}

		Type * Pointer() override {
			// TODO(Kayomn): Implement.
		}

		Type const * Pointer() const override {
			// TODO(Kayomn): Implement.
		}

		Slice<Type> Sliced(size_t a, size_t b) override {
			// TODO(Kayomn): Implement.
		}

		Slice<Type const> Sliced(size_t a, size_t b) const override {
			// TODO(Kayomn): Implement.
		}

		Slice<Type> Values() override {
			// TODO(Kayomn): Implement.
		}

		Slice<Type const> Values() const override {
			// TODO(Kayomn): Implement.
		}
	};
}
