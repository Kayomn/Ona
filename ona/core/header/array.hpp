
namespace Ona::Core {
	template<typename Type> class Array : public Object {
		public:
		virtual Type & At(size_t index) = 0;

		virtual Type const & At(size_t index) const = 0;

		virtual size_t Length() const = 0;

		virtual Type * Pointer() = 0;

		virtual Type const * Pointer() const = 0;

		virtual Slice<Type> Sliced(size_t a, size_t b) = 0;

		virtual Slice<Type const> Sliced(size_t a, size_t b) const = 0;

		virtual Slice<Type> Values() = 0;

		virtual Slice<Type const> Values() const = 0;
	};

	template<typename Type, size_t Len> class InlineArray final : public Array<Type> {
		Type buffer[Len];

		public:
		InlineArray() = default;

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
		DynamicArray(Allocator * allocator, size_t length) : allocator{allocator} {
			if (length) {
				this->buffer = this->allocator->Allocate(sizeof(Type) * length).template As<Type>();
			} else {
				// Initialize dynamic array pointer to non-zero but invalid address so that it is
				// empty but not considered invalid by "IsInitialized".
				this->buffer = Slice<Type>{
					.length = 0,
					.pointer = reinterpret_cast<Type *>(0x1)
				};
			}
		}

		~DynamicArray() override {
			if (this->buffer.pointer) this->allocator->Deallocate(this->buffer.pointer);
		}

		Type & At(size_t index) override {
			return this->buffer.At(index);
		}

		Type const & At(size_t index) const override {
			return this->buffer.At(index);
		}

		bool IsInitialized() const override {
			return (this->buffer.pointer != nullptr);
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
