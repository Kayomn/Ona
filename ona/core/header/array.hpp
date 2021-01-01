
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

		virtual size_t Length() const override {
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
	};
}
