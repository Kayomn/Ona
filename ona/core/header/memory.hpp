
namespace Ona::Core {
	class Allocator : public Object {
		public:
		virtual Slice<uint8_t> Allocate(size_t size) = 0;

		virtual void Deallocate(void * allocation) = 0;

		virtual Slice<uint8_t> Reallocate(void * allocation, size_t size) = 0;
	};

	template<typename Type> class Unique final : public Object {
		public:
		Type value;

		Unique(Type const & value) : value{value} { }

		~Unique() override {
			this->value.Free();
		}

		Type & ValueOf() {
			return this->value;
		}
	};

	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> const & source);

	Slice<uint8_t> WriteMemory(Slice<uint8_t> destination, uint8_t value);

	template<typename Type> Slice<uint8_t const> AsBytes(Type const & value) {
		return SliceOf(&value, 1).AsBytes();
	}

	/**
	 * Zeroes the memory contents of `destination`.
	 */
	Slice<uint8_t> ZeroMemory(Slice<uint8_t> destination);
}
