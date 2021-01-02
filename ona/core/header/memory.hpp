
namespace Ona::Core {
	class Allocator : public Object {
		public:
		virtual uint8_t * Allocate(size_t size) = 0;

		virtual void Deallocate(void * allocation) = 0;

		virtual uint8_t * Reallocate(void * allocation, size_t size) = 0;
	};

	template<typename Type> class Owned final : public Object {
		public:
		Type value;

		Owned(Type const & value) : value{value} { }

		~Owned() override {
			this->value.Free();
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
